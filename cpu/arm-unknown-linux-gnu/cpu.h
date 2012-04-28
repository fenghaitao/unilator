/////////////////////////////////////////////////////////////////////////
// $Id: cpu.h,v 1.8 2003/11/08 07:24:21 fht Exp $
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2001  MandrakeSoft S.A.
//
//    MandrakeSoft S.A.
//    43, rue d'Aboukir
//    75002 Paris - France
//    http://www.linux-mandrake.com/
//    http://www.mandrakesoft.com/
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

/* hacking machine specific features 2003-07-03 */
#ifndef BX_CPU_H
#define BX_CPU_H 1

#ifndef ARM_H
#define ARM_H
/* copied from misc.h 2003-07-02*/

/* boolean value defs */
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* various useful macros */
#ifndef MAX
#define MAX(a, b)    (((a) < (b)) ? (b) : (a))
#endif
#ifndef MIN
#define MIN(a, b)    (((a) < (b)) ? (a) : (b))
#endif

#include "cpu/arm-unknown-linux-gnu/host.h"

/* not applicable/available, usable in most definition contexts */
#define NA		0

/*
 * target-dependent memory module configuration
 */

/* physical memory page size (must be a power-of-two) */
#define MD_PAGE_SIZE            4096
#define MD_LOG_PAGE_SIZE        12

/*
 * target-dependent instruction faults
 */

enum md_fault_type {
  md_fault_none = 0,            /* no fault */
  md_fault_access,              /* storage access fault */
  md_fault_alignment,           /* storage alignment fault */
  md_fault_overflow,            /* signed arithmetic overflow fault */
  md_fault_div0,                /* division by zero fault */
  md_fault_invalid,             /* invalid arithmetic operation */
                               /* added to allow SQRT{S,T} in FIX exts */
  md_fault_break,               /* BREAK instruction fault */
  md_fault_unimpl,              /* unimplemented instruction fault */
  md_fault_internal             /* internal S/W fault */
};

/* cpu modes */
enum md_cpu_modes{
  md_cpu_usr =0,
  md_cpu_fiq,
  md_cpu_irq,
  md_cpu_svc,
  md_cpu_abt,
  md_cpu_und
};

#define is_fiq_mode	(PSR&0x1f ==0x10001)

/* address type definition */
typedef word_t md_addr_t;

/*
 * target-dependent register file definitions, used by regs.[hc]
 */

/* number of integer registers */
#define MD_UCODE_IREGS		16

#define MD_NUM_IREGS		(/* arch */16 + /* Ucode */16)

/* number of floating point registers */
#define MD_NUM_FREGS		8

/* number of control registers */
#define MD_NUM_CREGS		3

/* total number of registers, excluding PC and NPC */
#define MD_TOTAL_REGS							\
  (/*int*/32 + /*fp*/8 + /*misc*/3 + /*tmp*/1 + /*mem*/1 + /*ctrl*/1)

#define UNBANKED_NUM_IREGS	8
	
/* general purpose (integer) register file entry type */
typedef word_t md_unbanked_t[UNBANKED_NUM_IREGS];
typedef word_t md_ucode_t[MD_UCODE_IREGS];
typedef word_t md_bank2_t[2];
typedef word_t md_bank6_t[6];
typedef word_t md_bank5_t[5];

/* general purpose (integer) register file entry type */
typedef word_t* md_gpr_p_t[MD_NUM_IREGS];

typedef struct{
  md_gpr_p_t	regs_R_p;
  md_unbanked_t	regs_R_unbanked;
  md_bank2_t	r8,r9,r10,r11,r12;
  md_bank6_t	r13,r14;
  md_addr_t 	r15;            /* program counter */
  md_ucode_t	ucodes_R;
}md_gpr_t;


/* floating point register file entry type */
typedef union {
  qword_t q[MD_NUM_FREGS];	/* integer qword view */
  dfloat_t d[MD_NUM_FREGS];	/* double-precision floating point view */
} md_fpr_t;

/* control register file contents */
typedef struct {
  word_t cpsr;			/* processor status register */
  word_t *spsr;
  md_bank5_t	spsr_modes;
  word_t fpsr;			/* floating point status register */
} md_ctrl_t;

