/* sim-fast.c - sample fast functional simulator implementation */

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
#include <math.h>
#include <setjmp.h>
/*
 * This file implements a very fast functional simulator.  This functional
 * simulator implementation is much more difficult to digest than the simpler,
 * cleaner sim-safe functional simulator.  By default, this simulator performs
 * no instruction error checking, as a result, any instruction errors will
 * manifest as simulator execution errors, possibly causing sim-fast to
 * execute incorrectly or dump core.  Such is the price we pay for speed!!!!
 *
 * The following options configure the bag of tricks used to make sim-fast
 * live up to its name.  For most machines, defining all the options results
 * in the fastest functional simulator.
 */

/* don't count instructions flag, enabled by default, disable for inst count */
#undef NO_INSN_COUNT

#ifdef __GNUC__
/* faster dispatch mechanism, requires GNU GCC C extensions, CAVEAT: some
   versions of GNU GCC core dump when optimizing the jump table code with
   optimization levels higher than -O1 */
/* #define USE_JUMP_TABLE */
#endif /* __GNUC__ */

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "regs.h"
#include "memory.h"
#include "loader.h"
#include "syscall.h"
#include "dlite.h"
#include "sim.h"
#include "encode.def"

/* simulated registers */
static struct regs_t regs;

/* simulated memory */
static struct mem_t *mem = NULL;

/* lrsc recorder */
static struct lrsc_t *lrsc = NULL;

#ifdef TARGET_ALPHA
/* predecoded text memory */
static struct mem_t *dec = NULL;
#elif defined(TARGET_RISCV)
#define FENCE()
#endif

/* register simulator-specific options */
void
sim_reg_options(struct opt_odb_t *odb) {
    opt_reg_header(odb,
                   "sim-fast: This simulator implements a very fast functional simulator.  This\n"
                   "functional simulator implementation is much more difficult to digest than\n"
                   "the simpler, cleaner sim-safe functional simulator.  By default, this\n"
                   "simulator performs no instruction error checking, as a result, any\n"
                   "instruction errors will manifest as simulator execution errors, possibly\n"
                   "causing sim-fast to execute incorrectly or dump core.  Such is the\n"
                   "price we pay for speed!!!!\n"
    );
}

/* check simulator-specific option values */
void
sim_check_options(struct opt_odb_t *odb, int argc, char **argv) {
    if (dlite_active)
        fatal("sim-fast does not support DLite debugging");
}

/* register simulator-specific statistics */
void
sim_reg_stats(struct stat_sdb_t *sdb) {
#ifndef NO_INSN_COUNT
    stat_reg_counter(sdb, "sim_num_insn",
                     "total number of instructions executed",
                     &sim_num_insn, sim_num_insn, NULL);
#endif /* !NO_INSN_COUNT */
    stat_reg_int(sdb, "sim_elapsed_time",
                 "total simulation time in seconds",
                 &sim_elapsed_time, 0, NULL);
#ifndef NO_INSN_COUNT
    stat_reg_formula(sdb, "sim_inst_rate",
                     "simulation speed (in insts/sec)",
                     "sim_num_insn / sim_elapsed_time", NULL);
#endif /* !NO_INSN_COUNT */
    ld_reg_stats(sdb);
    mem_reg_stats(mem, sdb);
}

struct lrsc_t* lrsc_create(){
  struct lrsc_t* lrsc = (struct lrsc_t*)malloc(sizeof(struct lrsc_t));
  return lrsc;
}

void lrsc_init(struct lrsc_t *lrsc){
  memset(lrsc, 0, sizeof(*lrsc));
}

