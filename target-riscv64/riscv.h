#ifndef RISCV_H
#define RISCV_H

#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <float.h>
#include "host.h"
#include "misc.h"
#include "config.h"
#include "endian.h"

#define TARGET_RISCV

#ifndef HOST_HAS_QWORD
#define HOST_HAS_QWORD
#endif
#define ARITH_OVFL(RESULT, OP1, OP2) ((RESULT) < (OP1) || (RESULT) < (OP2))

/* TODO check for alpha.h
 probe cross-endian

#if defined(BYTES_BIG_ENDIAN)
#define MD_CROSS_ENDIAN
#endif

*/

/* not applicable/available, usable in most definition contexts */
#define NA       0



/*
 * target-dependent type definitions
 */
#define MD_QWORD_ADDRS
//#undef MD_QWORD_ADDRS

/* address type definition */
typedef qword_t md_addr_t;
#define XLEN 64

typedef union {
    sword_t sw;
    word_t w;
    float f;
} w2f_t;
typedef union {
    sqword_t sq;
    qword_t q;
    double d;
} q2d_t;

#include "floating_point_handler.def"

/* value */
#define UINT64_MIN 0
#define UINT64_MAX 0xffffffffffffffff
#define UINT32_MIN 0
#define UINT32_MAX 0xffffffff
#define INT64_MIN (-(1LL << 63))
#define INT64_MAX ((1ULL << 63) - 1)
#define INT32_MIN (-(1LL << 31))
#define INT32_MAX ((1ULL << 31) - 1)


/*
 * target-dependent memory module configuration
 */

/* physical memory page size (must be a power-of-two) */
#define MD_PAGE_SIZE        (4096)
#define MD_LOG_PAGE_SIZE     12

/* 
 * target-dependent instruction faults
 */
/* TODO falut*/
enum md_fault_type {
    md_fault_none = 0,        /* no fault */
    md_fault_access,      /* storage access fault */
    md_fault_alignment,       /* storage alignment fault */
    md_fault_overflow,        /* signed arithmetic overflow fault */
    md_fault_div0,        /* division by zero fault */
    md_fault_break,       /* BREAK instruction fault */
    md_fault_unimpl,      /* unimplemented instruction fault */
    md_fault_internal     /* internal S/W fault */
};


/*
 * target-dependent register file definitions, used by regs.[hc]
 */

/* number of integer registers */
#define MD_NUM_IREGS        32

/* number of floating point registers */
#define MD_NUM_FREGS        32

/* number of control registers */
#define MD_NUM_CREGS        3

/* total number of registers, excluding PC and NPC */
#define MD_TOTAL_REGS                           \
      (/*int*/32 + /*fp*/32 + /*misc*/3 + /*tmp*/1 + /*mem*/1 + /*ctrl*/1)

/* general purpose (integer) register file entry type */
typedef qword_t md_gpr_t[MD_NUM_IREGS];

typedef union {
    word_t w[MD_NUM_FREGS * 2]; /* integer word view */
    qword_t q[MD_NUM_FREGS]; /* integer word view */
    sword_t sw[MD_NUM_FREGS * 2]; /* integer word view */
    sqword_t sq[MD_NUM_FREGS]; /* integer word view */
    sfloat_t f[MD_NUM_FREGS * 2]; /* single-precision floating point view */
    dfloat_t d[MD_NUM_FREGS]; /* double-precision floating point view */
} md_fpr_t;

/* control register file contents */
typedef struct {
    qword_t fcsr; /* |reserved | frm | fflags| */
    qword_t csr[4096];
} md_ctrl_t;