typedef struct regs_t {
  md_gpr_t regs_R;              /* (signed) integer register file */
  md_fpr_t regs_F;              /* floating point register file */
  md_ctrl_t regs_C;             /* control register file */
  md_addr_t regs_PC;            /* program counter */
  md_addr_t regs_NPC;           /* next-cycle program counter */
}bx_regs_t;

/* well known registers */
enum md_reg_names {
  MD_REG_R0 = 0,
  MD_REG_V0 = 0,	/* return value reg */
  MD_REG_A0 = 0,	/* argument regs */
  MD_REG_R1 = 1,
  MD_REG_A1 = 1,
  MD_REG_R2 = 2,
  MD_REG_A2 = 2,
  MD_REG_R3 = 3,
  MD_REG_A3 = 3,
  MD_REG_R4 = 4,
  MD_REG_R5 = 5,
  MD_REG_R6 = 6,
  MD_REG_R7 = 7,
  MD_REG_R8 = 8,
  MD_REG_R9 = 9,
  MD_REG_R10 = 10,
  MD_REG_SL = 10,
  MD_REG_R11 = 11,
  MD_REG_FP = 11,       /* frame pointer */
  MD_REG_R12 = 12,
  MD_REG_IP = 12,
  MD_REG_R13 = 13,
  MD_REG_SP = 13,       /* stack pointer */
  MD_REG_R14 = 14,
  MD_REG_LR = 14,       /* link register */
  MD_REG_R15 = 15,
  MD_REG_PC = 15,       /* link register */
  MD_REG_TMP0 = 16,     /* temp registers - used by Ucode, 16 total */ 
  MD_REG_TMP1 = 17,
  MD_REG_TMP2 = 18,
  MD_REG_TMP3 = 19,

  SYS_REG_R0 = 0,
  SYS_REG_R1 = 1,
  SYS_REG_R2 = 2,
  SYS_REG_R3 = 3,
  SYS_REG_R4 = 4,
  SYS_REG_R5 = 5,
  SYS_REG_R6 = 6,
  SYS_REG_R7 = 7,
  SYS_REG_R8 = 8,
  SYS_REG_R9 = 9,
  SYS_REG_R10 = 10,
  SYS_REG_R11 = 11,
  SYS_REG_R12 = 12,
  SYS_REG_R13 = 13,
  SYS_REG_R14 = 14,
  SYS_REG_R15 = 15,
};

/*
 * machine.def specific definitions
 */

/* inst -> enum md_opcode mapping, use this macro to decode insts */
#define MD_TOP_OP(INST)		(((INST) >> 24) & 0x0f)
#define MD_SET_OPCODE(OP, INST)						              \
  { OP = BX_CPU_THIS_PTR  md_mask2op[MD_TOP_OP(INST)];				      \
    while (BX_CPU_C::md_opmask[OP])				      	      \
      OP= BX_CPU_THIS_PTR md_mask2op[((INST >> BX_CPU_C::md_opshift[OP])       \
	      & BX_CPU_C::md_opmask[OP]) + BX_CPU_THIS_PTR md_opoffset[OP]]; }

/* largest opcode field value (currently upper 8-bit are used for pre/post-
    incr/decr operation specifiers */
#define MD_MAX_MASK		2048
#define InAPrivilegedMode       (PSR&0x0f)
      
/* global opcode names, these are returned by the decoder (MD_OP_ENUM()) */
enum md_opcode {
  OP_NA = 0,	/* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,O3,I1,I2,I3,I4) OP,
#define DEFUOP(OP,NAME,OPFORM,RES,FLAGS,O1,O2,O3,I1,I2,I3,I4) OP,
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT) OP,
#define CONNECT(OP)
#include "cpu/arm-unknown-linux-gnu/cpu.def"
  OP_MAX	/* number of opcodes + NA */
};


/* function unit classes, update md_fu2name if you update this definition */
typedef enum {
    FUClamd_NA = 0,       /* inst does not use a functional unit */
    IntALU,               /* integer ALU */
    IntMULT,              /* integer multiplier */
    IntDIV,               /* integer divider */
    FloatADD,             /* floating point adder/subtractor */
    FloatCMP,             /* floating point comparator */
    FloatCVT,             /* floating point<->integer converter */
    FloatMULT,            /* floating point multiplier */
    FloatDIV,             /* floating point divider */
    FloatSQRT,            /* floating point square root */
    RdPort,               /* memory read port */
    WrPort,               /* memory write port */
    CoProc,
   NUM_FU_CLASSES        /* total functional unit classes */
}md_fu_class;


