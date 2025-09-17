/* loader.c - program loader routines */

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

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "endian.h"
#include "regs.h"
#include "memory.h"
#include "sim.h"
#include "eio.h"
#include "loader.h"

#ifdef BFD_LOADER
#include <bfd.h>
#else /* !BFD_LOADER */

#include "target-riscv64/elf.h"

#endif /* BFD_LOADER */

/* amount of tail padding added to all loaded text segments */
#define TEXT_TAIL_PADDING 128

/* program text (code) segment base */
md_addr_t ld_text_base = 0xffffffff;
md_addr_t ld_text_bound = 0;

/* program text (code) size in bytes */
unsigned int ld_text_size = 0;

/* program initialized data segment base */
md_addr_t ld_data_base = 0xffffffff;
md_addr_t ld_data_bound = 0;

/* top of the data segment */
md_addr_t ld_brk_point = 0;

/* program initialized ".data" and uninitialized ".bss" size in bytes */
unsigned int ld_data_size = 0;

/* program stack segment base (highest address in stack) */
md_addr_t ld_stack_base = MD_STACK_BASE;

/* program initial stack size */
unsigned int ld_stack_size = 0;

/* lowest address accessed on the stack */
md_addr_t ld_stack_min = -1;

/* program file name */
char *ld_prog_fname = NULL;

/* program entry point (initial PC) */
md_addr_t ld_prog_entry = 0;

/* program environment base address address */
md_addr_t ld_environ_base = 0;

/* target executable endian-ness, non-zero if big endian */
int ld_target_big_endian;

/* register simulator-specific statistics */
void
ld_reg_stats(struct stat_sdb_t *sdb)    /* stats data base */
{
    stat_reg_addr(sdb, "ld_text_base",
                  "program text (code) segment base",
                  &ld_text_base, ld_text_base, "  0x%08p");
    stat_reg_addr(sdb, "ld_text_bound",
                  "program text (code) segment bound",
                  &ld_text_bound, ld_text_bound, "  0x%08p");
    stat_reg_uint(sdb, "ld_text_size",
                  "program text (code) size in bytes",
                  &ld_text_size, ld_text_size, NULL);
    stat_reg_addr(sdb, "ld_data_base",
                  "program initialized data segment base",
                  &ld_data_base, ld_data_base, "  0x%08p");
    stat_reg_addr(sdb, "ld_data_bound",
                  "program initialized data segment bound",
                  &ld_data_bound, ld_data_bound, "  0x%08p");
    stat_reg_uint(sdb, "ld_data_size",
                  "program init'ed `.data' and uninit'ed `.bss' size in bytes",
                  &ld_data_size, ld_data_size, NULL);
    stat_reg_addr(sdb, "ld_stack_base",
                  "program stack segment base (highest address in stack)",
                  &ld_stack_base, ld_stack_base, "  0x%08p");
    stat_reg_uint(sdb, "ld_stack_size",
                  "program initial stack size",
                  &ld_stack_size, ld_stack_size, NULL);
#if 0 /* FIXME: broken... */
    stat_reg_addr(sdb, "ld_stack_min",
          "program stack segment lowest address",
          &ld_stack_min, ld_stack_min, "  0x%08p");
#endif
    stat_reg_addr(sdb, "ld_prog_entry",
                  "program entry point (initial PC)",
                  &ld_prog_entry, ld_prog_entry, "  0x%08p");
    stat_reg_addr(sdb, "ld_environ_base",
                  "program environment base address address",
                  &ld_environ_base, ld_environ_base, "  0x%08p");
    stat_reg_int(sdb, "ld_target_big_endian",
                 "target executable endian-ness, non-zero if big endian",
                 &ld_target_big_endian, ld_target_big_endian, NULL);
}




void ld_load_prog_oneline(char *fname,        /* program to load */
                          int argc, char **argv,    /* simulated program cmd line args */
                          char **envp,        /* simulated program environment */
                          struct regs_t *regs,    /* registers to initialize for load */
                          struct mem_t *mem,        /* memory space to load prog into */
                          int zero_bss_segs)        /* zero uninit data segment? */
{
    FILE *fobj = fopen(argv[0], "r");
    if (!fobj) {
        fputs("Not read a file", stderr);
    }
    fseek(fobj, 0, SEEK_END);
    long lsize = ftell(fobj);
    rewind(fobj);
    char *p = (char *) calloc(lsize, sizeof(char));
    if (p == NULL) {
        fputs("error", stderr);
        exit(1);
    }
    fread(p, lsize, 1, fobj);
    fprintf(stderr, "%ld\n", lsize);

    char *mem_0 = (char *) calloc(5000, sizeof(char));
    memset(mem_0, 0, 5000);
    mem_bcopy(mem_access, mem, Write, 0,
              mem_0, 5000);
    mem_bcopy(mem_access, mem, Write, 1000,
              p, lsize);

    regs->regs_PC = 1000;
}