/* well known registers */
enum md_reg_names {
    MD_REG_ZERO = 0, /* zero register */
    MD_REG_RA = 1, /* return address reg */
    MD_REG_SP = 2, /* stack pointer */
    MD_REG_GP = 3, /* global pointer */
    MD_REG_TP = 4, /* thread pointer */
    MD_REG_T0 = 5, /* temporaries 0 */
    MD_REG_T1 = 6, /* temporaries 1 */
    MD_REG_T2 = 7, /* temporaries 2 */
    MD_REG_FP = 8, /* frame pointer */
    MD_REG_S0 = 8, /* saved register 0 */
    MD_REG_S1 = 9, /* saved register 1 */
    MD_REG_A0 = 10, /* arguments 0 */
    MD_REG_A1 = 11, /* arguments 1 */
    MD_REG_A2 = 12, /* arguments 2 */
    MD_REG_A3 = 13, /* arguments 3 */
    MD_REG_A4 = 14, /* arguments 4 */
    MD_REG_A5 = 15, /* arguments 5 */
    MD_REG_A6 = 16, /* arguments 6 */
    MD_REG_A7 = 17, /* arguments 7 */
    MD_REG_S2 = 18, /* saved register 2 */
    MD_REG_S3 = 19, /* saved register 3 */
    MD_REG_S4 = 20, /* saved register 4 */
    MD_REG_S5 = 21, /* saved register 5 */
    MD_REG_S6 = 22, /* saved register 6 */
    MD_REG_S7 = 23, /* saved register 7 */
    MD_REG_S8 = 24, /* saved register 8 */
    MD_REG_S9 = 25, /* saved register 9 */
    MD_REG_S10 = 26, /* saved register 10 */
    MD_REG_S11 = 27, /* saved register 11 */
    MD_REG_T3 = 28, /* temporaries 3 */
    MD_REG_T4 = 29, /* temporaries 4 */
    MD_REG_T5 = 30, /* temporaries 5 */
    MD_REG_T6 = 31, /* temporaries 6 */
};

/* Load-reserved/store-condition recorder */
struct lrsc_t{
  md_addr_t target_addr;
  qword_t val;
};

/*
 * target-dependent instruction format definition
 */
#define MD_MAX_MASK    2048
/* instruction formats */
typedef sword_t md_inst_t;        /* simplescalar unsigned immediate fields */
typedef shalf_t md_c_inst_t;

/* preferred nop & illgel instruction definition */
extern md_inst_t MD_NOP_INST;
extern md_c_inst_t MD_C_NOP_INST;
extern md_c_inst_t MD_C_ILLGEL_INST;

/* target swap support */
#ifdef MD_CROSS_ENDIAN

#define MD_SWAPH(X)     SWAP_HALF(X)
#define MD_SWAPW(X)     SWAP_WORD(X)
#define MD_SWAPQ(X)     SWAP_QWORD(X)
#define MD_SWAPI(X)     ((X).a = SWAP_WORD((X).a),      \
                         (X).b = SWAP_WORD((X).b))

#else /* !MD_CROSS_ENDIAN */

#define MD_SWAPH(X)     (X)
#define MD_SWAPW(X)     (X)
#define MD_SWAPQ(X)     (X)
#define MD_SWAPD(X)     (X)
#define MD_SWAPI(X)     (X)

#endif

/* fetch an instruction */
#define MD_FETCH_INST(INST, MEM, PC)                    \
  { \
    (INST) = MEM_READ_HALF((MEM), (PC)); \
    if(!INST_IsCompressed(INST))  {\
        md_addr_t instHigh = MEM_READ_HALF((MEM), (PC + 2));  \
        (MEM)->ptab_accesses--;	\
        instHigh <<= 16;  \
        (INST) |= instHigh; \
        \	
        if(((MD_PAGE_SIZE - 1) - (PC & (MD_PAGE_SIZE - 1))) <= sizeof(md_c_inst_t)) {\
            (MEM)->ptab_accesses++;	\
            mem_cross_page_refs++;  \
        } \
    } \
  }

/*
 * target-dependent loader module configuration
 */

/* virtual memory segment limits */

/*TODO　check memory MODEL*/
#define MD_TEXT_BASE        0x00400000
#define MD_DATA_BASE        0x10000000
#define MD_STACK_BASE       0x0 //0x7fffc000

/* maximum size of argc+argv+envp environment */
#define MD_MAX_ENVIRON      16384


/*
 * machine.def specific definitions
 */

// #define NX(fcsr)    (fcsr & 0x1)
// #define UF(fcsr)    ((fcsr & 0x2)>>1)
// #define OF(fcsr)    ((fcsr & 0x4)>>2)
// #define DZ(fcsr)    ((fcsr & 0x8)>>3)
// #define NV(fcsr)    ((fcsr & 0x10)>>4)
// #define FRM(fcsr)    ((fcsr & 0b11100000)>>5)


