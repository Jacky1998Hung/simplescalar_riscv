#include <stdio.h>
#include <stdlib.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "eval.h"
#include "regs.h"
#include <stdbool.h>

/* preferred nop instruction definition */
md_inst_t MD_NOP_INST = 0x00000013; /* addi r0, r0, 0 */
md_c_inst_t MD_C_NOP_INST = 0x0001; /* c.addi r0, r0, 0 */

/* opcode mask -> enum md_opcodem, used by decoder (MD_OP_ENUM()) */
enum md_opcode md_mask2op[MD_MAX_MASK + 1];
unsigned int md_opoffset[OP_MAX];

/* enum md_opcode -> mask for decoding next level */
unsigned int md_opmask[OP_MAX] = {
        0, /* NA */
#define DEFINST(OP, MSK, NAME, OPFORM, RES, FLAGS, O1, O2, O3, I1, I2, I3, I4) 0,
#define DEFLINK(OP, MSK, NAME, SHIFT, MASK, SECOND_SHIFT) MASK,
#define CONNECT(OP)

#include "machine.def"
};

/* enum md_opcode -> shift for decoding next level */
unsigned int md_opshift[OP_MAX] = {
        0, /* NA */
#define DEFINST(OP, MSK, NAME, OPFORM, RES, FLAGS, O1, O2, O3, I1, I2, I3, I4) 0,
#define DEFLINK(OP, MSK, NAME, SHIFT, MASK, SECOND_SHIFT) SHIFT,
#define CONNECT(OP)

#include "machine.def"
};

/* enum md_opcode -> description string */
char *md_op2name[OP_MAX] = {
        NULL, /* NA */
#define DEFINST(OP, MSK, NAME, OPFORM, RES, FLAGS, O1, O2, O3, I1, I2, I3, I4) NAME,
#define DEFLINK(OP, MSK, NAME, MASK, SHIFT, SECOND_SHIFT) NAME,
#define CONNECT(OP)

#include "machine.def"
};

/* enum md_opcode -> opcode operand format, used by disassembler */
char *md_op2format[OP_MAX] = {
        NULL, /* NA */
#define DEFINST(OP, MSK, NAME, OPFORM, RES, FLAGS, O1, O2, O3, I1, I2, I3, I4) OPFORM,
#define DEFLINK(OP, MSK, NAME, MASK, SHIFT, SECOND_SHIFT) NULL,
#define CONNECT(OP)

#include "machine.def"
};

/* enum md_opcode -> enum md_fu_class, used by performance simulators */
enum md_fu_class md_op2fu[OP_MAX] = {
        FUClamd_NA, /* NA */
#define DEFINST(OP, MSK, NAME, OPFORM, RES, FLAGS, O1, O2, O3, I1, I2, I3, I4) RES,
#define DEFLINK(OP, MSK, NAME, MASK, SHIFT, SECOND_SHIFT) FUClamd_NA,
#define CONNECT(OP)

#include "machine.def"
};

/* enum md_fu_class -> description string */
char *md_fu2name[NUM_FU_CLASSES] = {
        NULL, /* NA */
        "fu-int-ALU",
        "fu-int-multiply",
        "fu-int-divide",
        "fu-FP-add/sub",
        "fu-FP-comparison",
        "fu-FP-conversion",
        "fu-FP-multiply",
        "fu-FP-divide",
        "fu-FP-sqrt",
        "rd-port",
        "wr-port"
};

/* enum md_opcode -> opcode flags, used by simulators */
unsigned int md_op2flags[OP_MAX] = {
        NA, /* NA */
#define DEFINST(OP, MSK, NAME, OPFORM, RES, FLAGS, O1, O2, O3, I1, I2, I3, I4) FLAGS,
#define DEFLINK(OP, MSK, NAME, MASK, SHIFT, SECOND_SHIFT) NA,
#define CONNECT(OP)

#include "machine.def"
};

/* lwl/lwr/swl/swr masks */
word_t md_lr_masks[] = {
#ifdef BYTES_BIG_ENDIAN
        0x00000000,
        0x000000ff,
        0x0000ffff,
        0x00ffffff,
        0xffffffff,
#else
        0xffffffff,
        0x00ffffff,
        0x0000ffff,
        0x000000ff,
        0x00000000,
#endif
};


char *md_amode_str[md_amode_NUM] =
        {
                "(const)",            /* immediate addressing mode */
                "(gp + const)",       /* global data access through global pointer */
                "(sp + const)",       /* stack access through stack pointer */
                "(fp + const)",       /* stack access through frame pointer */
                "(reg + const)",      /* (reg + const) addressing */
                "(reg + reg)"         /* (reg + reg) addressing */
        };