/* enum md_opcode -> description string */
#define MD_OP_NAME(OP)		(BX_CPU_C::md_op2name[OP])

/* enum md_opcode -> opcode operand format, used by disassembler */
#define MD_OP_FORMAT(OP)	(BX_CPU_C::md_op2format[OP])

/* enum md_fu_class -> description string */
#define MD_FU_NAME(FU)		(BX_CPU_C::md_fu2name[FU])

/* instruction formats */
typedef word_t md_inst_t;

/* hacking different features 2003-07-03 */
/* instruction flags */
#define F_ICOMP		0x00000001	/* integer computation */
#define F_FCOMP		0x00000002	/* FP computation */
#define F_CTRL		0x00000004	/* control inst */
#define F_UNCOND	0x00000008	/*   unconditional change */
#define F_COND		0x00000010	/*   conditional change */
#define F_MEM		0x00000020	/* memory access inst */
#define F_LOAD		0x00000040	/*   load inst */
#define F_STORE		0x00000080	/*   store inst */
#define F_DISP		0x00000100	/*   displaced (R+C) addr mode */
#define F_RR		0x00000200	/*   R+R addr mode */
#define F_DIRECT	0x00000400	/*   direct addressing mode */
#define F_TRAP		0x00000800	/* traping inst */
#define F_LONGLAT	0x00001000	/* long latency inst (for sched) */
#define F_DIRJMP	0x00002000	/* direct jump */
#define F_INDIRJMP	0x00004000	/* indirect jump */
#define F_CALL		0x00008000	/* function call */
#define F_FPCOND	0x00010000	/* FP conditional branch */
#define F_IMM		0x00020000	/* instruction has immediate operand */
#define F_CISC		0x00040000	/* CISC instruction */
#define F_AGEN		0x00080000	/* AGEN micro-instruction */

/* enum md_opcode -> opcode flags, used by simulators */
#define MD_OP_FLAGS(OP)		(BX_CPU_C::md_op2flags[OP])

/* SYS CONTROL REGISTER */
#define SYS_OPCODE2     ((inst >>  5) & 0x07)

/* integer register specifiers */

#define RN		((inst >> 16) & 0x0f)
#define CRN		((inst >> 16) & 0x0f)
#define URN		((inst >> 17) & 0x1f)	/* Ucode RN */
#define RD		((inst >> 12) & 0x0f)
#define CRD		((inst >> 12) & 0x0f)
#define URD		((inst >> 12) & 0x1f)	/* Ucode RD */
#define RS		((inst >> 8) & 0x0f)
#define RM		(inst & 0x0f)
#define CRM		(inst & 0x0f)

/* floating point register specifiers */
#define FN		((inst >> 16) & 0x07)
#define FD		((inst >> 12) & 0x07)
#define FM		(inst & 0x07)

/* register shift accessors */
#define SHIFT_BITS	((inst >> 4) & 0xff)
#define SHIFT_REG	((inst >> 4) & 0x01)
#define SHIFT_REG_PAD	((inst >> 7) & 0x01)
#define SHIFT_TYPE	((inst >> 5) & 0x03)
#define SHIFT_SHAMT	((inst >> 7) & 0x1f)

/* register shift types */
#define SHIFT_LSL	0x00
#define SHIFT_LSR	0x01
#define SHIFT_ASR	0x02
#define SHIFT_ROR	0x03

/* rotated immediate accessors */
#define ROTIMM		(inst & 0xff)
#define ROTAMT		((inst >> 8) & 0x0f)

/* after installing clean-linux,restarting 2003-07-10 */

/* rotate operator */
#define ROTR(VAL,N)							\
  (((VAL) >> (int)((N) & 31)) | ((VAL) << (32 - (int)((N) & 31))))

/* load/store 14-bit unsigned offset field value */
#define OFS		((word_t)(inst & 0xfff))

/* load/store 10-bit unsigned offset field value */
#define HOFS		((word_t)((RS << 4) + RM))