/* returns the opcode field value of SimpleScalar instruction INST */
#define INST_IsCompressed(INST) (((INST) & 0x03) != 0x03)
#define MD_TOP_OP(INST)    ((INST) & 0x7f)
#define MD_C_TOP_OP(INST)   ((INST) & 0x03)
#define MD_OPFIELD(INST)    ((INST) & 0x7f)
// #define MD_SET_OPCODE(OP, INST)            \
//     { OP = md_mask2op[MD_TOP_OP(INST)];    \
//       while (md_opmask[OP])                \
//         OP = md_mask2op[((INST >> md_opshift[OP]) & md_opmask[OP]) \
//         + md_opoffset[OP]]; \
//     }
#define MD_SET_OPCODE(OP, INST)            \
    { \
      if(INST_IsCompressed(INST)) { \
        OP = md_mask2op[MD_C_TOP_OP(INST)];    \
         /**/ \
         /*fprintf(stderr, "OP(%x) = md_mask2op[MD_C_TOP_OP(INST(0x%x)(0x%x;%d))]\n", \
           md_mask2op[MD_C_TOP_OP(INST)], INST, MD_C_TOP_OP(INST), MD_C_TOP_OP(INST));*/ \
         /**/  \
      } \
      else  {\
        OP = md_mask2op[MD_TOP_OP(INST)]; \
        /**/ \
        /*fprintf(stderr, "OP(%x) = md_mask2op[MD_TOP_OP(INST(0x%x)(0x%x;%d))]\n",\
        md_mask2op[MD_TOP_OP(INST)], INST, MD_TOP_OP(INST), MD_TOP_OP(INST));*/ \
        /**/ \
      } \
      /**/ \
      /*debug("MD_SET_OPCODE: OP enum : %d\n", OP);*/ \
      /**/ \
      while (md_opmask[OP])    {            \
      /**/ \
        /*debug("md_opmask[%d]: %x\n", OP, md_opmask[OP]);  \
        debug( \
        "OP(%x;%d) = md_mask2op[((INST(%x) >> md_opshift[OP(%x;%d)](%x;%d)) & md_opmask[OP(%x;%d)](%x)) + md_opoffset[OP(%x;%d)](%x;%d)](%x;%d);\n",\
        md_mask2op[((INST >> md_opshift[OP]) & md_opmask[OP]) + md_opoffset[OP]],\
        md_mask2op[((INST >> md_opshift[OP]) & md_opmask[OP]) + md_opoffset[OP]],\
        INST, OP, OP, md_opshift[OP], md_opshift[OP], OP, OP, md_opmask[op], OP, OP, md_opoffset[OP], md_opoffset[OP],\
        ((INST >> md_opshift[OP]) & md_opmask[OP]) + md_opoffset[OP],\
        ((INST >> md_opshift[OP]) & md_opmask[OP]) + md_opoffset[OP]);*/ \
      /**/ \
        OP = md_mask2op[((INST >> md_opshift[OP]) & md_opmask[OP]) + md_opoffset[OP]]; \
      } \
    }

extern unsigned int md_opoffset[];
extern unsigned int md_opmask[];
extern unsigned int md_opshift[];
enum md_opcode {
    OP_NA = 0, /* NA */
#define DEFINST(OP, MSK, NAME, OPFORM, RES, FLAGS, O1, O2, O3, I1, I2, I3, I4) OP,
#define DEFLINK(OP, MSK, NAME, MASK, SHIFT, SECOND_SHIFT) OP,
#define CONNECT(OP)

#include "machine.def"

    OP_MAX
};

/* inst -> enum md_opcode mapping, use this macro to decode insts */
#define MD_OP_ENUM(MSK)     (md_mask2op[MSK])
extern enum md_opcode md_mask2op[];

/* enum md_opcode -> description string */
#define MD_OP_NAME(OP)      (md_op2name[OP])
extern char *md_op2name[];

/* enum md_opcode -> opcode operand format, used by disassembler */
#define MD_OP_FORMAT(OP)    (md_op2format[OP])
extern char *md_op2format[];

/* function unit classes, update md_fu2name if you update this definition */
enum md_fu_class {
    FUClamd_NA = 0,   /* inst does not use a functional unit */
    IntALU,       /* integer ALU */
    IntMULT,      /* integer multiplier */
    IntDIV,       /* integer divider */
    FloatADD,     /* floating point adder/subtractor */
    FloatCMP,     /* floating point comparator */
    FloatCVT,     /* floating point<->integer converter */
    FloatMULT,        /* floating point multiplier */
    FloatDIV,     /* floating point divider */
    FloatSQRT,        /* floating point square root */
    FloatMULT_ACC,        /* floating point multiplier */
    RdPort,       /* memory read port */
    WrPort,       /* memory write port */
    AMOPort,       /* AMO */
    NUM_FU_CLASSES    /* total functional unit classes */
};

/* enum md_opcode -> enum md_fu_class, used by performance simulators */
#define MD_OP_FUCLASS(OP)   (md_op2fu[OP])
extern enum md_fu_class md_op2fu[];


/* enum md_fu_class -> description string */
#define MD_FU_NAME(FU)      (md_fu2name[FU])
extern char *md_fu2name[];