/* initialize the simulator */
void
sim_init(void) {
    /* allocate and initialize register file */
    regs_init(&regs);

    /* allocate and initialize memory space */
    mem = mem_create("mem");
    mem_init(mem);

    /* allocate and initialize LR/SC recorder */
    lrsc = lrsc_create();
    lrsc_init(lrsc);
}
/* load program into simulated state */
void
sim_load_prog(char *fname,        /* program to load */
              int argc, char **argv,    /* program arguments */
              char **envp)        /* program environment */
{
    /* load program text and data, set up environment, memory, and regs */
    ld_load_prog(fname, argc, argv, envp, &regs, mem, TRUE);

#ifdef TARGET_ALPHA
    /* pre-decode text segment */
    {
      unsigned i, num_insn = (ld_text_size + 3) / 4;

      fprintf(stderr, "** pre-decoding %u insts...", num_insn);

      /* allocate decoded text space */
      dec = mem_create("dec");

      for (i=0; i < num_insn; i++)
        {
      enum md_opcode op;
      md_inst_t inst;
      md_addr_t PC;

      /* compute PC */
      PC = ld_text_base + i * sizeof(md_inst_t);

      /* get instruction from memory */
      MD_FETCH_INST(inst, mem, PC);

      /* decode the instruction */
      MD_SET_OPCODE(op, inst);

      /* insert into decoded opcode space */
      MEM_WRITE_WORD(dec, PC << 1, (word_t)op);
      MEM_WRITE_WORD(dec, (PC << 1)+sizeof(word_t), inst);
        }
      fprintf(stderr, "done\n");
    }
#endif /* TARGET_ALPHA */
}

/* print simulator-specific configuration information */
void
sim_aux_config(FILE *stream) {
    /* nothing currently */
}

/* dump simulator-specific auxiliary simulator statistics */
void
sim_aux_stats(FILE *stream) {
    /* nada */
}

/* un-initialize simulator-specific state */
void
sim_uninit(void) {
    /* nada */
}
/*
 * configure the execution engine
 */

/* next program counter */
#define SET_NPC(EXPR)        (regs.regs_NPC = (EXPR))

/* current program counter */
#define CPC            (regs.regs_PC)

/* general purpose registers */
#define GPR(N)            (regs.regs_R[N])
#define SET_GPR(N, EXPR)        (regs.regs_R[N] = (EXPR))

#if defined(TARGET_PISA)

/* floating point registers, L->word, F->single-prec, D->double-prec */
#define FPR_L(N)		(regs.regs_F.l[(N)])
#define SET_FPR_L(N,EXPR)	(regs.regs_F.l[(N)] = (EXPR))
#define FPR_F(N)		(regs.regs_F.f[(N)])
#define SET_FPR_F(N,EXPR)	(regs.regs_F.f[(N)] = (EXPR))
#define FPR_D(N)		(regs.regs_F.d[(N) >> 1])
#define SET_FPR_D(N,EXPR)	(regs.regs_F.d[(N) >> 1] = (EXPR))

/* miscellaneous register accessors */
#define SET_HI(EXPR)		(regs.regs_C.hi = (EXPR))
#define HI			(regs.regs_C.hi)
#define SET_LO(EXPR)		(regs.regs_C.lo = (EXPR))
#define LO			(regs.regs_C.lo)
#define FCC			(regs.regs_C.fcc)
#define SET_FCC(EXPR)		(regs.regs_C.fcc = (EXPR))

#elif defined(TARGET_ALPHA)

/* floating point registers, L->word, F->single-prec, D->double-prec */
#define FPR_Q(N)		(regs.regs_F.q[N])
#define SET_FPR_Q(N,EXPR)	(regs.regs_F.q[N] = (EXPR))
#define FPR(N)			(regs.regs_F.d[N])
#define SET_FPR(N,EXPR)		(regs.regs_F.d[N] = (EXPR))

/* miscellaneous register accessors */
#define FPCR			(regs.regs_C.fpcr)
#define SET_FPCR(EXPR)		(regs.regs_C.fpcr = (EXPR))
#define UNIQ			(regs.regs_C.uniq)
#define SET_UNIQ(EXPR)		(regs.regs_C.uniq = (EXPR))

#elif defined(TARGET_RISCV)

#include "regs.def"

#else
#error No ISA target defined...
#endif

/* precise architected memory state accessor macros */
#define READ_BYTE(SRC, FAULT)                        \
  ((FAULT) = md_fault_none, MEM_READ_BYTE(mem, (SRC)))
#define READ_HALF(SRC, FAULT)                        \
  ((FAULT) = md_fault_none, MEM_READ_HALF(mem, (SRC)))
