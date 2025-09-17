#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <setjmp.h>
#include <stdbool.h>

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

static struct regs_t result_regs;

/* simulated memory */
static struct mem_t *result_mem = NULL;
static struct mem_t *mem = NULL;
static char fastone_fname[128];
#define FENCE()


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

/* initialize the simulator */
void
sim_init(void) {
    /* allocate and initialize register file */
    regs_init(&regs);
    regs_init(&result_regs);

    /* allocate and initialize memory space */
    mem = mem_create("mem");
    result_mem = mem_create("result_mem");
    mem_init(mem);
    mem_init(result_mem);
}

/* load program into simulated state */
void
sim_load_prog(char *fname,        /* program to load */
              int argc, char **argv,    /* program arguments */
              char **envp)        /* program environment */
{
    /* load program text and data, set up environment, memory, and regs */
    strncpy(fastone_fname, fname, strlen(fname));
    load_prog_test_state();
    ld_load_prog_oneline(fname, argc, argv, envp, &regs, mem, TRUE);
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

#if defined(TARGET_RISCV)

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
#endif /* NO_INSN_COUNT */

#ifdef TARGET_ALPHA
#define ZERO_FP_REG()	regs.regs_F.d[MD_REG_ZERO] = 0.0
#else
#define ZERO_FP_REG()    /* nada... */
#endif


bool check_ans();

/* setting initial field of CSR */
void sim_machine_init(void) {
    regs.regs_C.csr[MISA] = 1;
}

/* track whether a control transfer inst has taken */
static bool IsCtrlInstTaken;
#define CtrlInstTaken(RES) { IsCtrlInstTaken = RES; }

/* start simulation, program loaded, processor precise state initialized */
void
sim_main(void) {

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

        /* keep an instruction count */
#ifndef NO_INSN_COUNT
        sim_num_insn++;
#endif /* !NO_INSN_COUNT */
        MD_FETCH_INST(inst, mem, regs.regs_PC);
        /* decode the instruction */
        MD_SET_OPCODE(op, inst);
        /* execute the instruction */
        switch (op) {
#define DEFINST(OP, MSK, NAME, OPFORM, RES, FLAGS, O1, O2, O3, I1, I2, I3, I4)    \
    case OP:                            \
      SYMCAT(OP,_IMPL);                        \
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
                debug("%010x", regs.regs_PC);
                if (!check_ans()) {
                    fprintf(stderr, "Test Failed \n");
                }
                panic("inst: 0x%08x \n", inst);
                panic("attempted to execute a bogus opcode 0x%x", op);
        }
        /* execute next instruction */
        regs.regs_PC = regs.regs_NPC;
        regs.regs_NPC += sizeof(md_inst_t);
    }
}

int md_check_memory(
        md_addr_t mem_addr,
        qword_t val,
        qword_t *now_val) {
    enum md_fault_type _fault;
    *now_val = READ_QWORD(mem_addr, _fault);
    return *now_val == val;
}
void write_mem(md_addr_t addr, qword_t val)
{
    // use zero regis for write qword to memory
    //SET_GPR(0, addr);
    //WRITE_QWORD(0, addr, 0);
}
void write_reg(char *regs_name, qword_t val)
{

    for (int i = 0; md_reg_names[i].str != NULL; i++) {
        if (strncmp(md_reg_names[i].str, regs_name, strlen(regs_name)) == 0) {
            switch (md_reg_names[i].file) {
                case rt_gpr:
                    SET_GPR(md_reg_names[i].reg, val);
                    return;
                case rt_PC:
                    SET_NPC(val);
                    return;
                case rt_dpr:
                    // TODO
                    panic("rt_dpr not implement");
                    return;
                case rt_fpr:
                    panic("rt_fpr not implement");
                    return;
                default:
                    break;
            }
        }
    }
    fprintf(stderr, "regs_name %s error\n", regs_name);
    return false;
}
int md_check_regs(
        char *regs_name,
        qword_t val,
        qword_t *now_val) {
    for (int i = 0; md_reg_names[i].str != NULL; i++) {
        if (strncmp(md_reg_names[i].str, regs_name, strlen(regs_name)) == 0) {
            switch (md_reg_names[i].file) {
                case rt_gpr:
                    *now_val = GPR(md_reg_names[i].reg);
                    break;
                case rt_PC:
                    *now_val = CPC;
                    break;
                case rt_dpr:
                    // TODO
                    *now_val = GPR(md_reg_names[i].reg);
                    break;
                case rt_fpr:
                    // TODO
                    *now_val = GPR(md_reg_names[i].reg);
                    break;
                default:
                    break;
            }
            return *now_val == val;
        }
    }
    fprintf(stderr, "regs_name %s error\n", regs_name);
    return false;
}