/* instruction flags */
#define F_ICOMP     0x00000001  /* integer computation */
#define F_FCOMP     0x00000002  /* FP computation */
#define F_CTRL      0x00000004  /* control inst */
#define F_UNCOND    0x00000008  /*   unconditional change */
#define F_COND      0x00000010  /*   conditional change */
#define F_MEM       0x00000020  /* memory access inst */
#define F_LOAD      0x00000040  /*   load inst */
#define F_STORE     0x00000080  /*   store inst */
#define F_DISP      0x00000100  /*   displaced (R+C) addr mode */
#define F_RR        0x00000200  /*   R+R addr mode */
#define F_DIRECT    0x00000400  /*   direct addressing mode */
#define F_TRAP      0x00000800  /* traping inst */
#define F_LONGLAT   0x00001000  /* long latency inst (for sched) */
#define F_DIRJMP    0x00002000  /* direct jump */
#define F_INDIRJMP  0x00004000  /* indirect jump */
#define F_CALL      0x00008000  /* function call */
#define F_FPCOND    0x00010000  /* FP conditional branch */
#define F_IMM       0x00020000  /* instruction has immediate operand */
#define F_ATOMIC    0x00040000  /* instruction has atomic */
#define F_SYNC      0x00080000  /* instruction has synchronize attribute */
#define F_AMO       0x00100000  /* instruction is amo type of atomic attribute */
#define F_IFLUSH    0x00200000  /* fflush instruction cache */

/* enum md_opcode -> opcode flags, used by simulators */
#define MD_OP_FLAGS(OP)     (md_op2flags[OP])
extern unsigned int md_op2flags[];

/* integer register specifiers */
#define ZERO    (0)
#define RS1      ((inst >> 15) & 0x1f)     /* reg source #1 */
#define RS2      ((inst >> 20) & 0x1f)     /* reg source #2 */
#define RS3      ((inst >> 27) & 0x1f)     /* reg source #3 */
#define RD       ((inst >> 7) & 0x1f)      /* reg dest */
#define RL       ((inst >> 25) & 0x01)      /* rl */
#define AQ       ((inst >> 26) & 0x01)      /* aq */

/* definition for ext.C */
#define EXTRA_OFFSET 32

#define RD_C_x8          (((inst >> 2) & 0x07) + 0x08)
#define RD_C_x32         ((inst >> 7) & 0x1f)
#define RS1_C_x8         (((inst >> 7) & 0x07) + 0x08)
#define RS1_C_x32        RD_C_x32
#define RS1_RD_C_x8      RS1_C_x8
#define RS1_RD_C_x32     RS1_C_x32
#define RS2_C_x8         RD_C_x8
#define RS2_C_x32        ((inst >> 2) & 0x1f)

#define FUNCT3_C_SHIFT 13
#define FUNCT2_C_SHIFT 10
#define FUNCT1_C_SHIFT 12
#define FUNCT2_2ND_C_SHIFT 5
#define FUNCT3_C_11_7_SHIFT 7
#define RS1_C_SHIFT 9
#define RS2_C_SHIFT 2
#define RS1_C_2ND_SHIFT 7

#define FUNCT3_C_MASK 0x07
#define FUNCT2_C_MASK 0x03
#define FUNCT1_C_MASK 0x01
#define FUNCT2_2ND_C_MASK 0x03
#define FUNCT3_C_11_7_MASK 0x1f
#define RS1_C_MASK 0x1f
#define RS2_C_MASK 0x1f
#define RS1_C_2ND_MASK 0x1f

#define SHAMT_C IMM_C_5_4_0

#define IMM_C_R ((inst) & 0x7c)
#define IMM_C_L ((inst) & 0x1f80)
#define IMM_C_5 ((((inst) >> 12) & 0x01) << 5)
#define WIMM_C_R ((((IMM_C_R) & 0x70) >> 2) | (((IMM_C_R) & 0x0c) << 4) | (IMM_C_5))
#define WIMM_C_L ((((IMM_C_L) & 0x1e00) >> 7) | (((IMM_C_L) & 0x0180) >> 1))
#define DIMM_C_R ((((IMM_C_R) & 0x60) >> 2) | (((IMM_C_R) & 0x1c) << 4) | (IMM_C_5))
#define DIMM_C_L ((((IMM_C_L) & 0x1c00) >> 7) | (((IMM_C_L) & 0x0380) >> 1))
#define WIMM_C_BRLS (((inst & 0x40) >> 4) | ((inst & 0x20) << 1) | (((inst >> 10) & 0x07) <<3 ))
#define DIMM_C_BRLS (((inst & 0x60) << 1) | (((inst >> 10) & 0x07) << 3))
#define JICI (inst & 0b1111111111100)
#define JIMM_C (((JICI&0x4)<<3) | ((JICI&0x38)>>2) | ((JICI&0x40)<<1) | \
               ((JICI&0x80)>>1) | ((JICI&0x100)<<2) | ((JICI&0x600)>>1) | \
               (((JICI >> 11) & 0x01) <<4) | (((JICI >> 12 ) & 0x01 ) << 11))
