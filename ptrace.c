/* ptrace.c - pipeline tracing routines */

/* SimpleScalar(TM) Tool Suite
 * Copyright (C) 1994-2003 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
 * All Rights Reserved. 
 * 
 * THIS IS A LEGAL DOCUMENT, BY USING SIMPLESCALAR,
 * YOU ARE AGREEING TO THESE TERMS AND CONDITIONS.
 * 
 * No portion of this work may be used by any commercial entity, or for any
 * commercial purpose, without the prior, written permission of SimpleScalar,
 * LLC (info@simplescalar.com). Nonprofit and noncommercial use is permitted
 * as described below.
 * 
 * 1. SimpleScalar is provided AS IS, with no warranty of any kind, express
 * or implied. The user of the program accepts full responsibility for the
 * application of the program and the use of any results.
 * 
 * 2. Nonprofit and noncommercial use is encouraged. SimpleScalar may be
 * downloaded, compiled, executed, copied, and modified solely for nonprofit,
 * educational, noncommercial research, and noncommercial scholarship
 * purposes provided that this notice in its entirety accompanies all copies.
 * Copies of the modified software can be delivered to persons who use it
 * solely for nonprofit, educational, noncommercial research, and
 * noncommercial scholarship purposes provided that this notice in its
 * entirety accompanies all copies.
 * 
 * 3. ALL COMMERCIAL USE, AND ALL USE BY FOR PROFIT ENTITIES, IS EXPRESSLY
 * PROHIBITED WITHOUT A LICENSE FROM SIMPLESCALAR, LLC (info@simplescalar.com).
 * 
 * 4. No nonprofit user may place any restrictions on the use of this software,
 * including as modified by the user, by any other authorized user.
 * 
 * 5. Noncommercial and nonprofit users may distribute copies of SimpleScalar
 * in compiled or executable form as set forth in Section 2, provided that
 * either: (A) it is accompanied by the corresponding machine-readable source
 * code, or (B) it is accompanied by a written offer, with no time limit, to
 * give anyone a machine-readable copy of the corresponding source code in
 * return for reimbursement of the cost of distribution. This written offer
 * must permit verbatim duplication by anyone, or (C) it is distributed by
 * someone who received only the executable form, and is accompanied by a
 * copy of the written offer of source code.
 * 
 * 6. SimpleScalar was developed by Todd M. Austin, Ph.D. The tool suite is
 * currently maintained by SimpleScalar LLC (info@simplescalar.com). US Mail:
 * 2395 Timbercrest Court, Ann Arbor, MI 48105.
 * 
 * Copyright (C) 1994-2003 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "range.h"
#include "ptrace.h"
#include "ring_buffer.h"
#include "capture_buf.h"

/* pipetrace file */
FILE *ptrace_outfd = NULL;

/* ##### konata trace file ######*/
FILE *konata_file = NULL;

FILE *ring_konata = NULL;

/* pipetracing is active */
int ptrace_active = FALSE;

/* ##### konata tracing is active ##### */
int konata_active = FALSE;

/* pipetracing range */
struct range_range_t ptrace_range;

/* one-shot switch for pipetracing */
int ptrace_oneshot = FALSE;

static RingBuffer *rb = NULL;

int is_event = FALSE;

char *event_name = NULL;

typedef enum { CAP_IDLE = 0, CAP_WITH_SNAPSHOT, CAP_POST_ONLY } CapState;
static CapState cap_state = CAP_IDLE;
static int      cap_active = 0;               /* mirrors on/off */
static int      recent_dump = 0;              /* last segment already dumped */
static size_t   cap_before_entries = 128;     /* tunable */
static size_t   cap_post_cycles_target = 50;  /* tunable */
static size_t   cap_post_cycles_seen = 0;
static CaptureBuf capbuf;
static TraceEntry tmp_before[512];            /* temp for snapshot */
static tick_t  current_cycle = 0;

static inline void rb_logf(int cycle, const char *fmt, ...) {
    char line[RING_BUFFER_ENTRY_BYTES];
    va_list ap; va_start(ap, fmt);
    vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);
    rb_push_raw(rb, cycle, line);
    if (cap_active) cap_append(&capbuf, cycle, line);
}

static inline void START_MIRROR(void){ cap_active = 1; }
static inline void STOP_MIRROR(void) { cap_active = 0; }
static inline void RESET_COUNTDOWN(void){ cap_post_cycles_seen = 0; }
static inline void INC_COUNTDOWN(void){ ++cap_post_cycles_seen; }

static void SNAPSHOT_BEFORE(size_t N) {
    size_t got = rb_copy_last(rb, N, tmp_before, sizeof(tmp_before)/sizeof(tmp_before[0]));
    cap_reset(&capbuf);
    for (size_t i=0; i<got; ++i)
        if (tmp_before[i].valid) cap_append(&capbuf, tmp_before[i].cycle, tmp_before[i].entry);
}