struct md_reg_names_t md_reg_names[] =
        {
                {"x0",    rt_gpr, 0},
                {"x1",    rt_gpr, 1},
                {"x2",    rt_gpr, 2},
                {"x3",    rt_gpr, 3},
                {"x4",    rt_gpr, 4},
                {"x5",    rt_gpr, 5},
                {"x6",    rt_gpr, 6},
                {"x7",    rt_gpr, 7},
                {"x8",    rt_gpr, 8},
                {"x9",    rt_gpr, 9},
                {"x10",   rt_gpr, 10},
                {"x11",   rt_gpr, 11},
                {"x12",   rt_gpr, 12},
                {"x13",   rt_gpr, 13},
                {"x14",   rt_gpr, 14},
                {"x15",   rt_gpr, 15},
                {"x16",   rt_gpr, 16},
                {"x17",   rt_gpr, 17},
                {"x18",   rt_gpr, 18},
                {"x19",   rt_gpr, 19},
                {"x20",   rt_gpr, 20},
                {"x21",   rt_gpr, 21},
                {"x22",   rt_gpr, 22},
                {"x23",   rt_gpr, 23},
                {"x24",   rt_gpr, 24},
                {"x25",   rt_gpr, 25},
                {"x26",   rt_gpr, 26},
                {"x27",   rt_gpr, 27},
                {"x28",   rt_gpr, 28},
                {"x29",   rt_gpr, 29},
                {"x30",   rt_gpr, 30},
                {"x31",   rt_gpr, 31},

                {"$zero", rt_gpr, 0},
                {"$ra",   rt_gpr, 1},
                {"$sp",   rt_gpr, 2},
                {"$gp",   rt_gpr, 3},
                {"$tp",   rt_gpr, 4},
                {"$t0",   rt_gpr, 5},
                {"$t1",   rt_gpr, 6},
                {"$t2",   rt_gpr, 7},
                {"$s0",   rt_gpr, 8},
                // {"$fp",   rt_gpr, 8},
                {"$s1",   rt_gpr, 9},
                {"$a0",   rt_gpr, 10},
                {"$a1",   rt_gpr, 11},
                {"$a2",   rt_gpr, 12},
                {"$a3",   rt_gpr, 13},
                {"$a4",   rt_gpr, 14},
                {"$a5",   rt_gpr, 15},
                {"$a6",   rt_gpr, 16},
                {"$a7",   rt_gpr, 17},
                {"$s2",   rt_gpr, 18},
                {"$s3",   rt_gpr, 19},
                {"$s4",   rt_gpr, 20},
                {"$s5",   rt_gpr, 21},
                {"$s6",   rt_gpr, 22},
                {"$s7",   rt_gpr, 23},
                {"$s8",   rt_gpr, 24},
                {"$s9",   rt_gpr, 25},
                {"$s10",  rt_gpr, 26},
                {"$s11",  rt_gpr, 27},
                {"$t3",   rt_gpr, 28},
                {"$t4",   rt_gpr, 29},
                {"$t5",   rt_gpr, 30},
                {"$t6",   rt_gpr, 31},


                /* floating point register file - single precision */
                {"$f0",   rt_fpr, 0},
                {"$f1",   rt_fpr, 1},
                {"$f2",   rt_fpr, 2},
                {"$f3",   rt_fpr, 3},
                {"$f4",   rt_fpr, 4},
                {"$f5",   rt_fpr, 5},
                {"$f6",   rt_fpr, 6},
                {"$f7",   rt_fpr, 7},
                {"$f8",   rt_fpr, 8},
                {"$f9",   rt_fpr, 9},
                {"$f10",  rt_fpr, 10},
                {"$f11",  rt_fpr, 11},
                {"$f12",  rt_fpr, 12},
                {"$f13",  rt_fpr, 13},
                {"$f14",  rt_fpr, 14},
                {"$f15",  rt_fpr, 15},
                {"$f16",  rt_fpr, 16},
                {"$f17",  rt_fpr, 17},
                {"$f18",  rt_fpr, 18},
                {"$f19",  rt_fpr, 19},
                {"$f20",  rt_fpr, 20},
                {"$f21",  rt_fpr, 21},
                {"$f22",  rt_fpr, 22},
                {"$f23",  rt_fpr, 23},
                {"$f24",  rt_fpr, 24},
                {"$f25",  rt_fpr, 25},
                {"$f26",  rt_fpr, 26},
                {"$f27",  rt_fpr, 27},
                {"$f28",  rt_fpr, 28},
                {"$f29",  rt_fpr, 29},
                {"$f30",  rt_fpr, 30},
                {"$f31",  rt_fpr, 31},

                {"$ft0",   rt_fpr, 0},
                {"$ft1",   rt_fpr, 1},
                {"$ft2",   rt_fpr, 2},
                {"$ft3",   rt_fpr, 3},
                {"$ft4",   rt_fpr, 4},
                {"$ft5",   rt_fpr, 5},
                {"$ft6",   rt_fpr, 6},
                {"$ft7",   rt_fpr, 7},
                {"$fs0",   rt_fpr, 8},
                {"$fs1",   rt_fpr, 9},
                {"$fa0",  rt_fpr, 10},
                {"$fa1",  rt_fpr, 11},
                {"$fa2",  rt_fpr, 12},
                {"$fa3",  rt_fpr, 13},
                {"$fa4",  rt_fpr, 14},
                {"$fa5",  rt_fpr, 15},
                {"$fa6",  rt_fpr, 16},
                {"$fa7",  rt_fpr, 17},
                {"$fs2",  rt_fpr, 18},
                {"$fs3",  rt_fpr, 19},
                {"$fs4",  rt_fpr, 20},
                {"$fs5",  rt_fpr, 21},
                {"$fs6",  rt_fpr, 22},
                {"$fs7",  rt_fpr, 23},
                {"$fs8",  rt_fpr, 24},
                {"$fs9",  rt_fpr, 25},
                {"$fs10",  rt_fpr, 26},
                {"$fs11",  rt_fpr, 27},
                {"$ft8",  rt_fpr, 28},
                {"$ft9",  rt_fpr, 29},
                {"$ft10",  rt_fpr, 30},
                {"$ft11",  rt_fpr, 31},
                
                /* program counters */
                {"$pc",   rt_PC,  0},
                {"$npc",  rt_NPC, 0},

                /* sentinel */
                {NULL,    rt_gpr, 0}
        };