#define IMM_C_12_10 (inst & 0x1c00)
#define BIMM_C (((IMM_C_R & 0x04) << 3) | ((IMM_C_R & 0x18) >> 2) | ((IMM_C_R & 0x60) << 1) | \
               ((IMM_C_12_10 & 0x0c00) >> 7) | ((IMM_C_12_10 & 0x1000) >> 4))
#define IMM_C_5_4_0 ((IMM_C_R >> 2) | (IMM_C_5))
#define UIMM_C ((IMM_C_R << 10) | (IMM_C_5 << 12))
#define IMM_C_4SPN (((inst & 0x20) >> 2) | ((inst & 0x40) >> 4) | ((inst & 0x780) >> 1) | ((inst & 0x1800) >> 7))
#define IMM_C_16SP (((IMM_C_R & 0x04) << 3) | ((IMM_C_R & 0x18) << 4) | \
                   ((IMM_C_R & 0x20) << 1)  | ((IMM_C_R & 0x40) >> 2) | \
                   (IMM_C_5 << 4))
/* end of def. for ext.C */

#define FUNCT3    ((inst >> 12) & 0x03)
#define FUNCT7    ((inst >> 25) & 0x7f)
#define FUNCT5    ((inst >> 27) & 0x1f)
#define IMM       (inst >> 20)
#define SIMM_5_11  (inst >> 25)
#define SIMM_0_4   RD
#define SIMM      ((IMM & 0xffffffe0)|(SIMM_0_4))
#define BIMM_1_4  (SIMM_0_4 >> 1)
#define BIMM_11   (SIMM_0_4 & 0x01)
#define BIMM_5_10 (SIMM_5_11 & 0x3f)
#define BIMM_12   ((inst) >> 31)
//#define BIMM	  ((BIMM_12 << 12) | (BIMM_11 << 11) | (BIMM_5_10 << 5) | (BIMM_1_4 << 1))
#define BIMM      ((SIMM & 0xfffff7fe) | ((inst >> 7 & 0x01) << 11))
#define UIMM      ( (inst >> 12 ) << 12)
#define UIMM4      (RS1)
#define JIMM_12_19 (UIMM & 0xff)
#define JIMM_11   ((UIMM >> 8) & 0x01)
#define JIMM_1_10 ((UIMM >> 9) & 0x01ff)
#define JIMM_20   ((inst) >> 31)
#define JIMM     (((IMM) & 0xfff007fe) | ((UIMM) & 0x000ff000) | (((inst >> 20) & 0x01) << 11))


#define FUNCT2_SHIFT 25
#define FUNCT3_SHIFT 12
#define FUNCT7_SHIFT 25
#define FUNCT2_MASK 0x03
#define FUNCT3_MASK 0x07
#define FUNCT5_MASK 0x1f
#define FUNCT7_MASK 0x7f

#define CSR       ((unsigned int)inst >> 20)
#define SHAMT     RS2
#define BYTE_MASK 0xff
#define HALF_MASK 0xffff
#define WORD_MASK 0xffffffff
#define QWORD_MASK 0xffffffffffffffffLL
#define HIGH_MASK 0xffffffff00000000LL
#define LOW_MASK  0x00000000ffffffffLL
#define SHAMT_I	((inst >> 20) & 0x3f)
#define SHAMT_IW ((inst >> 20) & 0x1f)
#define SHIFT_MASK 0b111111
#define SHIFTW_MASK 0b11111

#define MUL_MASK 0xffffffffffffffffLL

/* Used for Floating point */
#define RM ((inst >> 12) & 0x3)

/* Used for FENCE*/
#define PRED      ((IMM >> 4) & 0b1111)
#define SUCC      (IMM & 0b1111)

typedef sqword_t ext_signed_t;
typedef qword_t ext_unsigned_t;
typedef sword_t ext_signed32_t;
typedef word_t ext_unsigned32_t;


/* sign-extend operands */

#define EXT(X) \
  ((qword_t)(X) & 0x00000000ffffffffLL)
#define EXT32(X) \
  ((word_t)(X) & 0x0000ffffL)