static void DUMP_AND_RESET(FILE *out) {
    fprintf(out, "----- segment dump -----\n");
    cap_flush(&capbuf, out);
    cap_reset(&capbuf);
    STOP_MIRROR();
    recent_dump = 1;
    cap_state = CAP_IDLE;
    is_event = 0;
    event_name = NULL;
    rb_clear(rb); /* optional, but keeps ring_konata segments cleanly separated */
}


/* open pipeline trace */
void
ptrace_open(char *fname,		/* output filename */
	    char *range)		/* trace range */
{
  char *errstr;

  /*###########################################*/
  printf("############################### MY TEST #################################\n");

  /* parse the output range */
  if (!range)
    {
      /* no range */
      errstr = range_parse_range(":", &ptrace_range);
      if (errstr)
	panic("cannot parse pipetrace range, use: {<start>}:{<end>}");
      ptrace_active = TRUE;
    }
  else
    {
      errstr = range_parse_range(range, &ptrace_range);
      if (errstr)
	fatal("cannot parse pipetrace range, use: {<start>}:{<end>}");
      ptrace_active = FALSE;
    }

  if (ptrace_range.start.ptype != ptrace_range.end.ptype)
    fatal("range endpoints are not of the same type");

  /* open output trace file */
  if (!fname || !strcmp(fname, "-") || !strcmp(fname, "stderr"))
    ptrace_outfd = stderr;
  else if (!strcmp(fname, "stdout"))
    ptrace_outfd = stdout;
  else
    {
      ptrace_outfd = fopen(fname, "w");

      /* name konata file */
      char konata_name[1024];
      char ring_konata_name[1024];
      snprintf(konata_name, sizeof(konata_name), "%s.txt", fname);
      snprintf(ring_konata_name, sizeof(ring_konata_name), "%s.ring.txt", fname);
      /* open konata file */
      konata_file = fopen(konata_name, "w");
      ring_konata = fopen(ring_konata_name, "w");
      if (!konata_file) 
        fatal("cannot open pipetrace output file `%s'", fname);

      /* header */
      fprintf(konata_file, "Kanata 0004\n");
      fprintf(ring_konata, "Kanata 0004\n");
      rb = malloc(sizeof(RingBuffer));
      rb_init(rb);
      cap_init(&capbuf, 512);
      printf("rb is initialized, BUFFER_SIZE = %d\n", RING_BUFFER_SIZE); 
      if(!rb)
	fatal("RingBuffer is not initialized");

      if (!ptrace_outfd)
	fatal("cannot open pipetrace output file `%s'", fname);
    }
}

/* close pipeline trace */
void
ptrace_close(void)
{
  if (ptrace_outfd != NULL && ptrace_outfd != stderr && ptrace_outfd != stdout)
    fclose(ptrace_outfd);

  /* ##### close konata trace file ##### */
  fflush(konata_file);
  fclose(konata_file);
  fflush(ring_konata);
  fclose(ring_konata);
  free(rb);
  cap_free(&capbuf);
}

#define PTRACE_C
/* declare a new instruction */
void
__ptrace_newinst(unsigned int iseq,	/* instruction sequence number */
		 md_inst_t inst,	/* new instruction */
		 md_addr_t pc,		/* program counter of instruction */
		 md_addr_t addr,        /* address referenced, if load/store */
		 tick_t cycle)
{
  myfprintf(ptrace_outfd, "+ %u 0x%08p 0x%08p ", iseq, pc, addr);

  /* ##### konata add 'I' command */
  fprintf(konata_file, "I\t%u\t%u\t%u\n", iseq, iseq, 0);

  /* rb_pushf */
  //rb_pushf(rb, cycle, "I\t%u\t%u\t%u\n", iseq, iseq, 0);
  fprintf(ring_konata, "I\t%u\t%u\t%u\n", iseq, iseq, 0);

  /* ##### konata add 'L' command, this need to execute before 'md_print_insn_konata'*/
  fprintf(konata_file, "L\t%u\t0\t%.8llx:", iseq, pc);
	
  md_print_insn(inst, addr, ptrace_outfd);

  /* ##### Enter 'md_print_insn_konata' to continue produce 'L''s konata output ##### */
  md_print_insn_konata(inst, addr, konata_file);

  /* md_print_insn_rb */
  char *buffer = md_print_insn_rb(inst, addr);
  //rb_pushf(rb, cycle, "L\t%u\t0\t%.8llx:%s Cycle: %.0f\n", iseq, pc, buffer, (double)cycle);
  rb_logf(cycle, "L\t%u\t0\t%.8llx:%s Cycle: %.0f\n", iseq, pc, buffer, (double)cycle);
  fprintf(ptrace_outfd, "\n");

  fprintf(konata_file, "\n");

  if (ptrace_outfd == stderr || ptrace_outfd == stdout)
    fflush(ptrace_outfd);
}