/* returns a register name string */
char *
md_reg_name(enum md_reg_type rt, int reg) {
    int i;
    
    for (i = 0; md_reg_names[i].str != NULL; i++) {
        if (md_reg_names[i].file == rt && md_reg_names[i].reg == reg) {
            int len = strlen(md_reg_names[i].str) + strlen(md_reg_names[i+32].str);
            char* result = (char*)malloc(len);
            strcpy(result, md_reg_names[i].str);
            strcat(result, "|");
            strcat(result, md_reg_names[i+32].str);
            return result;
        }
    }
    /* no found... */
    return NULL;
}

char *                                          /* err str, NULL for no err */
md_reg_obj(struct regs_t *regs,                 /* registers to access */
           int is_write,                        /* access type */
           enum md_reg_type rt,                 /* reg bank to probe */
           int reg,                             /* register number */
           struct eval_value_t *val)            /* input, output */
{

// TODO if you have float register control...
    switch (rt) {
        case rt_gpr:
            if (reg < 0 || reg >= MD_NUM_IREGS)
                return "register number out of range";

            if (!is_write) {
                val->type = et_uint;
                val->value.as_uint = regs->regs_R[reg];
            } else
                regs->regs_R[reg] = eval_as_uint(*val);
            break;
        case rt_fpr:
            if (reg < 0 || reg >= MD_NUM_FREGS)
                return "register number out of range";
            if (!is_write) {
                val->type = et_float;
                val->value.as_float = regs->regs_F.f[reg];
            } else
                regs->regs_F.f[reg] = eval_as_float(*val);
            break;
        case rt_PC:
            if (!is_write) {
                val->type = et_addr;
                val->value.as_addr = regs->regs_PC;
            } else
                regs->regs_PC = eval_as_addr(*val);
            break;

        case rt_NPC:
            if (!is_write) {
                val->type = et_addr;
                val->value.as_addr = regs->regs_NPC;
            } else
                regs->regs_NPC = eval_as_addr(*val);
            break;

        default:
            panic("bogus register bank");
    }
    return NULL;
}


/* print integer REG(S) to STREAM */
void
md_print_ireg(md_gpr_t regs, int reg, FILE *stream) {
    fprintf(stream, "%5s: %12lld/0x%16llx",
            md_reg_name(rt_gpr, reg), regs[reg], regs[reg]);
}

void
md_print_iregs(md_gpr_t regs, FILE *stream) {
    int i;

    for (i = 0; i < MD_NUM_IREGS; i += 2) {
        md_print_ireg(regs, i, stream);
        fprintf(stream, "  ");
        md_print_ireg(regs, i + 1, stream);
        fprintf(stream, "\n");
    }
}