/* load program text and initialized data into simulated virtual memory
   space and initialize program segment range variables */
void
ld_load_prog(char *fname,        /* program to load */
             int argc, char **argv,    /* simulated program cmd line args */
             char **envp,        /* simulated program environment */
             struct regs_t *regs,    /* registers to initialize for load */
             struct mem_t *mem,        /* memory space to load prog into */
             int zero_bss_segs)        /* zero uninit data segment? */
{
    int i;
    md_addr_t temp;
    md_addr_t sp, data_break = 0, null_ptr = 0, argv_addr, envp_addr;

    if (eio_valid(fname)) {
        if (argc != 1) {
            fprintf(stderr, "error: EIO file has arguments\n");
            exit(1);
        }

        fprintf(stderr, "sim: loading EIO file: %s\n", fname);

        sim_eio_fname = mystrdup(fname);

        /* open the EIO file stream */
        sim_eio_fd = eio_open(fname);

        /* load initial state checkpoint */
        if (eio_read_chkpt(regs, mem, sim_eio_fd) != -1)
            fatal("bad initial checkpoint in EIO file");

        /* load checkpoint? */
        if (sim_chkpt_fname != NULL) {
            counter_t restore_icnt;

            FILE *chkpt_fd;

            fprintf(stderr, "sim: loading checkpoint file: %s\n",
                    sim_chkpt_fname);

            if (!eio_valid(sim_chkpt_fname))
                fatal("file `%s' does not appear to be a checkpoint file",
                      sim_chkpt_fname);

            /* open the checkpoint file */
            chkpt_fd = eio_open(sim_chkpt_fname);

            /* load the state image */
            restore_icnt = eio_read_chkpt(regs, mem, chkpt_fd);

            /* fast forward the baseline EIO trace to checkpoint location */
            myfprintf(stderr, "sim: fast forwarding to instruction %n\n",
                      restore_icnt);
            eio_fast_forward(sim_eio_fd, restore_icnt);
        }

        /* computed state... */
        ld_environ_base = regs->regs_R[MD_REG_SP];
        ld_prog_entry = regs->regs_PC;

        /* fini... */
        return;
    }
#ifdef MD_CROSS_ENDIAN
    else
      {
        fatal("SimpleScalar/RISCV not supports binary execution on\n"
          "cross-endian hosts");
      }
#endif /* MD_CROSS_ENDIAN */


    if (sim_chkpt_fname != NULL)
        fatal("checkpoints only supported while EIO tracing");

    FILE *fobj;
    long floc;
    struct elf_filehdr fhdr;
    struct elf_scnhdr shdr;
    struct elf_phdr phdr;


    ld_prog_fname = argv[0];

    fobj = fopen(argv[0], "r");
    if (!fobj)
        fatal("cannot open executable `%s'", argv[0]);
    if (fread(&fhdr, sizeof(struct elf_filehdr), 1, fobj) < 1)
        fatal("cannot read header frome executable `%s'", argv[0]);

    /* check if it is a valid ELF file*/
    if (!(fhdr.e_ident[EI_MAG0] == 0x7f &&
          fhdr.e_ident[EI_MAG1] == 'E' &&
          fhdr.e_ident[EI_MAG2] == 'L' &&
          fhdr.e_ident[EI_MAG3] == 'F'))
        fatal("bad magic number in executable `%s' (not an executable)", argv[0]);

    if (fhdr.e_type != ET_EXEC)
        fatal("object file `%s' is not executable", argv[0]);
    //if (fhdr.e_machine != EM_RISCV)
    //  fatal("executable file `%s' is not for the RISCV architecture this file machine number is %d", argv[0], fhdr.e_machine);

    ld_prog_entry = fhdr.e_entry;
    debug("entry!!!%x\n", ld_prog_entry);

    debug("number of sections in execution = %d\n", fhdr.e_shnum);
    debug("offset to section header table = %d\n", fhdr.e_shoff);
    /* perform sanity checks on segment ranges */

    for (i = 0; i < fhdr.e_phnum; i++) {
        char *p;
        if (fseek(fobj, fhdr.e_phoff + (i * fhdr.e_phentsize), SEEK_SET) == -1)
            fatal("ArchLib: error parsing %s", argv[0]);
        size_t len = fread(&phdr, 1, sizeof(phdr), fobj);
        if (len != sizeof(phdr))
            fatal("ArchLib: error parsing %s", argv[0]);
        switch (phdr.p_type) {
            case PT_NULL:
                break;
            case PT_NOTE:
                break;
            case PT_SHLIB:
                break;
            case PT_LOOS:
                break;
            case PT_HIOS:
                break;
            case PT_LOPROC:
                break;
            case PT_HIPROC:
                break;
            case PT_LOAD:
            case PT_DYNAMIC:
            case PT_INTERP:
            case PT_PHDR:
                // seek to the beginning of the segment
                if (fseek(fobj, phdr.p_offset, SEEK_SET) == -1)
                    fatal("ArchLib: error seeking to program seg %d from %s",
                          i, argv[0]);
                // read the segment
                p = (char *) calloc(phdr.p_filesz, sizeof(char));
                if (!p)
                    fatal("ArchLib: out of virtual memory");
                if (fread(p, phdr.p_filesz, 1, fobj) < 1)
                    fatal("ArchLib: error reading program segment %d from %s",
                          i, argv[0]);
                mem_bcopy(mem_access, mem, Write, MD_SWAPQ(phdr.p_vaddr), p, MD_SWAPQ(phdr.p_filesz));
                free(p);
                break;
            default:
                fprintf(stderr, "ArchLib: ignored ELF64 program segment UNKNOWN: %d \n", phdr.p_type);
                //panic("%d\n", phdr.p_type);
                break;
        }
    }
    /* seek to the beginning of the first section header, the file header comes
        first, followed by the optional header (this is the aouthdr), the size
        of the aouthdr is given in Fdhr.f_opthdr */
    fseek(fobj, fhdr.e_shoff + (fhdr.e_shstrndx * fhdr.e_shentsize), SEEK_SET);
    if (fread(&shdr, sizeof(struct elf_scnhdr), 1, fobj) < 1)
        fatal("error reading string section header from `%s'", argv[0]);


    byte_t *strtab, *buffer;
    /* allocated space for string table */
    strtab = (byte_t *) calloc(shdr.sh_size, sizeof(char));
    if (!strtab)
        fatal("out of virtual memory");
    /* read the string table */
    if (fseek(fobj, shdr.sh_offset, SEEK_SET) < 0)
        fatal("cannot seek to string table section");
    if (fread(strtab, shdr.sh_size, 1, fobj) < 0)
        fatal("cannot read string table section");

    debug("size of string table = %d", shdr.sh_size);
    debug("type of section = %d", shdr.sh_type);
    debug("offset of string table in file = %d", shdr.sh_offset);

    debug("processing %d sections in `%s'...", fhdr.e_shnum, argv[0]);

    for (i = 0; i < fhdr.e_shnum; i++) {
        buffer = NULL;
        if (fseek(fobj, fhdr.e_shoff + (i * fhdr.e_shentsize), SEEK_SET) < 0)
            fatal("could not reset location in executable");
        if (fread(&shdr, sizeof(struct elf_scnhdr), 1, fobj) < 1)
            fatal("could not read section %d from executable", i);

        /* make sure the file is static */
        if (shdr.sh_type == SHT_DYNAMIC || shdr.sh_type == SHT_DYNSYM)
            fatal("file is dynamically linked, compile with `-static'");

        debug("processing section `%s'...", &strtab[shdr.sh_name]);
        debug("  section flags = 0x%08x", shdr.sh_flags);
        debug("  section type = %d", shdr.sh_type);
        debug("  section address = 0x%08x", shdr.sh_addr);
        debug("  section offset = %d", shdr.sh_offset);
        debug("  section size = %d", shdr.sh_size);
        debug("  section link = %d", shdr.sh_link);
        debug("  section extra info = %d", shdr.sh_info);
        debug("  section entry size = %d", shdr.sh_entsize);
        if (shdr.sh_addr != 0
            && shdr.sh_size != 0
            && (shdr.sh_type == SHT_PROGBITS || shdr.sh_type == SHT_NOBITS || shdr.sh_type == SHT_FINI_ARRAY ||
                shdr.sh_type == SHT_INIT_ARRAY)) {
            /* if the ELF format designates an address for the section then
               load it into memory */
            debug("  processing section `%s'...", &strtab[shdr.sh_name]);

            if ((shdr.sh_flags & SHF_ALLOC) != 0 && shdr.sh_type != SHT_NOBITS) {
                debug("  loading section `%s'...", &strtab[shdr.sh_name]);
                /* go to the offset in file where section is located */
                if (fseek(fobj, shdr.sh_offset, SEEK_SET) < 0)
                    fatal("cannot file pointer");

                /* allocate memory for the section contents */
                buffer = (byte_t *) calloc(shdr.sh_size, sizeof(byte_t));
                if (!buffer)
                    fatal("out of virtual memory");

                /* read section into buffer */
                if (fread(buffer, shdr.sh_size, 1, fobj) < 1)
                    fatal("could not read all the contents of section");

                /* copy the section contents into simulator target memory */
                mem_bcopy(mem_access, mem, Write, shdr.sh_addr,
                          buffer, shdr.sh_size);
            }

            /* update program space limits */
            if ((shdr.sh_flags & SHF_EXECINSTR) != 0) {
                debug("updating text size...");
                if (shdr.sh_addr < ld_text_base)
                    ld_text_base = shdr.sh_addr;
                if ((shdr.sh_addr + shdr.sh_size) > ld_text_bound)
                    ld_text_bound = shdr.sh_addr + shdr.sh_size;
            } else {
                debug("updating data size...");
                if (shdr.sh_addr < ld_data_base)
                    ld_data_base = shdr.sh_addr;
                if ((shdr.sh_addr + shdr.sh_size) > ld_data_bound)
                    ld_data_bound = shdr.sh_addr + shdr.sh_size;
            }
            ld_text_size = ld_text_bound - ld_text_base;
            ld_data_size = ld_data_bound - ld_data_base;

            /* release buffer storage */
            if (buffer != NULL)
                free(buffer);
        }
    }
    /* release string table storage */
    free(strtab);
    if (!ld_text_base || !ld_text_size)
        fatal("executable is missing a `.text' section");
    if (!ld_data_base || !ld_data_size)
        fatal("executable is missing a `.data' section");
    if (!ld_prog_entry)
        fatal("program entry point not specified");
    /* determine byte/words swapping required to execute on this host */
    sim_swap_bytes = (endian_host_byte_order() != endian_target_byte_order());
    if (sim_swap_bytes) {
        fatal("binary endian does not match host endian");
    }
    sim_swap_words = (endian_host_word_order() != endian_target_word_order());
    if (sim_swap_words) {
        fatal("binary endian does not match host endian");
    }

    ld_stack_base = 0xc0000000;
    sp = ld_stack_base - MD_MAX_ENVIRON;
    ld_stack_size = ld_stack_base - sp;
    ld_environ_base = sp;

    /* write [argc] to stack */
    temp = MD_SWAPQ(argc);
    mem_access(mem, Write, sp, &temp, sizeof(qword_t));
    sp += sizeof(qword_t);

    /* skip past argv array and NULL */
    argv_addr = sp;
    sp = sp + (argc + 1) * sizeof(md_addr_t);

    /* save space for envp array and NULL */
    envp_addr = sp;
    for (i = 0; envp[i]; i++)
        sp += sizeof(md_addr_t);
    sp += sizeof(md_addr_t);

    /* fill in the argv pointer array and data */
    for (i = 0; i < argc; i++) {
        /* write the argv pointer array entry */
        temp = MD_SWAPQ(sp);
        mem_access(mem, Write, argv_addr + i * sizeof(md_addr_t),
                   &temp, sizeof(md_addr_t));
        /* and the data */
        mem_strcpy(mem_access, mem, Write, sp, argv[i]);
        sp += strlen(argv[i]) + 1;
    }
    /* terminate argv array with a NULL */
    mem_access(mem, Write, argv_addr + i * sizeof(md_addr_t),
               &null_ptr, sizeof(md_addr_t));

    /* write envp pointer array and data to stack */
    for (i = 0; envp[i]; i++) {
        /* write the envp pointer array entry */
        temp = MD_SWAPQ(sp);
        mem_access(mem, Write, envp_addr + i * sizeof(md_addr_t),
                   &temp, sizeof(md_addr_t));
        /* and the data */
        mem_strcpy(mem_access, mem, Write, sp, envp[i]);
        sp += strlen(envp[i]) + 1;
    }
    /* terminate the envp array with a NULL */
    mem_access(mem, Write, envp_addr + i * sizeof(md_addr_t),
               &null_ptr, sizeof(md_addr_t));

    /* did we tromp off the stop of the stack? */
    if (sp > ld_stack_base) {
        /* we did, indicate to the user that MD_MAX_ENVIRON must be increased,
       alternatively, you can use a smaller environment, or fewer
       command line arguments */
        fatal("environment overflow, increase MD_MAX_ENVIRON in riscv.h");
    }

    /* initialize the bottom of heap to top of data segment */
    ld_brk_point = ROUND_UP(ld_data_base + ld_data_size, MD_PAGE_SIZE);

    /* set initial minimum stack pointer value to initial stack value */
    ld_stack_min = regs->regs_R[MD_REG_SP];

    /* set up initial register state */
    regs->regs_R[MD_REG_SP] = ld_environ_base;
    regs->regs_PC = ld_prog_entry;

    debug("ld_text_base: 0x%08x  ld_text_size: 0x%08x",
          ld_text_base, ld_text_size);
    debug("ld_data_base: 0x%08x  ld_data_size: 0x%08x",
          ld_data_base, ld_data_size);
    debug("ld_stack_base: 0x%08x  ld_stack_size: 0x%08x",
          ld_stack_base, ld_stack_size);
    debug("ld_prog_entry: 0x%08x", ld_prog_entry);
}