/* declare a new uop */
void
__ptrace_newuop(unsigned int iseq,	/* instruction sequence number */
		char *uop_desc,		/* new uop description */
		md_addr_t pc,		/* program counter of instruction */
		md_addr_t addr)		/* address referenced, if load/store */
{
  myfprintf(ptrace_outfd,
	    "+ %u 0x%08p 0x%08p [%s]\n", iseq, pc, addr, uop_desc);

  if (ptrace_outfd == stderr || ptrace_outfd == stdout)
    fflush(ptrace_outfd);

  /* ##### konata add 'I' command */
  fprintf(konata_file, "I\t%u\t%u\t%u\n", iseq, iseq, 0);

  //rb_pushf(rb, 0, "I\t%u\t%u\t%u\n", iseq, iseq, 0);
  rb_logf(0, "I\t%u\t%u\t%u\n", iseq, iseq, 0);
  /* ##### konata add 'L' command, this need to execute before 'md_print_insn_konata'*/
  fprintf(konata_file, "L\t%u\t0\t%.8llx:[%s]\n", iseq, pc, uop_desc);

  //rb_pushf(rb, 0, "L\t%u\t0\t%.8llx:[%s]\n", iseq, pc, uop_desc);
  rb_logf(0, "L\t%u\t0\t%.8llx:[%s]\n", iseq, pc, uop_desc);
}

/* declare instruction retirement or squash */
void
__ptrace_endinst(unsigned int iseq)	/* instruction sequence number */
{
  fprintf(ptrace_outfd, "- %u\n", iseq);

  if (ptrace_outfd == stderr || ptrace_outfd == stdout)
    fflush(ptrace_outfd);

  /* ##### konata add 'R' command */
  fprintf(konata_file, "R\t%u\t0\t0\n", iseq);
  //rb_pushf(rb, 0,  "R\t%u\t0\t0\n", iseq);
  rb_logf(0,  "R\t%u\t0\t0\n", iseq);

}

/* declare a new cycle */
void
__ptrace_newcycle(tick_t cycle)		/* new cycle */
{
  fprintf(ptrace_outfd, "@ %.0f\n", (double)cycle);
  
  current_cycle = cycle;
  /* ##### konata new cycle ##### */
  fprintf(konata_file, "C\t1\n");
  //rb_pushf(rb, cycle,  "C\t1\n");
  rb_logf((int)cycle,  "C\t1\n"); 
 /* 
  if(is_event) {
	  
    fprintf(ring_konata, "-------------------Cycle: %.0f Event name:%s-----------------\n", (double)cycle, event_name);
    
    rb_dump_before(rb, 128, ring_konata);
    rb_clear(rb);
    is_event = 0;
  }
*/
  if (cap_state != CAP_IDLE) {
    INC_COUNTDOWN();
    if (cap_post_cycles_seen >= cap_post_cycles_target) {
        DUMP_AND_RESET(ring_konata);
    }
  }

  if (ptrace_outfd == stderr || ptrace_outfd == stdout)
    fflush(ptrace_outfd);
}

/* indicate instruction transition to a new pipeline stage */
void
__ptrace_newstage(unsigned int iseq,	/* instruction sequence number */
		  char *pstage,		/* pipeline stage entered */
		  unsigned int pevents)/* pipeline events while in stage */
{
  fprintf(ptrace_outfd, "* %u %s 0x%08x\n", iseq, pstage, pevents);

  fprintf(konata_file,  "S\t%u\t0\t%s\n", iseq, pstage);

  //rb_pushf(rb, 0, "S\t%u\t0\t%s\n", iseq, pstage);  
  rb_logf(0, "S\t%u\t0\t%s\n", iseq, pstage);
  if (ptrace_outfd == stderr || ptrace_outfd == stdout)
    fflush(ptrace_outfd);

  const char *event = NULL;

  if (pevents & PEV_CACHEMISS) {
        event = "i-cache-miss";
	is_event = 1;
	event_name = event;
	    if (cap_state == CAP_IDLE) {
              if (!recent_dump) {
              /* first event in segment -> snapshot before + start mirroring */
              SNAPSHOT_BEFORE(cap_before_entries);
              START_MIRROR();
              RESET_COUNTDOWN();
              cap_state = CAP_WITH_SNAPSHOT;
              } else {
            /* new segment after a recent dump -> mirror only */
              START_MIRROR();
              RESET_COUNTDOWN();
              cap_state = CAP_POST_ONLY;
                }
            } else {
              /* already capturing: extend window */
              RESET_COUNTDOWN();
              }
  

  } else if (pevents & PEV_TLBMISS) {
        event = "i-tlb-miss";
  } else if (pevents & PEV_MPOCCURED) {
        event = "mis-pred-branch-occured";
  } else if (pevents & PEV_MPDETECT) {
        event = "mis-pred-branch-detected";
  } else if (pevents & PEV_AGEN) {
        event = "address-generation";
  }

  if (event != NULL) {
      fprintf(konata_file,  "L\t%u\t1\t%s\n", iseq, event);
      //rb_pushf(rb, 0, "L\t%u\t1\t%s\n", iseq, event);
      rb_logf(0, "L\t%u\t1\t%s\n", iseq, event);
  }
}