/* fp load/store 8-bit unsigned offset field value */
#define FPOFS		((word_t)((inst & 0xff) << 2))

/* returns 24-bit signed immediate field value - made 2's complement - Onur 07/24/00 */
#define BOFS								\
  ((inst & 0x800000)							\
   ? (0xfc000000 | ((inst & 0xffffff) << 2))				\
   : ((inst & 0x7fffff) << 2))

/* coprocessor operation code for CDP instruction */
#define CPOPC ((inst >> 20) & 0x0f)
#define CPEXT ((inst >> 5) & 0x07)

/* sign-extend operands */
#define SEXT24(X)							\
  (((X) & 0x800000) ? ((sword_t)(X) | 0xff800000)) : (sqword_t)(X))

/* load/store opcode accessors */
#define LDST_PU		((inst >> 23) & 0x03)
#define REGLIST         (inst & 0xffff)

/* floating point opcode accessors */
#define FPIMMBITS	(inst & 0x07)
#define FPIMM		(md_fpimm[FPIMMBITS])

#define EF_SIZE		(((inst >> 18) & 0x02) | ((inst >> 7) & 0x01))
#define LDST_EF_SIZE	(((inst >> 21) & 0x02) | ((inst >> 15) & 0x01))
#define GH_RNDMODE	((inst >> 5) & 0x03)
#define FCNT		(((inst >> 21) & 0x02) | ((inst >> 15) & 0x01))

/* ones counter */
#define ONES(EXPR)	(BX_CPU_THIS_PTR md_ones(EXPR))

/* arithmetic flags */
#define ADDC(LHS, RHS, RES)		BX_CPU_THIS_PTR md_addc((LHS), (RHS), (RES))
#define ADDV(LHS, RHS, RES)		BX_CPU_THIS_PTR md_addv((LHS), (RHS), (RES))
#define SUBC(LHS, RHS, RES)		BX_CPU_THIS_PTR md_subc((LHS), (RHS), (RES))
#define SUBV(LHS, RHS, RES)		BX_CPU_THIS_PTR md_subv((LHS), (RHS), (RES))

/* SWI accessors */
#define SYSCODE		(inst & 0xffffff)

#define GPR(N)                  (*(BX_CPU_THIS_PTR regs.regs_R.regs_R_p[N]))	
/* default target PC handling */
#ifndef SET_TPC
#define SET_TPC(PC)	(void)0
#endif /* SET_TPC */

/* processor status register accessors */
#define _PSR_N(PSR)		(((PSR) >> 31) & 1)
#define _SET_PSR_N(PSR, VAL)						\
  ((PSR) = (((PSR) & ~(1 << 31)) | (((VAL) & 1) << 31)))
#define _PSR_Z(PSR)		(((PSR) >> 30) & 1)
#define _SET_PSR_Z(PSR, VAL)						\
  ((PSR) = (((PSR) & ~(1 << 30)) | (((VAL) & 1) << 30)))
#define _PSR_C(PSR)		(((PSR) >> 29) & 1)
#define _SET_PSR_C(PSR, VAL)						\
  ((PSR) = (((PSR) & ~(1 << 29)) | (((VAL) & 1) << 29)))
#define _PSR_V(PSR)		(((PSR) >> 28) & 1)
#define _SET_PSR_V(PSR, VAL)						\
  ((PSR) = (((PSR) & ~(1 << 28)) | (((VAL) & 1) << 28)))

#define _PSR_IRQ(PSR)		(((PSR) >> 7) & 1)
#define _PSR_FIQ(PSR)		(((PSR) >> 6) & 1)
#define _PSR_MODE(PSR)		((PSR) & 0x1f)

/* condition code values */
#define COND_EQ		0x00
#define COND_NE		0x01
#define COND_CS		0x02
#define COND_CC		0x03
#define COND_MI		0x04
#define COND_PL		0x05
#define COND_VS		0x06
#define COND_VC		0x07
#define COND_HI		0x08
#define COND_LS		0x09
#define COND_GE		0x0a
#define COND_LT		0x0b
#define COND_GT		0x0c
#define COND_LE		0x0d
#define COND_AL		0x0e
#define COND_NV		0x0f