void
md_print_fpreg(md_fpr_t regs, int reg, FILE *stream) {
    myfprintf(stream, "%4s: %16ld / 0x%016llx / %f / %f",
              md_reg_name(rt_fpr, reg), regs.q[reg], regs.q[reg], regs.f[reg], regs.d[reg]);
}

void
md_print_fpregs(md_fpr_t regs, FILE *stream) {
    int i;
    for (i = 0; i < MD_NUM_FREGS; i += 2) {
        md_print_fpreg(regs, i, stream);
        fprintf(stream, "\n");
        md_print_fpreg(regs, i + 1, stream);
        fprintf(stream, "\n");
    }
}

void
md_print_creg(md_ctrl_t regs, int reg, FILE *stream) {
    /* index is only used to iterate over these registers, hence no enums...
    switch (reg)
      {
      case 0:
        myfprintf(stream, "FPCR: 0x%012lx", regs.fpcr);
        break;

      case 1:
        myfprintf(stream, "UNIQ: 0x%012lx", regs.uniq);
        break;

      default:
        panic("bogus control register index");
      }*/
}


void
md_print_cregs(md_ctrl_t regs, FILE *stream) {
    /*md_print_creg(regs, 0, stream);
    fprintf(stream, "  ");
    md_print_creg(regs, 1, stream);
    fprintf(stream, "\n");*/
}


/* xor checksum registers */
word_t
md_xor_regs(struct regs_t *regs) {
    int i;
    word_t checksum = 0;

    for (i = 0; i < MD_NUM_IREGS; i++)
        checksum ^= regs->regs_R[i];

    //for (i=0; i < MD_NUM_FREGS; i++)
    //  checksum ^= regs->regs_F.l[i];

    //checksum ^= regs->regs_C.hi;
    //checksum ^= regs->regs_C.lo;
    //checksum ^= regs->regs_C.fcc;
    checksum ^= regs->regs_PC;
    checksum ^= regs->regs_NPC;

    return checksum;
}

void print_op_table(char* table[], int just_added) {

    #define TABLE_NUM MD_MAX_MASK+1
    #define COL_NUM 30
    #define MAX_ROW TABLE_NUM/COL_NUM

    for(int i=0 ; i<=MAX_ROW ; i++) {
        if(table[i] != NULL) {
            if(i == just_added)
                printf(">>>");
            printf("%d ; 0x%x : %s\n", i, i, table[i]);
        }
    }
}

/* intialize the inst decoder, this function builds the ISA decode tables */
void
md_init_decoder(void) {
    unsigned long max_offset = 0;
    unsigned long offset = 0;

#define DEFINST(OP, MSK, NAME, OPFORM, RES, FLAGS, O1, O2, O3, I1, I2, I3, I4)            \
    if ((MSK)+offset >= MD_MAX_MASK) fatal("mask value is too small, index==%d %d %s",(MSK) ,+ offset, NAME);           \
    if (md_mask2op[(MSK)+offset]) fatal("doubly defined opcode, index==%d %s", (MSK) + offset, NAME);            \
    md_mask2op[(MSK)+offset]=(OP); max_offset=MAX(max_offset,(MSK) + offset); \

#define DEFLINK(OP, MSK, NAME, MASK, SHIFT, SECOND_SHIFT)            \
    if (md_mask2op[(MSK + offset + SECOND_SHIFT)]) fatal("doubly defined mask value");            \
    md_mask2op[(MSK)+offset]=(OP); max_offset=MAX(max_offset, (MSK)+offset);   \

#define CONNECT(OP)                                                     \
    offset = max_offset+1; md_opoffset[OP] = offset; \

#include "machine.def"

    if (max_offset >= MD_MAX_MASK)
        panic("MASK_MAX is too small, index==%d", max_offset);
}

void
md_print_insn_konata(md_inst_t inst,           /* instruction to disassemble */
              md_addr_t pc,             /* addr of inst, used for PC-rels */
              FILE *stream)
{
	md_print_insn(inst, pc, stream);
}