#define SEXT(X) \
  (((X) & 0x80000000LL) ? ((sqword_t)(X)|(0xffffffff00000000LL)) : (sqword_t)(X))
#define SEXT32(X) \
  (((X) & 0x8000L) ? ((sword_t)(X)|(0xffff0000L)) : (sword_t)(X))
#define SEXT_TO(X, ORIGINAL_BITS, TARGET_BITS) \
  ((((X) & (1ULL << (ORIGINAL_BITS - 1))) ? ((sqword_t)(X) | (~((1ULL << ORIGINAL_BITS) - 1))) : (sqword_t)(X)) \
  & ((1ULL << TARGET_BITS) - 1))
#define UEXT_TO(X, TARGET_BITS) \
  ((qword_t)(X) & ((1ULL << TARGET_BITS) - 1))


#ifndef SET_TPC
#define SET_TPC(PC) (void)0
#endif /* SET_TPC */


extern word_t md_lr_masks[];

#define SS_SYS_exit        1
// FIXME 
#define MD_EXIT_SYSCALL(REGS)   ((REGS)->regs_R[2] == SS_SYS_exit)

#define SS_SYS_write        4
#define MD_OUTPUT_SYSCALL(REGS)                    \
    ((REGS)->regs_R[2] == SS_SYS_write)            \
    && ((REGS)->regs_R[4] == /*stdout*/1 || (REGS)->regs_R[4] ==/*stderr*/ 2)
#define MD_STREAM_FILENO(REGS)        ((REGS)->regs_R[4])

#define MD_IS_CALL(OP)            \
    ((MD_OP_FLAGS(OP) & (F_CTRL|F_CALL)) == (F_CTRL|F_CALL))
#define MD_IS_RETURN(OP)        (((OP) == JAL) | ((OP) == C_JR)) // until you write C-extension
#define MD_IS_INDIR(OP)            ((OP) == JAL | ((OP) == C_JR))

enum md_amode_type {
    md_amode_imm,         /* immediate addressing mode */
    md_amode_gp,          /* global data access through global pointer */
    md_amode_sp,          /* stack access through stack pointer */
    md_amode_fp,          /* stack access through frame pointer */
    md_amode_disp,        /* (reg + const) addressing */
    md_amode_rr,          /* (reg + reg) addressing */
    md_amode_NUM
};
extern char *md_amode_str[md_amode_NUM];

/* addressing mode pre-probe FSM, must see all instructions */
#define MD_AMODE_PREPROBE(OP, FSM)                                      \
  { if ((OP) == LUI) (FSM) = (RD); }

/* compute addressing mode, only for loads/stores */
#define MD_AMODE_PROBE(AM, OP, FSM)                                     \
  {                                                                     \
    if (MD_OP_FLAGS(OP) & F_DISP)                                       \
      {                                                                 \
        if ((RS1) == (FSM))                                              \
          (AM) = md_amode_imm;                                          \
        else if ((RS1) == MD_REG_GP)                                     \
          (AM) = md_amode_gp;                                           \
        else if ((RS1) == MD_REG_SP)                                     \
          (AM) = md_amode_sp;                                           \
        else if ((RS1) == MD_REG_FP) /* && bind_to_seg(addr) == seg_stack */\
          (AM) = md_amode_fp;                                           \
        else                                                            \
          (AM) = md_amode_disp;                                         \
      }                                                                 \
    else if (MD_OP_FLAGS(OP) & F_RR)                                    \
      (AM) = md_amode_rr;                                               \
    else                                                                \
      panic("cannot decode addressing mode");                           \
  }

/* addressing mode pre-probe FSM, after all loads and stores */
#define MD_AMODE_POSTPROBE(FSM)                                         \
  { (FSM) = MD_REG_ZERO; }



/*
 * configure the DLite! debugger
 */


// register bank specifier 
enum md_reg_type {
    rt_gpr,               /* general purpose register */
    rt_lpr,               /* integer-precision floating pointer register */
    rt_fpr,               /* single-precision floating pointer register */
    rt_dpr,               /* double-precision floating pointer register */
    rt_ctrl,              /* control register */
    rt_PC,                /* program counter */
    rt_NPC,               /* next program counter */
    rt_NUM
};

struct md_reg_names_t {
    char *str; //register name
    enum md_reg_type file;
    int reg;
};
/* symbolic register names, parser is case-insensitive */
extern struct md_reg_names_t md_reg_names[];

/* returns a register name string */
char *md_reg_name(enum md_reg_type rt, int reg);

/* default register accessor object */
struct eval_value_t;
struct regs_t;