/* condition opcode accessor */
#define COND		((inst >> 28) & 0x0f)

/* test instruction condition value */
#define COND_VALID(PSR)	(BX_CPU_THIS_PTR md_cond_ok(inst, PSR))

/* compute shifted register RM value */
#define SHIFTRM(RMVAL, RSVAL, CFVAL)					\
  (((SHIFT_BITS)==0) ? (RMVAL) : BX_CPU_THIS_PTR md_shiftrm(inst, (RMVAL), (RSVAL), (CFVAL)))

#define SHIFTC(RMVAL, RSVAL, CFVAL)					\
  (((SHIFT_BITS)==0) ? (CFVAL) : BX_CPU_THIS_PTR md_shiftc(inst, (RMVAL), (RSVAL), (CFVAL)))

/*
 * various other helper macros/functions
 */

/* returns non-zero if instruction is a function call */
#define MD_IS_CALL(OP)			((OP) == BRL)

/* addressing mode probe, enums and strings */
enum md_amode_type {
  md_amode_imm,		/* immediate addressing mode */
  md_amode_gp,		/* global data access through global pointer */
  md_amode_sp,		/* stack access through stack pointer */
  md_amode_fp,		/* stack access through frame pointer */
  md_amode_disp,	/* (reg + const) addressing */
  md_amode_rr,		/* (reg + reg) addressing */
  md_amode_NUM
};

/* addressing mode pre-probe FSM, must see all instructions */
#define MD_AMODE_PREPROBE(OP, FSM)		{ (FSM) = 0; }

/* compute addressing mode, only for loads/stores */
#define MD_AMODE_PROBE(AM, OP, FSM)					\
  {									\
    if (MD_OP_FLAGS(OP) & F_DISP)					\
      {									\
	if ((RB) == MD_REG_GP)						\
	  (AM) = md_amode_gp;						\
	else if ((RB) == MD_REG_SP)					\
	  (AM) = md_amode_sp;						\
	else if ((RB) == MD_REG_FP) /* && bind_to_seg(addr) == seg_stack */\
	  (AM) = md_amode_fp;						\
	else								\
	  (AM) = md_amode_disp;						\
      }									\
    else if (MD_OP_FLAGS(OP) & F_RR)					\
      (AM) = md_amode_rr;						\
    else								\
      panic("cannot decode addressing mode");				\
  }


/* register bank specifier */
enum md_reg_type {
  rt_gpr,		/* general purpose register */
  rt_lpr,		/* integer-precision floating pointer register */
  rt_fpr,		/* single-precision floating pointer register */
  rt_dpr,		/* double-precision floating pointer register */
  rt_ctrl,		/* control register */
  rt_PC,		/* program counter */
  rt_NPC,		/* next program counter */
  rt_NUM
};

/* register name specifier */
typedef struct {
  char *str;			/* register name */
  enum md_reg_type file;	/* register file */
  int reg;			/* register index */
}md_reg_names_t;

/* ARM UOP definition */
#define MD_MAX_FLOWLEN		64

struct md_uop_t {
  enum md_opcode op;		/* decoded opcode of the UOP */
  md_inst_t inst;		/* instruction bits of UOP */
};


enum type_spec { type_byte, type_sbyte, type_half, type_shalf, type_word,
		 type_float, type_double, type_extended };
enum ofs_spec { ofs_rm, ofs_shiftrm, ofs_offset };

#endif /* ARM_H */


#include <setjmp.h>

#if BX_SMP_PROCESSORS==1
#define CPU_ID 0
#else
#define CPU_ID (BX_CPU_THIS_PTR local_apic.get_id())
#endif

class BX_CPU_C;

#if BX_SUPPORT_APIC
#define BX_CPU_INTR             (BX_CPU_THIS_PTR INTR || BX_CPU_THIS_PTR local_apic.INTR)
#else
#define BX_CPU_INTR             BX_CPU_THIS_PTR INTR
#endif

#if BX_USE_CPU_SMF == 0
// normal member functions.  This can ONLY be used within BX_CPU_C classes.
// Anyone on the outside should use the BX_CPU macro (defined in bochs.h)
// instead.
#  define BX_CPU_THIS_PTR  this->
#  define BX_CPU_THIS      this
#  define BX_SMF
#  define BX_CPU_C_PREFIX  BX_CPU_C::
// with normal member functions, calling a member fn pointer looks like
// object->*(fnptr)(arg, ...);
// Since this is different from when SMF=1, encapsulate it in a macro.
#  define BX_CPU_CALL_METHOD(func, args) \
            (this->*((BxExecutePtr_t) (func))) args