void
md_print_insn(md_inst_t inst,           /* instruction to disassemble */
              md_addr_t pc,             /* addr of inst, used for PC-rels */
              FILE *stream)             /* output stream */
{
    enum md_opcode op;

    /* use stderr as default output stream */
    if (!stream)
        stream = stderr;

    /* decode the instruction, assumes predecoded text segment */
    MD_SET_OPCODE(op, inst);

    /* disassemble the instruction */
    if (op == OP_NA || op >= OP_MAX) {
        /* bogus instruction */
        fprintf(stream, "<invalid inst: 0x%08x>", inst);
    } else if(op == MD_NOP_INST || op == MD_C_NOP_INST) {
        fprintf(stream, "<NOP inst>");
    }
    else{
        char *s;

        fprintf(stream, "%-10s", MD_OP_NAME(op));

        s = MD_OP_FORMAT(op);
        while (*s) {
            switch (*s) {
                case 'd':
                    fprintf(stream, "x%d", RD);
                    break;
                case 's':
                    fprintf(stream, "x%d", RS1);
                    break;
                case 'S':
                    fprintf(stream, "x%d", RS2);
                    break;
                case 'X':
                    fprintf(stream, "x%d", RS3);
                    break;
                case 'b':
                    fprintf(stream, "%d", BIMM);
                    break;
                case 'i':
                    fprintf(stream, "%d", IMM);
                    break;
                    /*case 'j':
                      fprintf(stream, "0x%x", (pc + 4 + (IMM << 2)));
                      break;*/
                case 'o':
                case 'j':
                    fprintf(stream, "%lld", SEXT(JIMM));
                    break;
                case 'H':
                    fprintf(stream, "%d", SHAMT);
                    break;
                case 'u':
                    fprintf(stream, "%u", UIMM);
                    break;
                case 'U':
                    fprintf(stream, "0x%llx", SEXT(UIMM));
                    break;
                    /* case 'J':
                       fprintf(stream, "0x%x", ((pc & 036000000000) | (TARG << 2)));
                       break;
                     case 'B':
                       fprintf(stream, "0x%x", BCODE);
                       break;*/
                default:
                    fputc(*s, stream);
            }
            s++;
        }
    }

}

char *
md_print_insn_rb(md_inst_t inst,
		md_addr_t pc)
{
    enum md_opcode op;
    char *result;         // Pointer to hold the final instruction string
    size_t buffer_size = 1024;
    char buffer[buffer_size];

    MD_SET_OPCODE(op, inst);
    

    result = malloc(buffer_size * sizeof(char));
    if (result == NULL) {
	perror("malloc failed");
	return NULL;
    }
    
    if (op == OP_NA || op >= OP_MAX) {
	//fprintf(stream, "<invalid inst: 0x%08x>", inst);
	snprintf(buffer, sizeof(buffer), "<invalid inst: 0x%08x>", inst);
	strcpy(result, buffer);
    }
    else if(op == MD_NOP_INST || op == MD_C_NOP_INST) {
	//fprintf(stream, "<NOP inst>");
	snprintf(buffer, sizeof(buffer), "<NOP inst>");
	strcpy(result, buffer);
    }
    else{
        char *s;
	snprintf(buffer, sizeof(buffer), "%-10s", MD_OP_NAME(op));
	strcpy(result, buffer);

	s = MD_OP_FORMAT(op);
	while (*s) {
	    switch (*s) {
	        case 'd':
	            snprintf(buffer, sizeof(buffer), "x%d", RD);
		    strcat(result, buffer);
		    break;
		case 's':
	            snprintf(buffer, sizeof(buffer), "x%d", RS1);
		    strcat(result, buffer);
		    break;
		case 'S':
	            snprintf(buffer, sizeof(buffer), "x%d", RS2);
		    strcat(result, buffer);
		    break;
	        case 'X':
	            snprintf(buffer, sizeof(buffer), "x%d", RS3);
		    strcat(result, buffer);
		    break;
		case 'b':
	            snprintf(buffer, sizeof(buffer), "%d", BIMM);
		    strcat(result, buffer);
		    break;
                case 'i':
	            snprintf(buffer, sizeof(buffer), "%d", IMM);
		    strcat(result, buffer);
		    break;
		case 'o':
		case 'j':
	            snprintf(buffer, sizeof(buffer), "%lld", SEXT(JIMM));
		    strcat(result, buffer);
		    break;
		case 'H':
	            snprintf(buffer, sizeof(buffer), "%d", SHAMT);
		    strcat(result, buffer);
		    break;
	    	case 'u':
	            snprintf(buffer, sizeof(buffer), "%u", UIMM);
		    strcat(result, buffer);
		    break;
	    	case 'U':
	            snprintf(buffer, sizeof(buffer), "0x%llx", SEXT(JIMM));
		    strcat(result, buffer);
		    break;
		default:
		    snprintf(buffer, sizeof(buffer), "%c", *s);
		    strcat(result, buffer);
	    }
	    s++;
	}
    }
    return result;
}