char *
md_reg_obj(struct regs_t *regs, int is_write, enum md_reg_type rt, int reg, struct eval_value_t *val);

/* print integer REG(S) to STREAM */
void md_print_ireg(md_gpr_t regs, int reg, FILE *stream);

void md_print_iregs(md_gpr_t regs, FILE *stream);

/* print floating point REG(S) to STREAM */
void md_print_fpreg(md_fpr_t regs, int reg, FILE *stream);

void md_print_fpregs(md_fpr_t regs, FILE *stream);

/* print control REG(S) to STREAM */
void md_print_creg(md_ctrl_t regs, int reg, FILE *stream);

void md_print_cregs(md_ctrl_t regs, FILE *stream);

/* xor checksum registers */
word_t md_xor_regs(struct regs_t *regs);

void md_print_stack(md_gpr_t regs, FILE *stream);
/*
 * configure sim-outorder specifics
 */

/* primitive operation used to compute addresses within pipeline */
#define MD_AGEN_OP              ADD
/* NOP operation when injected into the pipeline */
#define MD_NOP_OP               OP_NA
#define MD_C_NOP_OP             OP_NA


/* non-zero for a valid address, used to determine if speculative accesses
   should access the DL1 data cache */
#define MD_VALID_ADDR(ADDR)                                             \
  (((ADDR) >= ld_text_base && (ADDR) < (ld_text_base + ld_text_size))   \
   || ((ADDR) >= ld_data_base && (ADDR) < ld_brk_point)                 \
   || ((ADDR) >= (ld_stack_base - 16*1024*1024) && (ADDR) < ld_stack_base))
/*
 * configure branch predictors
 */

/* shift used to ignore branch address least significant bits, usually
   log2(sizeof(md_inst_t)) */
#define MD_BR_SHIFT             2       /* log2(4) */

/*
 * target-dependent routines
 */

/* intialize the inst decoder, this function builds the ISA decode tables */
void md_init_decoder(void);

void
md_print_insn_konata(md_inst_t inst,           /* instruction to disassemble */
              md_addr_t pc,             /* addr of inst, used for PC-rels */
              FILE *stream);            /* output stream */

/* disassemble a SimpleScalar instruction */
void
md_print_insn(md_inst_t inst,           /* instruction to disassemble */
              md_addr_t pc,             /* addr of inst, used for PC-rels */
              FILE *stream);            /* output stream */

char *
md_print_insn_rb(md_inst_t inst,           /* instruction to disassemble */
              md_addr_t pc);             /* addr of inst, used for PC-rels */

/*
   EIO package configuration/macros
*/

#define MD_EIO_FILE_FORMAT    EIO_PISA_FORMAT

#define MD_MISC_REGS_TO_EXO(REGS)        \
   exo_new(ec_list,                                                      \
         /*icnt*/exo_new(ec_integer, (exo_integer_t)sim_num_insn),     \
           /*PC*/exo_new(ec_address, (exo_integer_t)(REGS)->regs_PC),    \
           /*NPC*/exo_new(ec_address, (exo_integer_t)(REGS)->regs_NPC),  \
           /*HI exo_new(ec_integer, (exo_integer_t)(REGS)->regs_C.hi), */\
           /*LO exo_new(ec_integer, (exo_integer_t)(REGS)->regs_C.lo), */ \
           /*FCC exo_new(ec_integer, (exo_integer_t)(REGS)->regs_C.fcc),*/\
           NULL)
#define MD_IREG_TO_EXO(REGS, IDX)                                       \
  exo_new(ec_address, (exo_integer_t)(REGS)->regs_R[IDX])

#define MD_FREG_TO_EXO(REGS, IDX)                                       \
  exo_new(ec_address, (exo_integer_t)(REGS)->regs_F.sw[(IDX)<<1])