#else
// static member functions.  With SMF, there is only one CPU by definition.
#  define BX_CPU_THIS_PTR  BX_CPU(0)->
#  define BX_CPU_THIS      BX_CPU(0)
#  define BX_SMF           static
#  define BX_CPU_C_PREFIX
#  define BX_CPU_CALL_METHOD(func, args) \
            ((BxExecutePtr_t) (func)) args
#endif

class bx_cpu_stub_c;
class BX_CPU_C;
class BX_MEM_C;

#if BX_SMP_PROCESSORS==1
// single processor simulation, so there's one of everything
BOCHSAPI extern BX_CPU_C       bx_cpu;
BOCHSAPI extern bx_cpu_stub_c  *pluginCpu;
#else
// multiprocessor simulation, we need an array of cpus and memories
BOCHSAPI extern BX_CPU_C       *bx_cpu_array[BX_SMP_PROCESSORS];
#endif

class BOCHSAPI bx_cpu_stub_c : public logfunctions{
public:
  virtual void init (){};
  // now for some ancillary functions...
  virtual void cpu_loop(Bit32s max_instr_count){};
  virtual void reset(unsigned source){};  
  virtual void set_INTR(bx_bool value){};
  virtual void set_HRQ(){};
  virtual void cpu_load_state () {}
  virtual void cpu_save_state () {}
};

class BOCHSAPI BX_CPU_C : public bx_cpu_stub_c {
	
public: // for now...
/* static const variables 2003-07-02*/
/* enum md_opcode -> mask for decoding next level */
static const unsigned int md_opmask[OP_MAX];
/* enum md_opcode -> shift for decoding next level */
static const unsigned int md_opshift[OP_MAX];
/* enum md_opcode -> description string */
static char * const md_op2name[OP_MAX]; 
/* enum md_opcode -> opcode operand format, used by disassembler */
static char * const md_op2format[OP_MAX] ;
/* enum md_opcode -> enum md_fu_class, used by performance simulators */
/* this need re-write */
static const md_fu_class md_op2fu[OP_MAX]; 
/* enum md_opcode -> opcode flags, used by simulators */
static const unsigned int md_op2flags[OP_MAX];
static const double md_fpimm[8]; 
/* enum md_fu_class -> description string */
static char * const md_fu2name[NUM_FU_CLASSES]; 
static char* const md_amode_str[md_amode_NUM] ;
static const md_reg_names_t md_reg_names[] ;

/* add arm regster files 2003-07-02*/
  bx_regs_t	regs;
/* add arm decode funtion 2003-07-02*/
/* opcode mask -> enum md_opcodem, used by decoder (MD_OP_ENUM()) */
  enum md_opcode md_mask2op[MD_MAX_MASK+1];
  unsigned int md_opoffset[OP_MAX];
  
  char name[64];

#define BX_INHIBIT_INTERRUPTS 0x01
#define BX_INHIBIT_DEBUG      0x02
  // What events to inhibit at any given time.  Certain instructions
  // inhibit interrupts, some debug exceptions and single-step traps.
  unsigned inhibit_mask;

  // pointer to the address space that this processor uses.

  unsigned errorno;   /* signal exception during instruction emulation */

  Bit32u   debug_trap; // holds DR6 value to be set as well
  volatile bx_bool async_event;
  volatile bx_bool INTR;

  // for exceptions
  jmp_buf jmp_buf_env;
  Bit8u curr_exception[2];

#if BX_DEBUGGER
  Bit32u watchpoint;
  Bit8u break_point;
#ifdef MAGIC_BREAKPOINT
  Bit8u magic_break;
#endif
  Bit8u stop_reason;
  Bit8u trace;
  Bit8u trace_reg;
  Bit8u mode_break; /* BW */
  bx_bool debug_vm; /* BW contains current mode*/
  Bit8u show_eip;   /* BW record eip at special instr f.ex eip */
  Bit8u show_flag;  /* BW shows instr class executed */
  bx_guard_found_t guard_found;
#endif

#if BX_SUPPORT_APIC
  bx_local_apic_c local_apic;
#endif

//  BX_MEM_C *mem;
  // constructors & destructors...
  BX_CPU_C();
  ~BX_CPU_C(void);