void load_prog_test_state()
{

    char file_name[128];
    size_t n = strlen(fastone_fname);
    strncpy(file_name, fastone_fname, n+1);
    strcat(file_name, ".stat");
    fprintf(stderr, "Test file: %s\n", file_name);

    char buf[128];
    FILE *stat_fobj = fopen(file_name, "r");
    if (stat_fobj == NULL) {
        debug("no stat file");
        return;
    }
    fscanf(stat_fobj, "%s", buf);
    if (strncmp("regs", buf, sizeof("regs")) == 0) {
        int regs_len = 0;
        fscanf(stat_fobj, "%d", &regs_len);
        char regs_name[10];
        qword_t _val = 0;
        for (int i = 0; i < regs_len; i++) {
            fscanf(stat_fobj, "%s", regs_name);
            fscanf(stat_fobj, "%lld", &_val);
            write_reg(regs_name, _val);
        }
    }
    fscanf(stat_fobj, "%s", buf);
    if (strncmp("mem", buf, sizeof("mem")) == 0) {
        int mem_len = 0;
        md_addr_t mem_addr = 0;
        qword_t _val = 0;
        fscanf(stat_fobj, "%d", &mem_len);
        for (int i = 0; i < mem_len; i++) {
            fscanf(stat_fobj, "%lld", &mem_addr);
            fscanf(stat_fobj, "%lld", &_val);
            write_mem(mem_addr, _val);
        }
    }

}
bool check_ans() {
    fprintf(stderr, "Test file: %s\n", fastone_fname);
    bool pass = true;
    char file_name[128];
    size_t n = strlen(fastone_fname);
    strncpy(file_name, fastone_fname, n+1);
    // TODO refactor here
    strcat(file_name, ".ans");
    fprintf(stderr, "Test file: %s\n", file_name);

    char buf[128];
    FILE *ans_fobj = fopen(file_name, "r");
    if (ans_fobj == NULL) {
        return false;
    }
    fscanf(ans_fobj, "%s", buf);
    if (strncmp("regs", buf, sizeof("regs")) == 0) {
        int regs_len = 0;
        fscanf(ans_fobj, "%d", &regs_len);
        char regs_name[10];
        qword_t _ans = 0;
        qword_t _now = 0;
        for (int i = 0; i < regs_len; i++) {
            fscanf(ans_fobj, "%s", regs_name);
            fscanf(ans_fobj, "%lld", &_ans);
            if (!md_check_regs(regs_name, _ans, &_now)) {
                fprintf(stderr, "Regs %s val error ans: %lld, now: %lld\n", regs_name, _ans, _now);
                pass = false;
            }
        }
    }
    fscanf(ans_fobj, "%s", buf);
    if (strncmp("mem", buf, sizeof("mem")) == 0) {
        int mem_len = 0;
        md_addr_t mem_addr = 0;
        qword_t _ans = 0;
        qword_t _now = 0;
        fscanf(ans_fobj, "%d", &mem_len);
        for (int i = 0; i < mem_len; i++) {
            fscanf(ans_fobj, "%lld", &mem_addr);
            fscanf(ans_fobj, "%lld", &_ans);
            if (!md_check_memory(mem_addr, _ans, &_now)) {
                fprintf(stderr, "Mem %lld val error ans: %lld, now: %lld\n", mem_addr, _ans, _now);
                pass = false;
            }
        }
    }
    fscanf(ans_fobj, "%s", buf);

    return pass;
}