#define READ_WORD(SRC, FAULT)                        \
  ((FAULT) = md_fault_none, MEM_READ_WORD(mem, (SRC)))
#ifdef HOST_HAS_QWORD
#define READ_QWORD(SRC, FAULT)                        \
  ((FAULT) = md_fault_none, MEM_READ_QWORD(mem, (SRC)))
#endif /* HOST_HAS_QWORD */

#define WRITE_BYTE(SRC, DST, FAULT)                    \
  ((FAULT) = md_fault_none, MEM_WRITE_BYTE(mem, (DST), (SRC)))
#define WRITE_HALF(SRC, DST, FAULT)                    \
  ((FAULT) = md_fault_none, MEM_WRITE_HALF(mem, (DST), (SRC)))
#define WRITE_WORD(SRC, DST, FAULT)                    \
  ((FAULT) = md_fault_none, MEM_WRITE_WORD(mem, (DST), (SRC)))
#ifdef HOST_HAS_QWORD
#define WRITE_QWORD(SRC, DST, FAULT)                    \
  ((FAULT) = md_fault_none, MEM_WRITE_QWORD(mem, (DST), (SRC)))
#endif /* HOST_HAS_QWORD */

/* system call handler macro */
#define SYSCALL(INST)    sys_syscall(&regs, mem_access, mem, INST, TRUE)

#ifndef NO_INSN_COUNT
#define INC_INSN_CTR()    sim_num_insn++
#else /* !NO_INSN_COUNT */
#define INC_INSN_CTR()	/* nada */
#endif /* NO_INSN_COUNT */

#define ZERO_FP_REG()    /* nada... */

/* track whether a control transfer inst has taken */
static bool IsCtrlInstTaken;
#define CtrlInstTaken(RES) { IsCtrlInstTaken = RES; }

/* setting initial field of CSR */
void sim_machine_init(void) {
  /* todo */
  SET_CSRS(MISA, EXT_I|EXT_M|EXT_A|EXT_F|EXT_D|EXT_C);
  SET_CSRS(MHARTID, 0); /* single core: 0 */
}

static counter_t mem_cross_page_refs = 0;

/* start simulation, program loaded, processor precise state initialized */
void
sim_main(void) {
    static int i = 1;

    /* register allocate instruction buffer */
    register md_inst_t inst;

    /* decoded opcode */
    register enum md_opcode op;

    fprintf(stderr, "sim: ** starting *fast* functional simulation **\n");

    /* must have natural byte/word ordering */
    if (sim_swap_bytes || sim_swap_words)
        fatal("sim: *fast* functional simulation cannot swap bytes or words");

    /* set up initial default next PC */
    regs.regs_NPC = regs.regs_PC + sizeof(md_inst_t);
    while (TRUE) {
        /* maintain $r0 semantics */
        regs.regs_R[MD_REG_ZERO] = 0;

        /* maintain track whether control transfer inst has taken */
        IsCtrlInstTaken = false;

        /* keep an instruction count */
#ifndef NO_INSN_COUNT
        sim_num_insn++;
#endif /* !NO_INSN_COUNT */

        MD_FETCH_INST(inst, mem, regs.regs_PC);

        /* decode the instruction */
        MD_SET_OPCODE(op, inst);
        
        debug("%d\t, inst[%x], addr[%x], op[%d]", i, inst, regs.regs_PC, op);

        if(inst == 0x73 && GPR(MD_REG_A7) == 93) {
          if(GPR(MD_REG_A0) == 0)
            fprintf(stderr, "PASS");
          else
            fprintf(stderr, "failed");
        }

        /* execute the instruction */
        switch (op) {
#define DEFINST(OP, MSK, NAME, OPFORM, RES, FLAGS, O1, O2, O3, I1, I2, I3, I4)    \
    case OP:                            \
      SYMCAT(OP,_IMPL);                        \
      /*if(regs.regs_PC > 0x106f2 && regs.regs_PC < 0x10732)*/  \
      debug(NAME);\
      break;
#define DEFLINK(OP, MSK, NAME, MASK, SHIFT, SECOND_SHIFT)                    \
    case OP:                            \
      panic("attempted to execute a linking opcode");
#define CONNECT(OP)

#define DECLARE_FAULT(FAULT)                        \
      { /* uncaught... */break; } 

#include "machine.def"

            default:
                debug("%05x", regs.regs_PC);
                panic("inst: 0x%08x \nattempted to execute a bogus opcode 0x%x", inst, op);
        }
        
        /* print all register state and stack state. */
        if (verbose) {
            md_print_iregs(regs.regs_R, stderr);
            fprintf(stderr, "-\n");
            md_print_fpregs(regs.regs_F, stderr);
            // fprintf(stderr, "-\n");
            //printCertainMemArea(stderr);
            // md_print_stack(regs.regs_R, stderr);
        }

        // if(verbose){
        //   if(regs.regs_PC > 0x106f2 && regs.regs_PC < 0x10732) {
        //     fprintf(stderr, "%d\t, inst[%x], addr[%x], op[%d]\n", i, inst, regs.regs_PC, op);
        //     md_print_iregs(regs.regs_R, stderr);
        //     fprintf(stderr, "-\n");
        //   }
        // }
        
        /* execute next instruction */
        regs.regs_PC = regs.regs_NPC;        
        
        regs.regs_NPC += sizeof(md_inst_t);
        if((INST_IsCompressed(inst) && (!(MD_OP_FLAGS(op) & F_CTRL))) ||
           (INST_IsCompressed(inst) && (MD_OP_FLAGS(op) & F_CTRL) && (!IsCtrlInstTaken)))
          {
            regs.regs_PC -= sizeof(md_c_inst_t);
            regs.regs_NPC -= sizeof(md_c_inst_t);
          }
        i++;
    }

}