  virtual void init ();
  // now for some ancillary functions...
  virtual void cpu_loop(Bit32s max_instr_count);
  virtual void set_INTR(bx_bool value);
  virtual void set_HRQ();
  virtual void reset(unsigned source);
  BX_SMF unsigned handleAsyncEvent(void);

//functions for decoding  
  BX_SMF unsigned long  md_set_decoder(char *name,unsigned long mskbits, 
		unsigned long offset,enum md_opcode op, unsigned long max_offset);	  
  BX_SMF void md_init_decoder();
  BX_SMF md_cpu_modes cpu_mode();
  BX_SMF void switch_mode();
  BX_SMF word_t* unbanked_regs(int num);
  BX_SMF int md_ones(word_t val);
  BX_SMF int md_addc(word_t lhs, word_t rhs, word_t res);
  BX_SMF int md_addv(word_t lhs, word_t rhs, word_t res);
  BX_SMF int md_subc(word_t lhs, word_t rhs, word_t res);
  BX_SMF int md_subv(word_t lhs, word_t rhs, word_t res);
  BX_SMF int md_cond_ok(md_inst_t inst, word_t psr);
  BX_SMF word_t md_shiftrm(md_inst_t inst, word_t rmval, word_t rsval, word_t cfval);
  BX_SMF word_t md_shiftc(md_inst_t inst, word_t rmval, word_t rsval, word_t cfval);	
/* debugging instructions 2003-07-15 */
  BX_SMF char * md_reg_name(enum md_reg_type rt, int reg);

  BX_SMF void md_print_ireg(md_gpr_t regs, int reg, FILE *stream);

  BX_SMF void md_print_iregs(md_gpr_t regs, FILE *stream);

  BX_SMF void md_print_creg(md_ctrl_t regs, int reg, FILE *stream);
 
  BX_SMF void md_print_cregs(md_ctrl_t regs, FILE *stream);
 
/* disassemble an Arm instruction */
  BX_SMF void md_print_ifmt(char *fmt,
              md_inst_t inst,           /* instruction to disassemble */
              md_addr_t pc );             /* addr of inst, used for PC-rels */
/* UOP flow generator, returns a small non-cyclic program implementing OP,
   returns length of flow returned */
  BX_SMF int  md_get_flow(enum md_opcode op, md_inst_t inst, struct md_uop_t flow[MD_MAX_FLOWLEN]);


  BX_SMF void     atexit(void);
#if BX_DEBUGGER
  BX_SMF void     dbg_take_irq(void);
  BX_SMF void     dbg_force_interrupt(unsigned vector);
  BX_SMF void     dbg_take_dma(void);
  BX_SMF bx_bool  dbg_get_cpu(bx_dbg_cpu_t *cpu);
  BX_SMF bx_bool  dbg_set_cpu(bx_dbg_cpu_t *cpu);
  BX_SMF bx_bool  dbg_set_reg(unsigned reg, Bit32u val);
  BX_SMF Bit32u   dbg_get_reg(unsigned reg);
  BX_SMF bx_bool  dbg_get_sreg(bx_dbg_sreg_t *sreg, unsigned sreg_no);
  BX_SMF unsigned dbg_query_pending(void);
//  BX_SMF Bit32u   dbg_get_descriptor_l(bx_descriptor_t *);
//  BX_SMF Bit32u   dbg_get_descriptor_h(bx_descriptor_t *);
  BX_SMF Bit32u   dbg_get_eflags(void);
  BX_SMF bx_bool  dbg_is_begin_instr_bpoint(Bit32u laddr);
  BX_SMF bx_bool  dbg_is_end_instr_bpoint(Bit32u laddr);

  BX_SMF void  dbg_print_insn(md_inst_t inst, md_addr_t pc);
  BX_SMF void  dbg_dump_cpu(void);
#endif
  BX_SMF void    debug(Bit32u offset);

  };

#endif  // #ifndef BX_CPU_H