#define MD_EXO_TO_MISC_REGS(EXO, ICNT, REGS)                            \
  /* check EXO format for errors... */                                  \
  if (!exo                                                              \
      || exo->ec != ec_list                                             \
      || !exo->as_list.head                                             \
      || exo->as_list.head->ec != ec_integer                            \
      || !exo->as_list.head->next                                       \
      || exo->as_list.head->next->ec != ec_address                      \
      || !exo->as_list.head->next->next                                 \
      || exo->as_list.head->next->next->ec != ec_address                \
      || !exo->as_list.head->next->next->next                           \
      || exo->as_list.head->next->next->next->ec != ec_integer          \
      || !exo->as_list.head->next->next->next->next                     \
      || exo->as_list.head->next->next->next->next->ec != ec_integer    \
      || !exo->as_list.head->next->next->next->next->next               \
      || exo->as_list.head->next->next->next->next->next->ec != ec_integer\
      || exo->as_list.head->next->next->next->next->next->next != NULL) \
    fatal("could not read EIO misc regs");                              \
  (ICNT) = (counter_t)exo->as_list.head->as_integer.val;                \
  (REGS)->regs_PC = (md_addr_t)exo->as_list.head->next->as_address.val; \
  (REGS)->regs_NPC =                                                    \
    (md_addr_t)exo->as_list.head->next->next->as_address.val;           \
  /*(REGS)->regs_C.hi =                                                   \
    (word_t)exo->as_list.head->next->next->next->as_integer.val;        \
  (REGS)->regs_C.lo =                                                   \
    (word_t)exo->as_list.head->next->next->next->next->as_integer.val;  \
  (REGS)->regs_C.fcc =                                                  \
    (int)exo->as_list.head->next->next->next->next->next->as_integer.val; \
	*/
#define MD_EXO_TO_IREG(EXO, REGS, IDX)                                  \
  ((REGS)->regs_R[IDX] = (qword_t)(EXO)->as_integer.val)

#define MD_EXO_TO_FREG(EXO, REGS, IDX)                                  \
  ((REGS)->regs_F.sw[(IDX)<<1] = (word_t)(EXO)->as_integer.val)

#define MD_EXO_CMP_IREG(EXO, REGS, IDX)                                 \
  ((REGS)->regs_R[IDX] != (sword_t)(EXO)->as_integer.val)

#define MD_FIRST_IN_REG                 2
#define MD_LAST_IN_REG                  7

#define MD_FIRST_OUT_REG                2
#define MD_LAST_OUT_REG                 7

/* RISC-V HINT instruction detector */
/* opcode enum value. Modifying machine.def define order 
    may cause this encoding to malfunction. */
#define MD_HINT_INST(OP, O1, O2, O3); \
        switch(op) {  \
            case 1:     /* lui */ \
            case 2:     /* auipc */ \
            case 53:    /* addi */  \
            case 58:    /* andi */   \
            case 57:    /* ori */ \
            case 56:    /* xori */  \
            case 63:    /* add */ \
            case 71:    /* sub */ \
            case 70:    /* and */ \
            case 69:    /* or */  \
            case 67:    /* xor */ \
            case 64:    /* sll */ \
            case 68:    /* srl */ \
            case 72:    /* sra */ \
            case 73:    /* addw */  \
            case 74:    /* sllw */  \
            case 75:    /* srlw */  \
            case 76:    /* subw */  \
            case 77:    /* sraw */  \
            case 78:    /* fence */ \
            case 61:    /* srli */  \
            case 54:    /* slti */  \
            case 55:    /* sltiu */ \
            case 59:    /* slli */  \
            case 49:    /* slliw */ \
            case 51:    /* srliw */ \
            case 52:    /* sraiw */ \
            case 62:    /* srai */  \
            case 65:    /* slt */ \
            case 66:    /* sltu */  \
            case 209:   /* c.addi */  \
            case 211:   /* c.li */  \
            case 231:   /* c.lui */ \
            case 318:   /* c.add?? */ \
            case 261:   /* c.slli64 */  \
            case 290:   /* c.mv?? */  \
                if((op != MD_NOP_OP) && !spec_mode && ((out1 | out2 | out3) == 0)) {  \
                    static int hintCnt = 0; \
                    debug("HINT[%d], inst[%x], addr[%x], op[%d]\n", ++hintCnt, inst, regs.regs_PC, op);  \
                    fetch_head = (fetch_head + 1) & (ruu_ifq_size - 1); \
                    fetch_num--;  \
                    continue; \
                } \
            default:  \
                break;  \                   
        }

/*
	configure the EXO package
*/

typedef qword_t exo_integer_t;
typedef qword_t exo_address_t;
typedef double exo_float_t;
// FIXME you should know how to ....


/*
 * configure the stats package
 */

/* counter stats */
#ifdef HOST_HAS_QWORD
#define stat_reg_counter                stat_reg_sqword
#define sc_counter                      sc_sqword
#define for_counter                     for_sqword
#define stat_reg_addr                   stat_reg_qword
#else /* !HOST_HAS_QWORD */
#define stat_reg_counter                stat_reg_double
#define sc_counter                      sc_double
#define for_counter                     for_double
#define stat_reg_addr                   stat_reg_uint
#endif /* HOST_HAS_QWORD */
/* address stats */

// int op_log[1024];

#endif /*  RISCV_H */