void sim_check_ans(void) {}

void 
md_print_stack(md_gpr_t regs, FILE *stream) {
  #define STACK_BASE 0xbfffc000
  #define LAYER_BITS 0x8
  
  enum md_fault_type _fault;
  const md_addr_t stackPoint = regs[2];
  int stackLayer = 0;
  md_addr_t currentSP = STACK_BASE;

  fprintf(stream, "[stackPointer : %#llx]\n",stackPoint);
  
  while(currentSP <= STACK_BASE && currentSP >= stackPoint) {

    if(currentSP != stackPoint) {
      sword_t _result = READ_QWORD(currentSP, _fault);
      fprintf(stream, "%#llx\t|===============================|\n", currentSP);
      int width = fprintf(stream, "           \t|\t%#10llx", _result);
      fprintf(stream, "%*c", 40 - width, '|');
      fprintf(stream, "%5d\n", currentSP - stackPoint);      
    }    
    currentSP = STACK_BASE - (++stackLayer * LAYER_BITS);
  }

  fprintf(stream, "%#llx\t|===============================| StackPoint\n", stackPoint);
  #undef STACK_BASE
  #undef LAYER_BITS
}

void printCertainMemArea(FILE *stream) {
  #define MEM_BASE 0x8eb30
  #define LAYER_BITS 0x8
  #define TARGET_ADDR 64
  #define MAX_MEM_LAYER (TARGET_ADDR / LAYER_BITS)

  enum md_fault_type _fault;
  md_addr_t currentSP = 0;

  for(int layer = MAX_MEM_LAYER; layer >= 0; layer--) {
    union{
      float f;
      double d;
      sword_t sw;
      word_t uw;
      qword_t uq;
      sqword_t sq;
    }_result;
    currentSP = MEM_BASE + layer * LAYER_BITS;
    _result.uq = READ_QWORD(currentSP, _fault);
    // sqword_t _result = READ_QWORD(currentSP, _fault);
    // fprintf(stream, "currentSP: %#llx ; val: %#llx\n", currentSP, _result);
    fprintf(stream, "%#llx\t|===============================|\n", currentSP);
    int width = fprintf(stream, "           \t|\t%#10llx ; %f", _result.sw, _result.d);
      fprintf(stream, "%*c", 39 - width, '|');
      fprintf(stream, "%5d\n", currentSP - MEM_BASE);
  }

  #undef MEM_BASE
  #undef LAYER_BITS
  #undef MAX_MEM_LAYER
}