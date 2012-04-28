/////////////////////////////////////////////////////////////////////////
// $Id: init.cc,v 1.1.1.1 2003/09/25 03:12:53 fht Exp $
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


#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#define LOG_THIS BX_CPU_THIS_PTR


/* the device id and stepping id are loaded into DH & DL upon processor
   startup.  for device id: 3 = 80386, 4 = 80486.  just make up a
   number for the stepping (revision) id. */
#define BX_DEVICE_ID     3
#define BX_STEPPING_ID   0

BX_CPU_C::BX_CPU_C()
#if BX_SUPPORT_APIC
   : local_apic (this)
#endif
{
  // in case of SMF, you cannot reference any member data
  // in the constructor because the only access to it is via
  // global variables which aren't initialized quite yet.
  put("CPU");
  settype (CPU0LOG);
}


#if BX_WITH_WX

#if BX_SMP_PROCESSORS!=1
#ifdef __GNUC__
#warning cpu_param_handler only supports parameters for one processor.
#endif
// To fix this, I think I will need to change bx_param_num_c::set_handler
// so that I pass in a void* data value.  The void* will be passed to each
// handler.  In this case, I would pass a pointer to the BX_CPU_C object
// in the void*, then in the handler I'd cast it back to BX_CPU_C and call
// BX_CPU_C::cpu_param_handler() which then could be a member function. -BBD
#endif

#define CASE_SEG_REG_GET(x) \
  case BXP_CPU_SEG_##x: \
    return BX_CPU(0)->sregs[BX_SEG_REG_##x].selector.value;
#define CASE_SEG_REG_SET(reg, val) \
  case BXP_CPU_SEG_##reg: \
    BX_CPU(0)->load_seg_reg (&BX_CPU(0)->sregs[BX_SEG_REG_##reg],val); \
    break;
#define CASE_LAZY_EFLAG_GET(flag) \
    case BXP_CPU_EFLAGS_##flag: \
      return BX_CPU(0)->get_##flag ();
#define CASE_LAZY_EFLAG_SET(flag, val) \
    case BXP_CPU_EFLAGS_##flag: \
      BX_CPU(0)->set_##flag(val); \
      break;
#define CASE_EFLAG_GET(flag) \
    case BXP_CPU_EFLAGS_##flag: \
      return BX_CPU(0)->get_##flag ();
#define CASE_EFLAG_SET(flag, val) \
    case BXP_CPU_EFLAGS_##flag: \
      BX_CPU(0)->set_##flag(val); \
      break;


// implement get/set handler for parameters that need unusual set/get
static Bit64s
cpu_param_handler (bx_param_c *param, int set, Bit64s val)
{
  bx_id id = param->get_id ();
  if (set) {
    switch (id) {
      CASE_SEG_REG_SET (CS, val);
      CASE_SEG_REG_SET (DS, val);
      CASE_SEG_REG_SET (SS, val);
      CASE_SEG_REG_SET (ES, val);
      CASE_SEG_REG_SET (FS, val);
      CASE_SEG_REG_SET (GS, val);
      case BXP_CPU_SEG_LDTR:
        BX_CPU(0)->panic("setting LDTR not implemented");
        break;
      case BXP_CPU_SEG_TR:
        BX_CPU(0)->panic ("setting TR not implemented");
        break;
      CASE_LAZY_EFLAG_SET (OF, val);
      CASE_LAZY_EFLAG_SET (SF, val);
      CASE_LAZY_EFLAG_SET (ZF, val);
      CASE_LAZY_EFLAG_SET (AF, val);
      CASE_LAZY_EFLAG_SET (PF, val);
      CASE_LAZY_EFLAG_SET (CF, val);
      CASE_EFLAG_SET (ID,   val);
      //CASE_EFLAG_SET (VIP,  val);
      //CASE_EFLAG_SET (VIF,  val);
      CASE_EFLAG_SET (AC,   val);
      CASE_EFLAG_SET (VM,   val);
      CASE_EFLAG_SET (RF,   val);
      CASE_EFLAG_SET (NT,   val);
      CASE_EFLAG_SET (IOPL, val);
      CASE_EFLAG_SET (DF,   val);
      CASE_EFLAG_SET (IF,   val);
      CASE_EFLAG_SET (TF,   val);
      default:
        BX_CPU(0)->panic ("cpu_param_handler set id %d not handled", id);
    }
  } else {
    switch (id) {
      CASE_SEG_REG_GET (CS);
      CASE_SEG_REG_GET (DS);
      CASE_SEG_REG_GET (SS);
      CASE_SEG_REG_GET (ES);
      CASE_SEG_REG_GET (FS);
      CASE_SEG_REG_GET (GS);
      case BXP_CPU_SEG_LDTR:
        return BX_CPU(0)->ldtr.selector.value;
        break;
      case BXP_CPU_SEG_TR:
        return BX_CPU(0)->tr.selector.value;
        break;
      CASE_LAZY_EFLAG_GET (OF);
      CASE_LAZY_EFLAG_GET (SF);
      CASE_LAZY_EFLAG_GET (ZF);
      CASE_LAZY_EFLAG_GET (AF);
      CASE_LAZY_EFLAG_GET (PF);
      CASE_LAZY_EFLAG_GET (CF);
      CASE_EFLAG_GET (ID);
      //CASE_EFLAG_GET (VIP);
      //CASE_EFLAG_GET (VIF);
      CASE_EFLAG_GET (AC);
      CASE_EFLAG_GET (VM);
      CASE_EFLAG_GET (RF);
      CASE_EFLAG_GET (NT);
      CASE_EFLAG_GET (IOPL);
      CASE_EFLAG_GET (DF);
      CASE_EFLAG_GET (IF);
      CASE_EFLAG_GET (TF);
      default:
        BX_CPU(0)->panic ("cpu_param_handler get id %d ('%s') not handled", id, param->get_name ());
    }
  }
  return val;
}
#undef CASE_SEG_REG_GET
#undef CASE_SEG_REG_SET

#endif


const unsigned int BX_CPU_C::md_opmask[OP_MAX] = {
	    0, /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,O3,I1,I2,I3,I4) 0,
#define DEFUOP(OP,NAME,OPFORM,RES,FLAGS,O1,O2,O3,I1,I2,I3,I4) 0,
#define DEFLINK(OP,MSK,NAME,SHIFT,MASK) MASK,
#define CONNECT(OP)
#include "cpu.def"
};

/* enum md_opcode -> shift for decoding next level */
const unsigned int BX_CPU_C::md_opshift[OP_MAX] = {
	  0, /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,O3,I1,I2,I3,I4) 0,
#define DEFUOP(OP,NAME,OPFORM,RES,FLAGS,O1,O2,O3,I1,I2,I3,I4) 0,
#define DEFLINK(OP,MSK,NAME,SHIFT,MASK) SHIFT,
#define CONNECT(OP)
#include "cpu.def"
};
/* enum md_opcode -> description string */
char * const BX_CPU_C::md_op2name[OP_MAX] = {
	  NULL, /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,O3,I1,I2,I3,I4) NAME,
#define DEFUOP(OP,NAME,OPFORM,RES,FLAGS,O1,O2,O3, I1,I2,I3,I4) NAME,
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT) NAME,
#define CONNECT(OP)
#include "cpu.def"
};

/* enum md_opcode -> opcode operand format, used by disassembler */
char * const BX_CPU_C::md_op2format[OP_MAX] = {
	  NULL, /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,O3,I1,I2,I3,I4) OPFORM,
#define DEFUOP(OP,NAME,OPFORM,RES,FLAGS,O1,O2,O3,I1,I2,I3,I4) OPFORM,
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT) NULL,
#define CONNECT(OP)
#include "cpu.def"
};
/* enum md_opcode -> enum md_fu_class, used by performance simulators */
/* this need re-write */
const md_fu_class BX_CPU_C:: md_op2fu[OP_MAX] = {
	  FUClamd_NA, /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,O3,I1,I2,I3,I4) RES,
#define DEFUOP(OP,NAME,OPFORM,RES,FLAGS,O1,O2,O3,I1,I2,I3,I4) RES,
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT) FUClamd_NA,
#define CONNECT(OP)
#include "cpu.def"
};
/* enum md_opcode -> opcode flags, used by simulators */
const unsigned int BX_CPU_C::md_op2flags[OP_MAX] = {
	  NA, /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,O3,I1,I2,I3,I4) FLAGS,
#define DEFUOP(OP,NAME,OPFORM,RES,FLAGS,O1,O2,O3,I1,I2,I3,I4) FLAGS,
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT) NA,
#define CONNECT(OP)
#include "cpu.def"
};

const double BX_CPU_C::md_fpimm[8] =
  { 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 0.5, 10.0 };

/* enum md_fu_class -> description string */
char * const BX_CPU_C::md_fu2name[NUM_FU_CLASSES] = {
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

char* const BX_CPU_C::md_amode_str[md_amode_NUM] =
{
   "(const)",            /* immediate addressing mode */
   "(gp + const)",       /* global data access through global pointer */
   "(sp + const)",       /* stack access through stack pointer */
   "(fp + const)",       /* stack access through frame pointer */
   "(reg + const)",      /* (reg + const) addressing */
   "(reg + reg)"         /* (reg + reg) addressing */
};

const md_reg_names_t BX_CPU_C::md_reg_names[] =
{
   /* name */    /* file */      /* reg */
   /* integer register file */
  { "$r0",      rt_gpr,         0 },
  { "$v0",      rt_gpr,         0 },
  { "$a0",      rt_gpr,         0 },
  { "$r1",      rt_gpr,         1 },
  { "$a1",      rt_gpr,         1 },
  { "$r2",      rt_gpr,         2 },
  { "$a2",      rt_gpr,         2 },
  { "$r3",      rt_gpr,         3 },
  { "$a3",      rt_gpr,         3 },
  { "$r4",      rt_gpr,         4 },
  { "$r5",      rt_gpr,         5 },
  { "$r6",      rt_gpr,         6 },
  { "$r7",      rt_gpr,         7 },
  { "$r8",      rt_gpr,         8 },
  { "$r9",      rt_gpr,         9 },
  { "$r10",     rt_gpr,         10 },
  { "$r11",     rt_gpr,         11 },
  { "$r12",     rt_gpr,         12 },
  { "$r13",     rt_gpr,         13 },
  { "$r14",     rt_gpr,         14 },
  { "$r15",     rt_gpr,         15 },
  { "$r16",     rt_gpr,         16 },
  { "$r17",     rt_gpr,         17 },
  { "$r18",     rt_gpr,         18 },
  { "$r19",     rt_gpr,         19 },
  { "$r20",     rt_gpr,         20 },
  { "$r21",     rt_gpr,         21 },
  { "$r22",     rt_gpr,         22 },
  { "$r23",     rt_gpr,         23 },
  { "$r24",     rt_gpr,         24 },
  { "$r25",     rt_gpr,         25 },
  { "$r26",     rt_gpr,         26 },
  { "$fp",      rt_gpr,         27 },
  { "$r27",     rt_gpr,         27 },
  { "$ip",      rt_gpr,         28 },
  { "$r28",     rt_gpr,         28 },
  { "$sp",      rt_gpr,         29 },
  { "$r29",     rt_gpr,         29 },
  { "$lr",      rt_gpr,         30 },
  { "$r30",     rt_gpr,         30 },
  { "$pc",      rt_gpr,         31 },
  { "$r31",     rt_gpr,         31 },
 
  /* floating point register file - double precision */
  { "$f0",	rt_fpr,		0 },
  { "$f1",	rt_fpr,		1 },
  { "$f2",	rt_fpr,		2 },
  { "$f3",	rt_fpr,		3 },
  { "$f4",	rt_fpr,		4 },
  { "$f5",	rt_fpr,		5 },
  { "$f6",	rt_fpr,		6 },
  { "$f7",	rt_fpr,		7 },

  /* floating point register file - integer precision */
  { "$l0",	rt_lpr,		0 },
  { "$l1",	rt_lpr,		1 },
  { "$l2",	rt_lpr,		2 },
  { "$l3",	rt_lpr,		3 },
  { "$l4",	rt_lpr,		4 },
  { "$l5",	rt_lpr,		5 },
  { "$l6",	rt_lpr,		6 },
  { "$l7",	rt_lpr,		7 },

  /* miscellaneous registers */
  { "$cpsr",	rt_ctrl,	0 },
  { "$spsr",	rt_ctrl,	1 },
  { "$fpsr",	rt_ctrl,	2 },

  /* program counters */
  { "$pc",	rt_PC,		0 },
  { "$npc",	rt_NPC,		0 },

  /* sentinel */
  { NULL,	rt_gpr,		0 }
};


unsigned long BX_CPU_C::md_set_decoder(char *name,unsigned long mskbits, unsigned long offset,
		enum md_opcode op, unsigned long max_offset)
{
   unsigned long msk_base = mskbits & 0xff;
   unsigned long msk_bound = (mskbits >> 8) & 0xff;
   unsigned long msk;

   msk = msk_base;
   do {
      if ((msk + offset) >= MD_MAX_MASK)   
	      BX_CPU_THIS_PTR panic("MASK_MAX is too small, inst=`%s', index=%d",name, msk + offset);
#if BX_DEBUGGER
      if (md_mask2op[msk + offset])
              BX_INFO(("doubly defined opcode, inst=`%s', index=%d",name, msk + offset));
#endif      
      BX_CPU_THIS_PTR md_mask2op[msk + offset] = op;
      msk++;
      } while (msk <= msk_bound);
   return MAX(max_offset, (msk-1) + offset);
}


void BX_CPU_C::md_init_decoder(void)
{
   unsigned long max_offset = 0;
   unsigned long offset = 0;

#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,O3,I1,I2,I3,I4)      \
      max_offset = md_set_decoder(NAME, (MSK), offset, (OP), max_offset);
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT)                                 \
      max_offset = md_set_decoder(NAME, (MSK), offset, (OP), max_offset);
#if   BX_DEBUGGER
#define CONNECT(OP)                                                     \
      offset = max_offset+1;                                            \
    if (md_opoffset[OP])  	                                        \
      BX_INFO(("doubly defined opoffset, inst=`%s', op=%d, offset=%d",  \
            #OP, (int)(OP), offset));                                  \
     BX_CPU_THIS_PTR md_opoffset[OP] = offset;						
#else
#define CONNECT(OP)                                                     \
     offset = max_offset+1;                                            \
     BX_CPU_THIS_PTR md_opoffset[OP] = offset;						
#endif      
#include "cpu.def"
    if (max_offset >=MD_MAX_MASK)
    BX_CPU_THIS_PTR panic("MASK_MAX is too small, index==%d", max_offset);
}

int BX_CPU_C::md_ones(word_t val)
{
  int i, cnt = 0;
    for (i=0; i < 32; i++)
    {
      if ((val & (1 << i)) != 0)  cnt++;  
    }
    return cnt;   
}	



#define POS(i) ( (~(i)) >> 31 )
#define NEG(i) ( (i) >> 31 )

int BX_CPU_C::md_addc(word_t lhs, word_t rhs, word_t res)
{
  int cflag;

  if (((lhs | rhs) >> 30) != 0)
    cflag = ((NEG(lhs) && NEG(rhs))
	     || (NEG(lhs) && POS(res))
	     || (NEG(rhs) && POS(res)));
  else
    cflag = 0;

  return cflag;
}

int BX_CPU_C::md_addv(word_t lhs, word_t rhs, word_t res)
{
  int vflag;

  if (((lhs | rhs) >> 30) != 0)
    vflag = ((NEG(lhs) && NEG(rhs) && POS(res))
	     || (POS(lhs) && POS(rhs) && NEG(res)));
  else
    vflag = 0;

  return vflag;
}

int BX_CPU_C::md_subc(word_t lhs, word_t rhs, word_t res)
{
  int cflag;

  if ((lhs >= rhs) || ((rhs | lhs) >> 31) != 0)
    cflag = ((NEG(lhs) && POS(rhs))
	     || (NEG(lhs) && POS(res))
	     || (POS(rhs) && POS(res)));
  else
    cflag = 0;

  return cflag;
}

int BX_CPU_C::md_subv(word_t lhs, word_t rhs, word_t res)
{
  int vflag;

  if ((lhs >= rhs) || ((rhs | lhs) >> 31) != 0)
    vflag = ((NEG(lhs) && POS(rhs) && POS(res))
	     || (POS(lhs) && NEG(rhs) && NEG(res)));
  else
    vflag = 0;

  return vflag;
}

int BX_CPU_C::md_cond_ok(md_inst_t inst, word_t psr)
{
  int res;

  switch (COND)
    {
    case COND_EQ:
      res = _PSR_Z(psr);
      break;

    case COND_NE:
      res = !_PSR_Z(psr);
      break;

    case COND_CS:
      res = _PSR_C(psr);
      break;

    case COND_CC:
      res = !_PSR_C(psr);
      break;

    case COND_MI:
      res = _PSR_N(psr);
      break;

    case COND_PL:
      res = !_PSR_N(psr);
      break;

    case COND_VS:
      res = _PSR_V(psr);
      break;

    case COND_VC:
      res = !_PSR_V(psr);
      break;

    case COND_HI:
      res = _PSR_C(psr) && !_PSR_Z(psr);
      break;

    case COND_LS:
      res = !_PSR_C(psr) || _PSR_Z(psr);
      break;

    case COND_GE:
      res = (!_PSR_N(psr) && !_PSR_V(psr)) || (_PSR_N(psr) && _PSR_V(psr));
      break;

    case COND_LT:
      res = (_PSR_N(psr) && !_PSR_V(psr)) || (!_PSR_N(psr) && _PSR_V(psr));
      break;

    case COND_GT:
      res = ((!_PSR_N(psr) && !_PSR_V(psr) && !_PSR_Z(psr))
	     || (_PSR_N(psr) && _PSR_V(psr) && !_PSR_Z(psr)));
      break;

    case COND_LE:
      res = ((_PSR_N(psr) && !_PSR_V(psr))
	     || (!_PSR_N(psr) && _PSR_V(psr))
	     || _PSR_Z(psr));
      break;

    case COND_AL:
      res = TRUE;
      break;

    case COND_NV:
      res = FALSE;
      break;

    default:
      BX_CPU_THIS_PTR panic("bogus predicate condition");
    }
  return res;
}

word_t BX_CPU_C::md_shiftrm(md_inst_t inst, word_t rmval, word_t rsval, word_t cfval)
{
  if (SHIFT_REG)
    {
      int shamt = rsval & 0xff;

      switch (SHIFT_TYPE)
	{
	case SHIFT_LSL:
	  if (shamt == 0)
	    return rmval;
	  else if (shamt >= 32)
	    return 0;
	  else
	    return rmval << shamt;

	case SHIFT_LSR:
	  if (shamt == 0)
	    return rmval;
	  else if (shamt >= 32)
	    return 0;
	  else
	    return rmval >> shamt;

	case SHIFT_ASR:
	  if (shamt == 0)
	    return rmval;
	  else if (shamt >= 32)
	    return (word_t)(((sword_t)rmval) >> 31);
	  else
	    return (word_t)(((sword_t)rmval) >> shamt);

	case SHIFT_ROR:
	  shamt = shamt & 0x1f;
	  if (shamt == 0)
	    return rmval;
	  else
	    return (rmval << (32 - shamt)) | (rmval >> shamt);

	default:
	  BX_CPU_THIS_PTR panic("bogus shift type");
	}
    }
  else /* SHIFT IMM */
    {
      switch (SHIFT_TYPE)
	{
	case SHIFT_LSL:
	  return rmval << SHIFT_SHAMT;

	case SHIFT_LSR:
	  if (SHIFT_SHAMT == 0)
	    return 0;
	  else
	    return rmval >> SHIFT_SHAMT;

	case SHIFT_ASR:
	  if (SHIFT_SHAMT == 0)
	    return (word_t)(((sword_t)rmval) >> 31);
	  else
	    return (word_t)(((sword_t)rmval) >> SHIFT_SHAMT);

	case SHIFT_ROR:
	  if (SHIFT_SHAMT == 0)
	    return (rmval >> 1) | ((!!cfval) << 31);
	  else
	    return (rmval << (32 - SHIFT_SHAMT)) | (rmval >> SHIFT_SHAMT);

	default:
	  BX_CPU_THIS_PTR panic("bogus shift type");
	}
    }
}

word_t BX_CPU_C::md_shiftc(md_inst_t inst, word_t rmval, word_t rsval, word_t cfval)
{
  if (SHIFT_REG)
    {
      int shamt = rsval & 0xff;

      switch (SHIFT_TYPE)
	{
	case SHIFT_LSL:
	  if (shamt == 0)
	    return !!cfval;
	  else if (shamt >= 32)
	    {
	      if (shamt == 32)
		return (rmval & 1);
	      else if (shamt > 32)
		return 0;
	    }
	  else
	    return (rmval >> (32 - shamt)) & 1;

	case SHIFT_LSR:
	  if (shamt == 0)
	    return !!cfval;
	  else if (shamt >= 32)
	    {
	      if (shamt == 32)
		return ((rmval >> 31) & 1);
	      else if (shamt > 32)
		return 0;
	    }
	  else
	    return (rmval >> (shamt - 1)) & 1;

	case SHIFT_ASR:
	  if (shamt == 0)
	    return !!cfval;
	  else if (shamt >= 32)
	    return (rmval >> 31) & 1;
	  else
	    return ((word_t)((sword_t)rmval >> (shamt - 1))) & 1;

	case SHIFT_ROR:
	  if (shamt == 0)
	    return !!cfval;
	  shamt = shamt & 0x1f;
	  if (shamt == 0)
	    return (rmval >> 31) & 1;
	  else
	    return (rmval >> (shamt - 1)) & 1;

	default:
	  BX_CPU_THIS_PTR panic("bogus shift type");
	}
    }
  else /* SHIFT IMM */
    {
      switch (SHIFT_TYPE)
	{
	case SHIFT_LSL:
	  return (rmval >> (32 - SHIFT_SHAMT)) & 1;

	case SHIFT_LSR:
	  if (SHIFT_SHAMT == 0)
	    return (rmval >> 31) & 1;
	  else
	    return (rmval >> (SHIFT_SHAMT - 1)) & 1;

	case SHIFT_ASR:
	  if (SHIFT_SHAMT == 0)
	    return (rmval >> 31) & 1;
	  else
	    return ((word_t)((sword_t)rmval >> (SHIFT_SHAMT - 1))) & 1;

	case SHIFT_ROR:
	  if (SHIFT_SHAMT == 0)
	    return (rmval & 1) & 1;
	  else
	    return (rmval >> (SHIFT_SHAMT - 1)) & 1;

	default:
	  BX_CPU_THIS_PTR panic("bogus shift type");
	}
    }
}

/* instruction assemblers for UOP flow generator */
#define AGEN(C,URD,URN,SHAMT,SHFT,RM)					\
  (0x00000000 | ((URN) << 20) | ((URD) << 14)		\
   | ((SHAMT) << 9) | ((SHFT) << 6) | (RM))
#define AGENI(C,URD,URN,IMM)						\
  (0x00000000 | ((URN) << 20) | ((URD) << 14) | (IMM))
#define LDSTP(C,RD,URN)							\
  (0x00000000 | ((RD) << 14) | ((URN) << 20))


int BX_CPU_C::md_emit_ldst(struct md_uop_t *flow,
	     int high, int load, int pre, int writeback,
	     /* type and format */enum type_spec type, enum ofs_spec ofs,
	     /* dest */int rd,
	     /* src1 */int urn,
	     /* src2 */int shamt, int shift_type, int rm,
	     /* src2 */int sign, unsigned offset)
{
  int nuops = 0, eareg;
  int cond;
  /* adding support for multi-ldst */
  rd += high;

  if (pre)
    {
      /* pre-update */
      if (writeback)
	eareg = urn;
      else
	eareg = MD_REG_TMP0;


      /* emit base register update */
      switch (ofs)
	{
	case ofs_rm:
	  flow[nuops].op = (sign == 1) ? AGEN_U : AGEN;
	  flow[nuops++].inst = AGEN(cond, eareg, urn, 0, 0, rm);
	  break;
	case ofs_shiftrm:
	  flow[nuops].op = (sign == 1) ? AGEN_U : AGEN;
	  flow[nuops++].inst = AGEN(cond, eareg, urn, shamt, shift_type, rm);
	  break;
	case ofs_offset:
	  flow[nuops].op = (sign == 1) ? AGENI_U : AGENI;
	  flow[nuops++].inst = AGENI(cond, eareg, urn, offset);
	  break;
	default:
	  BX_CPU_THIS_PTR panic("bogus offset specifier");
	}

      /* emit load/store with base register access */
      switch (type)
	{
	case type_byte:
	  flow[nuops].op = load ? LDP_B : STP_B;
	  flow[nuops++].inst = LDSTP(cond, rd, eareg);
	  break;
	case type_sbyte:
	  flow[nuops].op = load ? LDP_SB : STP_B;
	  flow[nuops++].inst = LDSTP(cond, rd, eareg);
	  break;
	case type_half:
	  flow[nuops].op = load ? LDP_H : STP_H;
	  flow[nuops++].inst = LDSTP(cond, rd, eareg);
	  break;
	case type_shalf:
	  flow[nuops].op = load ? LDP_SH : STP_H;
	  flow[nuops++].inst = LDSTP(cond, rd, eareg);
	  break;
	case type_word:
	  flow[nuops].op = load ? LDP_W : STP_W;
	  flow[nuops++].inst = LDSTP(cond, rd, eareg);
	  break;
	case type_float:
	  flow[nuops].op = load ? LDP_S : STP_S;
	  flow[nuops++].inst = LDSTP(cond, /* FD */rd & 0x07, eareg);
	  break;
	case type_double:
	  flow[nuops].op = load ? LDP_D : STP_D;
	  flow[nuops++].inst = LDSTP(cond, /* FD */rd & 0x07, eareg);
	  break;
	case type_extended:
	  flow[nuops].op = load ? LDP_E : STP_E;
	  flow[nuops++].inst = LDSTP(cond, /* FD */rd & 0x07, eareg);
	  break;
	default:
	  BX_CPU_THIS_PTR panic("bogus type specifier");
	}
    }
  else
    {
      /* post-update */
      if (!writeback)
	BX_CPU_THIS_PTR panic("post-update without writeback");

      /* emit load/store with base register access */
      switch (type)
	{
	case type_byte:
	  flow[nuops].op = load ? LDP_B : STP_B;
	  flow[nuops++].inst = LDSTP(cond, rd, urn);
	  break;
	case type_sbyte:
	  flow[nuops].op = load ? LDP_SB : STP_B;
	  flow[nuops++].inst = LDSTP(cond, rd, urn);
	  break;
	case type_half:
	  flow[nuops].op = load ? LDP_H : STP_H;
	  flow[nuops++].inst = LDSTP(cond, rd, urn);
	  break;
	case type_shalf:
	  flow[nuops].op = load ? LDP_SH : STP_H;
	  flow[nuops++].inst = LDSTP(cond, rd, urn);
	  break;
	case type_word:
	  flow[nuops].op = load ? LDP_W : STP_W;
	  flow[nuops++].inst = LDSTP(cond, rd, urn);
	  break;
	case type_float:
	  flow[nuops].op = load ? LDP_S : STP_S;
	  flow[nuops++].inst = LDSTP(cond, /* FD */rd & 0x07, urn);
	  break;
	case type_double:
	  flow[nuops].op = load ? LDP_D : STP_D;
	  flow[nuops++].inst = LDSTP(cond, /* FD */rd & 0x07, urn);
	  break;
	case type_extended:
	  flow[nuops].op = load ? LDP_E : STP_E;
	  flow[nuops++].inst = LDSTP(cond, /* FD */rd & 0x07, urn);
	  break;
	default:
	  BX_CPU_THIS_PTR panic("bogus type specifier");
	}

      /* emit base register update */
      switch (ofs)
	{
	case ofs_rm:
	  flow[nuops].op = (sign == 1) ? AGEN_U : AGEN;
	  flow[nuops++].inst = AGEN(cond, urn, urn, 0, 0, rm);
	  break;
	case ofs_shiftrm:
	  flow[nuops].op = (sign == 1) ? AGEN_U : AGEN;
	  flow[nuops++].inst = AGEN(cond, urn, urn, shamt, shift_type, rm);
	  break;
	case ofs_offset:
	  flow[nuops].op = (sign == 1) ? AGENI_U : AGENI;
	  flow[nuops++].inst = AGENI(cond, urn, urn, offset);
	  break;
	default:
	  BX_CPU_THIS_PTR panic("bogus offset specifier");
	}
    }
  return nuops;
}

int BX_CPU_C::md_get_flow(enum md_opcode op, md_inst_t inst,
	    struct md_uop_t flow[MD_MAX_FLOWLEN])
{
  int nuops = 0;
  int nregs = ONES(REGLIST);
  int high  = ((inst >> 6) & 0x01) << 4;
  int offset, sign, pre, writeback, load;
  enum type_spec type;
  enum ofs_spec ofs;

  switch (op)
    {
    case STM:
      offset = nregs*4 - 4;
      sign = -1;
      goto do_STM;
    case STM_U:
      offset = 0;
      sign = 1;
      goto do_STM;
    case STM_PU:
      offset = 4;
      sign = 1;
      goto do_STM;
    case STM_P:
      offset = nregs*4;
      sign = -1;
      goto do_STM;

    do_STM:
      {
	
	int i, rd = 0, rn = (RN);

	for (i=0; i < nregs; i++,rd++)
	  {
	    while ((REGLIST & (1 << rd)) == 0) rd++;
	    nuops +=
	      md_emit_ldst(&flow[nuops],
			   high, /* st */FALSE, /* pre */TRUE, /* !wb */FALSE,
			   /* type and format */type_word, ofs_offset,
			   /* dest */rd,
			   /* src1 */rn,
			   /* src2 */0, 0, 0,
			   /* src2 */sign, sign*i*4+offset);
#if 0
	    flow[nuops].op = (sign == 1) ? STR_PU : STR_P;
	    flow[nuops++].inst =
	      STR(cond,/*P*/1,/*U*/(sign == 1),/*W*/0,
		  /*RD*/rd,/*RN*/rn,/*OFS*/sign*i*4+offset);
#endif
	  }
      }
      break; /* return nuops; */

    case LDM_L:
      offset = nregs*4 - 4;
      sign = -1;
      goto do_LDM;
    case LDM_UL:
      offset = 0;
      sign = 1;
      goto do_LDM;
    case LDM_PUL:
      offset = 4;
      sign = 1;
      goto do_LDM;
    case LDM_PL:
      offset = nregs*4;
      sign = -1;
      goto do_LDM;

    do_LDM:
      {
	int i, rd = 0, rn = (RN);

	flow[nuops].op = AGENI_U;
	flow[nuops++].inst = AGENI(cond, MD_REG_TMP1, rn, /* mov */0);
#if 0
	flow[nuops].op = MOVA;
	flow[nuops++].inst = MOVA(cond,/*RM*/rn);
#endif

	for (i=0; i < nregs; i++,rd++)
	  {
	    while ((REGLIST & (1 << rd)) == 0) rd++;
	    nuops +=
	      md_emit_ldst(&flow[nuops],
			   high, /* ld */TRUE, /* pre */TRUE, /* !wb */FALSE,
			   /* type and format */type_word, ofs_offset,
			   /* dest */rd,
			   /* src1 */MD_REG_TMP1,
			   /* src2 */0, 0, 0,
			   /* src2 */sign, sign*i*4+offset);
#if 0
	    flow[nuops].op = (sign == 1) ? LDA_PU : LDA_P;
	    flow[nuops++].inst =
	      LDR(cond,/*P*/1,/*U*/(sign == 1),/*W*/0,
		  /*RD*/rd,/*RN*/0,/*OFS*/sign*i*4+offset);
#endif
	  }
      }
      break; /* return nuops; */

    case STM_W:
      offset = nregs*4 - 4;
      sign = -1;
      goto do_STM_W;
    case STM_UW:
      offset = 0;
      sign = 1;
      goto do_STM_W;
    case STM_PUW:
      offset = 4;
      sign = 1;
      goto do_STM_W;
    case STM_PW:
      offset = nregs*4;
      sign = -1;
      goto do_STM_W;

    do_STM_W:
      {
	int i, rd = 0, rn = (RN);

	flow[nuops].op = AGENI_U;
	flow[nuops++].inst = AGENI(cond, MD_REG_TMP1, rn, /* mov */0);
#if 0
	flow[nuops].op = MOVA;
	flow[nuops++].inst = MOVA(cond,/*RM*/rn);
#endif

	while ((REGLIST & (1 << rd)) == 0) rd++;
	nuops +=
	  md_emit_ldst(&flow[nuops],
		       high, /* st */FALSE, /* pre */TRUE, /* !wb */FALSE,
		       /* type and format */type_word, ofs_offset,
		       /* dest */rd,
		       /* src1 */MD_REG_TMP1,
		       /* src2 */0, 0, 0,
		       /* src2 */sign, sign*0*4+offset);
#if 0
	flow[nuops].op = (sign == 1) ? STA_PU : STA_P;
	flow[nuops++].inst =
	  STR(cond,/*P*/1,/*U*/(sign == 1),/*W*/0,
	      /*RD*/rd,/*RN*/0,/*OFS*/sign*0*4+offset);
#endif

	flow[nuops].op = (sign == 1) ? AGENI_U : AGENI;
	flow[nuops++].inst = AGENI(cond, rn, rn, nregs*4);
#if 0
	flow[nuops].op = (sign == 1) ? ADDI : SUBI;
	flow[nuops++].inst =
	  ((sign == 1)
	   ? ADDI(cond, rn, rn, 0, nregs*4)
	   : SUBI(cond, rn, rn, 0, nregs*4));
#endif

	for (rd++,i=1; i < nregs; i++,rd++)
	  {
	    while ((REGLIST & (1 << rd)) == 0) rd++;
	    nuops +=
	      md_emit_ldst(&flow[nuops],
			   high, /* st */FALSE, /* pre */TRUE, /* !wb */FALSE,
			   /* type and format */type_word, ofs_offset,
			   /* dest */rd,
			   /* src1 */MD_REG_TMP1,
			   /* src2 */0, 0, 0,
			   /* src2 */sign, sign*i*4+offset);
#if 0
	    flow[nuops].op = (sign == 1) ? STA_PU : STA_P;
	    flow[nuops++].inst =
	      STR(cond,/*P*/1,/*U*/(sign == 1),/*W*/0,
		  /*RD*/rd,/*RN*/0,/*OFS*/sign*i*4+offset);
#endif
	  }
      }
      break; /* return nuops; */

    case LDM_WL:
      offset = nregs*4 - 4;
      sign = -1;
      goto do_LDM_W;
    case LDM_UWL:
      offset = 0;
      sign = 1;
      goto do_LDM_W;
    case LDM_PUWL:
      offset = 4;
      sign = 1;
      goto do_LDM_W;
    case LDM_PWL:
      offset = nregs*4;
      sign = -1;
      goto do_LDM_W;

    do_LDM_W:
      {
	int i, rd = 0, rn = (RN);

	flow[nuops].op = AGENI_U;
	flow[nuops++].inst = AGENI(cond, MD_REG_TMP1, rn, /* mov */0);

	while ((REGLIST & (1 << rd)) == 0) rd++;
	nuops +=
	  md_emit_ldst(&flow[nuops],
		       high, /* ld */TRUE, /* pre */TRUE, /* !wb */FALSE,
		       /* type and format */type_word, ofs_offset,
		       /* dest */rd,
		       /* src1 */MD_REG_TMP1,
		       /* src2 */0, 0, 0,
		       /* src2 */sign, sign*0*4+offset);
#if 0
	flow[nuops].op = (sign == 1) ? LDA_PU : LDA_P;
	flow[nuops++].inst =
	  LDR(cond,/*P*/1,/*U*/(sign == 1),/*W*/0,
	      /*RD*/rd,/*RN*/0,/*OFS*/sign*0*4+offset);
#endif

	flow[nuops].op = (sign == 1) ? AGENI_U : AGENI;
	flow[nuops++].inst = AGENI(cond, rn, rn, nregs*4);

	for (rd++,i=1; i < nregs; i++,rd++)
	  {
	    while ((REGLIST & (1 << rd)) == 0) rd++;
	    nuops +=
	      md_emit_ldst(&flow[nuops],
			   high, /* ld */TRUE, /* pre */TRUE, /* !wb */FALSE,
			   /* type and format */type_word, ofs_offset,
			   /* dest */rd,
			   /* src1 */MD_REG_TMP1,
			   /* src2 */0, 0, 0,
			   /* src2 */sign, sign*i*4+offset);
#if 0
	    flow[nuops].op = (sign == 1) ? LDA_PU : LDA_P;
	    flow[nuops++].inst =
	      LDR(cond,/*P*/1,/*U*/(sign == 1),/*W*/0,
		  /*RD*/rd,/*RN*/0,/*OFS*/sign*i*4+offset);
#endif
	  }
      }
      break; /* return nuops; */

    case SFM_P:
      sign = -1;
      goto do_SFM_P;

    case SFM_PU:
      sign = 1;
      goto do_SFM_P;

    do_SFM_P:
      {
	int i, rn = (RN);
	int count = FCNT ? FCNT : 4;

	for (i=0; i < count; i++)
	  {
	    nuops +=
	      md_emit_ldst(&flow[nuops],
			   high, /* st */FALSE, /* pre */TRUE, /* !wb */FALSE,
			   /* type and format */type_double, ofs_offset,
			   /* dest */(FD)+i,
			   /* src1 */rn,
			   /* src2 */0, 0, 0,
			   /* src2 */sign, sign*i*8+FPOFS);
#if 0
	    flow[nuops].op = (sign == 1) ? STFD_PU : STFD_P;
	    flow[nuops++].inst =
	      STFD(cond,/*P*/1,/*U*/(sign == 1),/*W*/0,
		  /*RD*/(FD)+i,/*RN*/rn,/*OFS*/sign*i*8+FPOFS);
#endif
	  }
      }
      break; /* return nuops; */

    case LFM_PL:
      sign = -1;
      goto do_LFM_P;

    case LFM_PUL:
      sign = 1;
      goto do_LFM_P;

    do_LFM_P:
      {
	int i, rn = (RN);
	int count = FCNT ? FCNT : 4;

	for (i=0; i < count; i++)
	  {
	    nuops +=
	      md_emit_ldst(&flow[nuops],
			   high, /* ld */TRUE, /* pre */TRUE, /* !wb */FALSE,
			   /* type and format */type_double, ofs_offset,
			   /* dest */(FD)+i,
			   /* src1 */rn,
			   /* src2 */0, 0, 0,
			   /* src2 */sign, sign*i*8+FPOFS);
#if 0
	    flow[nuops].op = (sign == 1) ? LDFD_PUL : LDFD_PL;
	    flow[nuops++].inst =
	      LDFD(cond,/*P*/1,/*U*/(sign == 1),/*W*/0,
		  /*RD*/(FD)+i,/*RN*/rn,/*OFS*/sign*i*8+FPOFS);
#endif
	  }
      }
      break; /* return nuops; */

    case SFM_W:
      sign = -1;
      pre = FALSE;
      goto do_SFM_PW;

    case SFM_UW:
      sign = 1;
      pre = FALSE;
      goto do_SFM_PW;

    case SFM_PW:
      sign = -1;
      pre = TRUE;
      goto do_SFM_PW;

    case SFM_PUW:
      sign = 1;
      pre = TRUE;
      goto do_SFM_PW;

    do_SFM_PW:
      {
	int i, rn = (RN);
	int count = FCNT ? FCNT : 4;

	if (pre)
	  {
	    flow[nuops].op = (sign == 1) ? AGENI_U : AGENI;
	    flow[nuops++].inst = AGENI(cond, rn, rn, FPOFS);
	  }
	for (i=0; i < count; i++)
	  {
	    nuops +=
	      md_emit_ldst(&flow[nuops],
			   high, /* st */FALSE, /* pre */TRUE, /* !wb */FALSE,
			   /* type and format */type_double, ofs_offset,
			   /* dest */(FD)+i,
			   /* src1 */rn,
			   /* src2 */0, 0, 0,
			   /* src2 */sign, i*8);
#if 0
	    flow[nuops].op = (sign == 1) ? STFD_PU : STFD_P;
	    flow[nuops++].inst =
	      STFD(cond,/*P*/1,/*U*/(sign == 1),/*W*/0,
		  /*RD*/(FD)+i,/*RN*/rn,/*OFS*/i*8);
#endif
	  }
	if (!pre)
	  {
	    flow[nuops].op = (sign == 1) ? AGENI_U : AGENI;
	    flow[nuops++].inst = AGENI(cond, rn, rn, FPOFS);
	  }
      }
      break; /* return nuops; */

    case LFM_WL:
      sign = -1;
      pre = FALSE;
      goto do_LFM_PW;

    case LFM_UWL:
      sign = 1;
      pre = FALSE;
      goto do_LFM_PW;

    case LFM_PWL:
      sign = -1;
      pre = TRUE;
      goto do_LFM_PW;

    case LFM_PUWL:
      sign = 1;
      pre = TRUE;
      goto do_LFM_PW;

    do_LFM_PW:
      {
	int i, rn = (RN);
	int count = FCNT ? FCNT : 4;

	if (pre)
	  {
	    flow[nuops].op = (sign == 1) ? AGENI_U : AGENI;
	    flow[nuops++].inst = AGENI(cond, rn, rn, FPOFS);
	  }
	for (i=0; i < count; i++)
	  {
	    nuops +=
	      md_emit_ldst(&flow[nuops],
			   high, /* ld */TRUE, /* pre */TRUE, /* !wb */FALSE,
			   /* type and format */type_double, ofs_offset,
			   /* dest */(FD)+i,
			   /* src1 */rn,
			   /* src2 */0, 0, 0,
			   /* src2 */sign, i*8);
#if 0
	    flow[nuops].op = (sign == 1) ? LDFD_PUL : LDFD_PL;
	    flow[nuops++].inst =
	      LDFD(cond,/*P*/1,/*U*/(sign == 1),/*W*/0,
		  /*RD*/(FD)+i,/*RN*/rn,/*OFS*/i*8);
#endif
	  }
	if (!pre)
	  {
	    flow[nuops].op = (sign == 1) ? AGENI_U : AGENI;
	    flow[nuops++].inst = AGENI(cond, rn, rn, FPOFS);
	  }
      }
      break; /* return nuops; */

    case STRH_R:
      load = FALSE;
      type = type_half;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_rm;
      sign = -1;
      offset = 0;
      goto do_STRLDR;
    case STRH_O:
      load = FALSE;
      type = type_half;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = -1;
      offset = HOFS;
      goto do_STRLDR;
    case STRH_RU:
      load = FALSE;
      type = type_half;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_rm;
      sign = 1;
      offset = 0;
      goto do_STRLDR;
    case STRH_OU:
      load = FALSE;
      type = type_half;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = 1;
      offset = HOFS;
      goto do_STRLDR;
    case STRH_PR:
      load = FALSE;
      type = type_half;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_rm;
      sign = -1;
      offset = 0;
      goto do_STRLDR;
    case STRH_PRW:
      load = FALSE;
      type = type_half;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_rm;
      sign = -1;
      offset = 0;
      goto do_STRLDR;
    case STRH_PO:
      load = FALSE;
      type = type_half;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_offset;
      sign = -1;
      offset = HOFS;
      goto do_STRLDR;
    case STRH_POW:
      load = FALSE;
      type = type_half;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = -1;
      offset = HOFS;
      goto do_STRLDR;
    case STRH_PRU:
      load = FALSE;
      type = type_half;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_rm;
      sign = 1;
      offset = 0;
      goto do_STRLDR;
    case STRH_PRUW:
      load = FALSE;
      type = type_half;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_rm;
      sign = 1;
      offset = 0;
      goto do_STRLDR;
    case STRH_POU:
      load = FALSE;
      type = type_half;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_offset;
      sign = 1;
      offset = HOFS;
      goto do_STRLDR;
    case STRH_POUW:
      load = FALSE;
      type = type_half;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = 1;
      offset = HOFS;
      goto do_STRLDR;
    case STR:
      load = FALSE;
      type = type_word;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = -1;
      offset = OFS;
      goto do_STRLDR;
    case STR_B:
      load = FALSE;
      type = type_byte;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = -1;
      offset = OFS;
      goto do_STRLDR;
    case STR_U:
      load = FALSE;
      type = type_word;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = 1;
      offset = OFS;
      goto do_STRLDR;
    case STR_UB:
      load = FALSE;
      type = type_byte;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = 1;
      offset = OFS;
      goto do_STRLDR;
    case STR_P:
      load = FALSE;
      type = type_word;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_offset;
      sign = -1;
      offset = OFS;
      goto do_STRLDR;
    case STR_PW:
      load = FALSE;
      type = type_word;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = -1;
      offset = OFS;
      goto do_STRLDR;
    case STR_PB:
      load = FALSE;
      type = type_byte;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_offset;
      sign = -1;
      offset = OFS;
      goto do_STRLDR;
    case STR_PBW:
      load = FALSE;
      type = type_byte;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = -1;
      offset = OFS;
      goto do_STRLDR;
    case STR_PU:
      load = FALSE;
      type = type_word;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_offset;
      sign = 1;
      offset = OFS;
      goto do_STRLDR;
    case STR_PUW:
      load = FALSE;
      type = type_word;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = 1;
      offset = OFS;
      goto do_STRLDR;
    case STR_PUB:
      load = FALSE;
      type = type_byte;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_offset;
      sign = 1;
      offset = OFS;
      goto do_STRLDR;
    case STR_PUBW:
      load = FALSE;
      type = type_byte;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = 1;
      offset = OFS;
      goto do_STRLDR;
    case STR_R:
      load = FALSE;
      type = type_word;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_shiftrm;
      sign = -1;
      offset = 0;
      goto do_STRLDR;
    case STR_RB:
      load = FALSE;
      type = type_byte;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_shiftrm;
      sign = -1;
      offset = 0;
      goto do_STRLDR;
    case STR_RU:
      load = FALSE;
      type = type_word;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_shiftrm;
      sign = 1;
      offset = 0;
      goto do_STRLDR;
    case STR_RUB:
      load = FALSE;
      type = type_byte;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_shiftrm;
      sign = 1;
      offset = 0;
      goto do_STRLDR;
    case STR_RP:
      load = FALSE;
      type = type_word;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_shiftrm;
      sign = -1;
      offset = 0;
      goto do_STRLDR;
    case STR_RPW:
      load = FALSE;
      type = type_word;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_shiftrm;
      sign = -1;
      offset = 0;
      goto do_STRLDR;
    case STR_RPB:
      load = FALSE;
      type = type_byte;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_shiftrm;
      sign = -1;
      offset = 0;
      goto do_STRLDR;
    case STR_RPBW:
      load = FALSE;
      type = type_byte;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_shiftrm;
      sign = -1;
      offset = 0;
      goto do_STRLDR;
    case STR_RPU:
      load = FALSE;
      type = type_word;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_shiftrm;
      sign = 1;
      offset = 0;
      goto do_STRLDR;
    case STR_RPUW:
      load = FALSE;
      type = type_word;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_shiftrm;
      sign = 1;
      offset = 0;
      goto do_STRLDR;
    case STR_RPUB:
      load = FALSE;
      type = type_byte;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_shiftrm;
      sign = 1;
      offset = 0;
      goto do_STRLDR;
    case STR_RPUBW:
      load = FALSE;
      type = type_byte;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_shiftrm;
      sign = 1;
      offset = 0;
      goto do_STRLDR;

    case STFS_W:
      load = FALSE;
      type = type_float;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = -1;
      offset = FPOFS;
      goto do_STRLDR;
    case STFD_W:
      load = FALSE;
      type = type_double;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = -1;
      offset = FPOFS;
      goto do_STRLDR;
    case STFE_W:
      load = FALSE;
      type = type_extended;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = -1;
      offset = FPOFS;
      goto do_STRLDR;
    case STFS_UW:
      load = FALSE;
      type = type_float;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = 1;
      offset = FPOFS;
      goto do_STRLDR;
    case STFD_UW:
      load = FALSE;
      type = type_double;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = 1;
      offset = FPOFS;
      goto do_STRLDR;
    case STFE_UW:
      load = FALSE;
      type = type_extended;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = 1;
      offset = FPOFS;
      goto do_STRLDR;
    case STFS_P:
      load = FALSE;
      type = type_float;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_offset;
      sign = -1;
      offset = FPOFS;
      goto do_STRLDR;
    case STFD_P:
      load = FALSE;
      type = type_double;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_offset;
      sign = -1;
      offset = FPOFS;
      goto do_STRLDR;
    case STFS_PW:
      load = FALSE;
      type = type_float;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = -1;
      offset = FPOFS;
      goto do_STRLDR;
    case STFD_PW:
      load = FALSE;
      type = type_double;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = -1;
      offset = FPOFS;
      goto do_STRLDR;
    case STFE_P:
      load = FALSE;
      type = type_extended;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_offset;
      sign = -1;
      offset = FPOFS;
      goto do_STRLDR;
    case STFE_PW:
      load = FALSE;
      type = type_extended;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = -1;
      offset = FPOFS;
      goto do_STRLDR;
    case STFS_PU:
      load = FALSE;
      type = type_float;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_offset;
      sign = 1;
      offset = FPOFS;
      goto do_STRLDR;
    case STFD_PU:
      load = FALSE;
      type = type_double;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_offset;
      sign = 1;
      offset = FPOFS;
      goto do_STRLDR;
    case STFS_PUW:
      load = FALSE;
      type = type_float;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = 1;
      offset = FPOFS;
      goto do_STRLDR;
    case STFD_PUW:
      load = FALSE;
      type = type_double;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = 1;
      offset = FPOFS;
      goto do_STRLDR;
    case STFE_PU:
      load = FALSE;
      type = type_extended;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_offset;
      sign = 1;
      offset = FPOFS;
      goto do_STRLDR;
    case STFE_PUW:
      load = FALSE;
      type = type_extended;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = 1;
      offset = FPOFS;
      goto do_STRLDR;

    case LDRH_RL:
      load = TRUE;
      type = type_half;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_rm;
      sign = -1;
      offset = 0;
      goto do_STRLDR;
    case LDRH_OL:
      load = TRUE;
      type = type_half;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = -1;
      offset = HOFS;
      goto do_STRLDR;
    case LDRH_RUL:
      load = TRUE;
      type = type_half;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_rm;
      sign = 1;
      offset = 0;
      goto do_STRLDR;
    case LDRH_OUL:
      load = TRUE;
      type = type_half;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = 1;
      offset = HOFS;
      goto do_STRLDR;
    case LDRH_PRL:
      load = TRUE;
      type = type_half;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_rm;
      sign = -1;
      offset = 0;
      goto do_STRLDR;
    case LDRH_PRWL:
      load = TRUE;
      type = type_half;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_rm;
      sign = -1;
      offset = 0;
      goto do_STRLDR;
    case LDRH_POL:
      load = TRUE;
      type = type_half;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_offset;
      sign = -1;
      offset = HOFS;
      goto do_STRLDR;
    case LDRH_POWL:
      load = TRUE;
      type = type_half;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = -1;
      offset = HOFS;
      goto do_STRLDR;
    case LDRH_PRUL:
      load = TRUE;
      type = type_half;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_rm;
      sign = 1;
      offset = 0;
      goto do_STRLDR;
    case LDRH_PRUWL:
      load = TRUE;
      type = type_half;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_rm;
      sign = 1;
      offset = 0;
      goto do_STRLDR;
    case LDRH_POUL:
      load = TRUE;
      type = type_half;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_offset;
      sign = 1;
      offset = HOFS;
      goto do_STRLDR;
    case LDRH_POUWL:
      load = TRUE;
      type = type_half;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = 1;
      offset = HOFS;
      goto do_STRLDR;
    case LDR_L:
      load = TRUE;
      type = type_word;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = -1;
      offset = OFS;
      goto do_STRLDR;
    case LDR_BL:
      load = TRUE;
      type = type_byte;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = -1;
      offset = OFS;
      goto do_STRLDR;
    case LDR_UL:
      load = TRUE;
      type = type_word;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = 1;
      offset = OFS;
      goto do_STRLDR;
    case LDR_UBL:
      load = TRUE;
      type = type_byte;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = 1;
      offset = OFS;
      goto do_STRLDR;
    case LDR_PL:
      load = TRUE;
      type = type_word;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_offset;
      sign = -1;
      offset = OFS;
      goto do_STRLDR;
    case LDR_PWL:
      load = TRUE;
      type = type_word;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = -1;
      offset = OFS;
      goto do_STRLDR;
    case LDR_PBL:
      load = TRUE;
      type = type_byte;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_offset;
      sign = -1;
      offset = OFS;
      goto do_STRLDR;
    case LDR_PBWL:
      load = TRUE;
      type = type_byte;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = -1;
      offset = OFS;
      goto do_STRLDR;
    case LDR_PUL:
      load = TRUE;
      type = type_word;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_offset;
      sign = 1;
      offset = OFS;
      goto do_STRLDR;
    case LDR_PUWL:
      load = TRUE;
      type = type_word;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = 1;
      offset = OFS;
      goto do_STRLDR;
    case LDR_PUBL:
      load = TRUE;
      type = type_byte;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_offset;
      sign = 1;
      offset = OFS;
      goto do_STRLDR;
    case LDR_PUBWL:
      load = TRUE;
      type = type_byte;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = 1;
      offset = OFS;
      goto do_STRLDR;
    case LDR_RL:
      load = TRUE;
      type = type_word;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_shiftrm;
      sign = -1;
      offset = 0;
      goto do_STRLDR;
    case LDR_RBL:
      load = TRUE;
      type = type_byte;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_shiftrm;
      sign = -1;
      offset = 0;
      goto do_STRLDR;
    case LDR_RUL:
      load = TRUE;
      type = type_word;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_shiftrm;
      sign = 1;
      offset = 0;
      goto do_STRLDR;
    case LDR_RUBL:
      load = TRUE;
      type = type_byte;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_shiftrm;
      sign = 1;
      offset = 0;
      goto do_STRLDR;
    case LDR_RPL:
      load = TRUE;
      type = type_word;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_shiftrm;
      sign = -1;
      offset = 0;
      goto do_STRLDR;
    case LDR_RPWL:
      load = TRUE;
      type = type_word;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_shiftrm;
      sign = -1;
      offset = 0;
      goto do_STRLDR;
    case LDR_RPBL:
      load = TRUE;
      type = type_byte;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_shiftrm;
      sign = -1;
      offset = 0;
      goto do_STRLDR;
    case LDR_RPBWL:
      load = TRUE;
      type = type_byte;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_shiftrm;
      sign = -1;
      offset = 0;
      goto do_STRLDR;
    case LDR_RPUL:
      load = TRUE;
      type = type_word;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_shiftrm;
      sign = 1;
      offset = 0;
      goto do_STRLDR;
    case LDR_RPUWL:
      load = TRUE;
      type = type_word;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_shiftrm;
      sign = 1;
      offset = 0;
      goto do_STRLDR;
    case LDR_RPUBL:
      load = TRUE;
      type = type_byte;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_shiftrm;
      sign = 1;
      offset = 0;
      goto do_STRLDR;
    case LDR_RPUBWL:
      load = TRUE;
      type = type_byte;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_shiftrm;
      sign = 1;
      offset = 0;
      goto do_STRLDR;

    case LDFS_WL:
      load = TRUE;
      type = type_float;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = -1;
      offset = FPOFS;
      goto do_STRLDR;
    case LDFD_WL:
      load = TRUE;
      type = type_double;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = -1;
      offset = FPOFS;
      goto do_STRLDR;
    case LDFE_WL:
      load = TRUE;
      type = type_extended;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = -1;
      offset = FPOFS;
      goto do_STRLDR;
    case LDFS_UWL:
      load = TRUE;
      type = type_float;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = 1;
      offset = FPOFS;
      goto do_STRLDR;
    case LDFD_UWL:
      load = TRUE;
      type = type_double;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = 1;
      offset = FPOFS;
      goto do_STRLDR;
    case LDFE_UWL:
      load = TRUE;
      type = type_extended;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = 1;
      offset = FPOFS;
      goto do_STRLDR;
    case LDFS_PL:
      load = TRUE;
      type = type_float;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_offset;
      sign = -1;
      offset = FPOFS;
      goto do_STRLDR;
    case LDFD_PL:
      load = TRUE;
      type = type_double;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_offset;
      sign = -1;
      offset = FPOFS;
      goto do_STRLDR;
    case LDFS_PWL:
      load = TRUE;
      type = type_float;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = -1;
      offset = FPOFS;
      goto do_STRLDR;
    case LDFD_PWL:
      load = TRUE;
      type = type_double;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = -1;
      offset = FPOFS;
      goto do_STRLDR;
    case LDFE_PL:
      load = TRUE;
      type = type_extended;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_offset;
      sign = -1;
      offset = FPOFS;
      goto do_STRLDR;
    case LDFE_PWL:
      load = TRUE;
      type = type_extended;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = -1;
      offset = FPOFS;
      goto do_STRLDR;
    case LDFS_PUL:
      load = TRUE;
      type = type_float;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_offset;
      sign = 1;
      offset = FPOFS;
      goto do_STRLDR;
    case LDFD_PUL:
      load = TRUE;
      type = type_double;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_offset;
      sign = 1;
      offset = FPOFS;
      goto do_STRLDR;
    case LDFS_PUWL:
      load = TRUE;
      type = type_float;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = 1;
      offset = FPOFS;
      goto do_STRLDR;
    case LDFD_PUWL:
      load = TRUE;
      type = type_double;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = 1;
      offset = FPOFS;
      goto do_STRLDR;
    case LDFE_PUL:
      load = TRUE;
      type = type_extended;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_offset;
      sign = 1;
      offset = FPOFS;
      goto do_STRLDR;
    case LDFE_PUWL:
      load = TRUE;
      type = type_extended;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = 1;
      offset = FPOFS;
      goto do_STRLDR;

    case LDRSB_RL:
      load = TRUE;
      type = type_sbyte;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_rm;
      sign = -1;
      offset = 0;
      goto do_STRLDR;
    case LDRSB_OL:
      load = TRUE;
      type = type_sbyte;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = -1;
      offset = HOFS;
      goto do_STRLDR;
    case LDRSB_RUL:
      load = TRUE;
      type = type_sbyte;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_rm;
      sign = 1;
      offset = 0;
      goto do_STRLDR;
    case LDRSB_OUL:
      load = TRUE;
      type = type_sbyte;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = 1;
      offset = HOFS;
      goto do_STRLDR;
    case LDRSH_RL:
      load = TRUE;
      type = type_shalf;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_rm;
      sign = -1;
      offset = 0;
      goto do_STRLDR;
    case LDRSH_OL:
      load = TRUE;
      type = type_shalf;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = -1;
      offset = HOFS;
      goto do_STRLDR;
    case LDRSH_RUL:
      load = TRUE;
      type = type_shalf;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_rm;
      sign = 1;
      offset = 0;
      goto do_STRLDR;
    case LDRSH_OUL:
      load = TRUE;
      type = type_shalf;
      pre = FALSE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = 1;
      offset = HOFS;
      goto do_STRLDR;
    case LDRSB_PRL:
      load = TRUE;
      type = type_sbyte;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_rm;
      sign = -1;
      offset = 0;
      goto do_STRLDR;
    case LDRSB_PRWL:
      load = TRUE;
      type = type_sbyte;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_rm;
      sign = -1;
      offset = 0;
      goto do_STRLDR;
    case LDRSB_POL:
      load = TRUE;
      type = type_sbyte;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_offset;
      sign = -1;
      offset = HOFS;
      goto do_STRLDR;
    case LDRSB_POWL:
      load = TRUE;
      type = type_sbyte;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = -1;
      offset = HOFS;
      goto do_STRLDR;
    case LDRSB_PRUL:
      load = TRUE;
      type = type_sbyte;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_rm;
      sign = 1;
      offset = 0;
      goto do_STRLDR;
    case LDRSB_PRUWL:
      load = TRUE;
      type = type_sbyte;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_rm;
      sign = 1;
      offset = 0;
      goto do_STRLDR;
    case LDRSB_POUL:
      load = TRUE;
      type = type_sbyte;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_offset;
      sign = 1;
      offset = HOFS;
      goto do_STRLDR;
    case LDRSB_POUWL:
      load = TRUE;
      type = type_sbyte;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = 1;
      offset = HOFS;
      goto do_STRLDR;
    case LDRSH_PRL:
      load = TRUE;
      type = type_shalf;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_rm;
      sign = -1;
      offset = 0;
      goto do_STRLDR;
    case LDRSH_PRWL:
      load = TRUE;
      type = type_shalf;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_rm;
      sign = -1;
      offset = 0;
      goto do_STRLDR;
    case LDRSH_POL:
      load = TRUE;
      type = type_shalf;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_offset;
      sign = -1;
      offset = HOFS;
      goto do_STRLDR;
    case LDRSH_POWL:
      load = TRUE;
      type = type_shalf;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = -1;
      offset = HOFS;
      goto do_STRLDR;
    case LDRSH_PRUL:
      load = TRUE;
      type = type_shalf;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_rm;
      sign = 1;
      offset = 0;
      goto do_STRLDR;
    case LDRSH_PRUWL:
      load = TRUE;
      type = type_shalf;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_rm;
      sign = 1;
      offset = 0;
      goto do_STRLDR;
    case LDRSH_POUL:
      load = TRUE;
      type = type_shalf;
      pre = TRUE;
      writeback = FALSE;
      ofs = ofs_offset;
      sign = 1;
      offset = HOFS;
      goto do_STRLDR;
    case LDRSH_POUWL:
      load = TRUE;
      type = type_shalf;
      pre = TRUE;
      writeback = TRUE;
      ofs = ofs_offset;
      sign = 1;
      offset = HOFS;
      goto do_STRLDR;

    do_STRLDR:
      nuops +=
	md_emit_ldst(&flow[nuops],
		     0, load, pre, writeback,
		     /* type and format */type, ofs,
		     /* dest */RD,
		     /* src1 */RN,
		     /* src2 */SHIFT_SHAMT, SHIFT_TYPE, RM,
		     /* src2 */sign, offset);
      break; /* return nuops; */

    default:
      BX_CPU_THIS_PTR panic("inst `%d' is not a CISC flow", (int)op);
    }

  if (nuops >= MD_MAX_FLOWLEN)
    BX_CPU_THIS_PTR panic("uop flow buffer overflow, increase MD_MAX_FLOWLEN");
  return nuops;
}

void BX_CPU_C::init(BX_MEM_C *addrspace)
{
  BX_DEBUG(( "Init $Id: init.cc,v 1.1.1.1 2003/09/25 03:12:53 fht Exp $"));
  // BX_CPU_C constructor
  BX_CPU_THIS_PTR set_INTR (0);
#if BX_SUPPORT_APIC
  local_apic.init ();
#endif
  // in SMP mode, the prefix of the CPU will be changed to [CPUn] in 
  // bx_local_apic_c::set_id as soon as the apic ID is assigned.

  /* hack for the following fields.  Its easier to decode mod-rm bytes if
     you can assume there's always a base & index register used.  For
     modes which don't really use them, point to an empty (zeroed) register.
   */
  empty_register = 0;

  // 16bit address mode base register, used for mod-rm decoding

  _16bit_base_reg[0] = &gen_reg[BX_16BIT_REG_BX].word.rx;
  _16bit_base_reg[1] = &gen_reg[BX_16BIT_REG_BX].word.rx;
  _16bit_base_reg[2] = &gen_reg[BX_16BIT_REG_BP].word.rx;
  _16bit_base_reg[3] = &gen_reg[BX_16BIT_REG_BP].word.rx;
  _16bit_base_reg[4] = (Bit16u*) &empty_register;
  _16bit_base_reg[5] = (Bit16u*) &empty_register;
  _16bit_base_reg[6] = &gen_reg[BX_16BIT_REG_BP].word.rx;
  _16bit_base_reg[7] = &gen_reg[BX_16BIT_REG_BX].word.rx;

  // 16bit address mode index register, used for mod-rm decoding
  _16bit_index_reg[0] = &gen_reg[BX_16BIT_REG_SI].word.rx;
  _16bit_index_reg[1] = &gen_reg[BX_16BIT_REG_DI].word.rx;
  _16bit_index_reg[2] = &gen_reg[BX_16BIT_REG_SI].word.rx;
  _16bit_index_reg[3] = &gen_reg[BX_16BIT_REG_DI].word.rx;
  _16bit_index_reg[4] = &gen_reg[BX_16BIT_REG_SI].word.rx;
  _16bit_index_reg[5] = &gen_reg[BX_16BIT_REG_DI].word.rx;
  _16bit_index_reg[6] = (Bit16u*) &empty_register;
  _16bit_index_reg[7] = (Bit16u*) &empty_register;

  // for decoding instructions: access to seg reg's via index number
  sreg_mod00_rm16[0] = BX_SEG_REG_DS;
  sreg_mod00_rm16[1] = BX_SEG_REG_DS;
  sreg_mod00_rm16[2] = BX_SEG_REG_SS;
  sreg_mod00_rm16[3] = BX_SEG_REG_SS;
  sreg_mod00_rm16[4] = BX_SEG_REG_DS;
  sreg_mod00_rm16[5] = BX_SEG_REG_DS;
  sreg_mod00_rm16[6] = BX_SEG_REG_DS;
  sreg_mod00_rm16[7] = BX_SEG_REG_DS;

  sreg_mod01_rm16[0] = BX_SEG_REG_DS;
  sreg_mod01_rm16[1] = BX_SEG_REG_DS;
  sreg_mod01_rm16[2] = BX_SEG_REG_SS;
  sreg_mod01_rm16[3] = BX_SEG_REG_SS;
  sreg_mod01_rm16[4] = BX_SEG_REG_DS;
  sreg_mod01_rm16[5] = BX_SEG_REG_DS;
  sreg_mod01_rm16[6] = BX_SEG_REG_SS;
  sreg_mod01_rm16[7] = BX_SEG_REG_DS;

  sreg_mod10_rm16[0] = BX_SEG_REG_DS;
  sreg_mod10_rm16[1] = BX_SEG_REG_DS;
  sreg_mod10_rm16[2] = BX_SEG_REG_SS;
  sreg_mod10_rm16[3] = BX_SEG_REG_SS;
  sreg_mod10_rm16[4] = BX_SEG_REG_DS;
  sreg_mod10_rm16[5] = BX_SEG_REG_DS;
  sreg_mod10_rm16[6] = BX_SEG_REG_SS;
  sreg_mod10_rm16[7] = BX_SEG_REG_DS;

  // the default segment to use for a one-byte modrm with mod==01b
  // and rm==i
  //
  sreg_mod01_rm32[0] = BX_SEG_REG_DS;
  sreg_mod01_rm32[1] = BX_SEG_REG_DS;
  sreg_mod01_rm32[2] = BX_SEG_REG_DS;
  sreg_mod01_rm32[3] = BX_SEG_REG_DS;
  sreg_mod01_rm32[4] = BX_SEG_REG_NULL;
    // this entry should never be accessed
    // (escape to 2-byte)
  sreg_mod01_rm32[5] = BX_SEG_REG_SS;
  sreg_mod01_rm32[6] = BX_SEG_REG_DS;
  sreg_mod01_rm32[7] = BX_SEG_REG_DS;
#if BX_SUPPORT_X86_64
  sreg_mod01_rm32[8] = BX_SEG_REG_DS;
  sreg_mod01_rm32[9] = BX_SEG_REG_DS;
  sreg_mod01_rm32[10] = BX_SEG_REG_DS;
  sreg_mod01_rm32[11] = BX_SEG_REG_DS;
  sreg_mod01_rm32[12] = BX_SEG_REG_DS;
  sreg_mod01_rm32[13] = BX_SEG_REG_DS;
  sreg_mod01_rm32[14] = BX_SEG_REG_DS;
  sreg_mod01_rm32[15] = BX_SEG_REG_DS;
#endif

  // the default segment to use for a one-byte modrm with mod==10b
  // and rm==i
  //
  sreg_mod10_rm32[0] = BX_SEG_REG_DS;
  sreg_mod10_rm32[1] = BX_SEG_REG_DS;
  sreg_mod10_rm32[2] = BX_SEG_REG_DS;
  sreg_mod10_rm32[3] = BX_SEG_REG_DS;
  sreg_mod10_rm32[4] = BX_SEG_REG_NULL;
    // this entry should never be accessed
    // (escape to 2-byte)
  sreg_mod10_rm32[5] = BX_SEG_REG_SS;
  sreg_mod10_rm32[6] = BX_SEG_REG_DS;
  sreg_mod10_rm32[7] = BX_SEG_REG_DS;
#if BX_SUPPORT_X86_64
  sreg_mod10_rm32[8] = BX_SEG_REG_DS;
  sreg_mod10_rm32[9] = BX_SEG_REG_DS;
  sreg_mod10_rm32[10] = BX_SEG_REG_DS;
  sreg_mod10_rm32[11] = BX_SEG_REG_DS;
  sreg_mod10_rm32[12] = BX_SEG_REG_DS;
  sreg_mod10_rm32[13] = BX_SEG_REG_DS;
  sreg_mod10_rm32[14] = BX_SEG_REG_DS;
  sreg_mod10_rm32[15] = BX_SEG_REG_DS;
#endif


  // the default segment to use for a two-byte modrm with mod==00b
  // and base==i
  //
  sreg_mod0_base32[0] = BX_SEG_REG_DS;
  sreg_mod0_base32[1] = BX_SEG_REG_DS;
  sreg_mod0_base32[2] = BX_SEG_REG_DS;
  sreg_mod0_base32[3] = BX_SEG_REG_DS;
  sreg_mod0_base32[4] = BX_SEG_REG_SS;
  sreg_mod0_base32[5] = BX_SEG_REG_DS;
  sreg_mod0_base32[6] = BX_SEG_REG_DS;
  sreg_mod0_base32[7] = BX_SEG_REG_DS;
#if BX_SUPPORT_X86_64
  sreg_mod0_base32[8] = BX_SEG_REG_DS;
  sreg_mod0_base32[9] = BX_SEG_REG_DS;
  sreg_mod0_base32[10] = BX_SEG_REG_DS;
  sreg_mod0_base32[11] = BX_SEG_REG_DS;
  sreg_mod0_base32[12] = BX_SEG_REG_DS;
  sreg_mod0_base32[13] = BX_SEG_REG_DS;
  sreg_mod0_base32[14] = BX_SEG_REG_DS;
  sreg_mod0_base32[15] = BX_SEG_REG_DS;
#endif

  // the default segment to use for a two-byte modrm with
  // mod==01b or mod==10b and base==i
  sreg_mod1or2_base32[0] = BX_SEG_REG_DS;
  sreg_mod1or2_base32[1] = BX_SEG_REG_DS;
  sreg_mod1or2_base32[2] = BX_SEG_REG_DS;
  sreg_mod1or2_base32[3] = BX_SEG_REG_DS;
  sreg_mod1or2_base32[4] = BX_SEG_REG_SS;
  sreg_mod1or2_base32[5] = BX_SEG_REG_SS;
  sreg_mod1or2_base32[6] = BX_SEG_REG_DS;
  sreg_mod1or2_base32[7] = BX_SEG_REG_DS;
#if BX_SUPPORT_X86_64
  sreg_mod1or2_base32[8] = BX_SEG_REG_DS;
  sreg_mod1or2_base32[9] = BX_SEG_REG_DS;
  sreg_mod1or2_base32[10] = BX_SEG_REG_DS;
  sreg_mod1or2_base32[11] = BX_SEG_REG_DS;
  sreg_mod1or2_base32[12] = BX_SEG_REG_DS;
  sreg_mod1or2_base32[13] = BX_SEG_REG_DS;
  sreg_mod1or2_base32[14] = BX_SEG_REG_DS;
  sreg_mod1or2_base32[15] = BX_SEG_REG_DS;
#endif

  mem = addrspace;
  sprintf (name, "CPU %p", this);

#if BX_WITH_WX
  static bx_bool first_time = 1;
  if (first_time) {
    first_time = 0;
    // Register some of the CPUs variables as shadow parameters so that
    // they can be visible in the config interface.
    // (Experimental, obviously not a complete list)
    bx_param_num_c *param;
    const char *fmt16 = "%04X";
    const char *fmt32 = "%08X";
    Bit32u oldbase = bx_param_num_c::set_default_base (16);
    const char *oldfmt = bx_param_num_c::set_default_format (fmt32);
    bx_list_c *list = new bx_list_c (BXP_CPU_PARAMETERS, "CPU State", "", 60);
#define DEFPARAM_NORMAL(name,field) \
    list->add (new bx_shadow_num_c (BXP_CPU_##name, #name, "", &(field)))


      DEFPARAM_NORMAL (EAX, EAX);
      DEFPARAM_NORMAL (EBX, EBX);
      DEFPARAM_NORMAL (ECX, ECX);
      DEFPARAM_NORMAL (EDX, EDX);
      DEFPARAM_NORMAL (ESP, ESP);
      DEFPARAM_NORMAL (EBP, EBP);
      DEFPARAM_NORMAL (ESI, ESI);
      DEFPARAM_NORMAL (EDI, EDI);
      DEFPARAM_NORMAL (EIP, EIP);
      DEFPARAM_NORMAL (DR0, dr0);
      DEFPARAM_NORMAL (DR1, dr1);
      DEFPARAM_NORMAL (DR2, dr2);
      DEFPARAM_NORMAL (DR3, dr3);
      DEFPARAM_NORMAL (DR6, dr6);
      DEFPARAM_NORMAL (DR7, dr7);
#if BX_SUPPORT_X86_64==0
#if BX_CPU_LEVEL >= 2
      DEFPARAM_NORMAL (CR0, cr0.val32);
      DEFPARAM_NORMAL (CR1, cr1);
      DEFPARAM_NORMAL (CR2, cr2);
      DEFPARAM_NORMAL (CR3, cr3);
#endif
#if BX_CPU_LEVEL >= 4
      DEFPARAM_NORMAL (CR4, cr4.registerValue);
#endif
#endif  // #if BX_SUPPORT_X86_64==0

    // segment registers require a handler function because they have
    // special get/set requirements.
#define DEFPARAM_SEG_REG(x) \
    list->add (param = new bx_param_num_c (BXP_CPU_SEG_##x, \
      #x, "", 0, 0xffff, 0)); \
    param->set_handler (cpu_param_handler); \
    param->set_format (fmt16);
#define DEFPARAM_GLOBAL_SEG_REG(name,field) \
    list->add (param = new bx_shadow_num_c (BXP_CPU_##name##_BASE,  \
        #name" base", "", \
        & BX_CPU_THIS_PTR field.base)); \
    list->add (param = new bx_shadow_num_c (BXP_CPU_##name##_LIMIT, \
        #name" limit", "", \
        & BX_CPU_THIS_PTR field.limit));

    DEFPARAM_SEG_REG(CS);
    DEFPARAM_SEG_REG(DS);
    DEFPARAM_SEG_REG(SS);
    DEFPARAM_SEG_REG(ES);
    DEFPARAM_SEG_REG(FS);
    DEFPARAM_SEG_REG(GS);
    DEFPARAM_SEG_REG(LDTR);
    DEFPARAM_SEG_REG(TR);
    DEFPARAM_GLOBAL_SEG_REG(GDTR, gdtr);
    DEFPARAM_GLOBAL_SEG_REG(IDTR, idtr);
#undef DEFPARAM_SEGREG

#if BX_SUPPORT_X86_64==0
    list->add (param = new bx_shadow_num_c (BXP_CPU_EFLAGS, "EFLAGS", "",
        &BX_CPU_THIS_PTR eflags.val32));
#endif

    // flags implemented in lazy_flags.cc must be done with a handler
    // that calls their get function, to force them to be computed.
#define DEFPARAM_EFLAG(name) \
    list->add ( \
        param = new bx_param_bool_c ( \
            BXP_CPU_EFLAGS_##name, \
            #name, "", get_##name())); \
    param->set_handler (cpu_param_handler);
#define DEFPARAM_LAZY_EFLAG(name) \
    list->add ( \
        param = new bx_param_bool_c ( \
            BXP_CPU_EFLAGS_##name, \
            #name, "", get_##name())); \
    param->set_handler (cpu_param_handler);

#if BX_CPU_LEVEL >= 4
    DEFPARAM_EFLAG(ID);
    //DEFPARAM_EFLAG(VIP);
    //DEFPARAM_EFLAG(VIF);
    DEFPARAM_EFLAG(AC);
#endif
#if BX_CPU_LEVEL >= 3
    DEFPARAM_EFLAG(VM);
    DEFPARAM_EFLAG(RF);
#endif
#if BX_CPU_LEVEL >= 2
    DEFPARAM_EFLAG(NT);
    // IOPL is a special case because it is 2 bits wide.
    list->add (
        param = new bx_shadow_num_c (
            BXP_CPU_EFLAGS_IOPL,
            "IOPL", "", 
            &eflags.val32,
            12, 13));
    param->set_range (0, 3);
#if BX_SUPPORT_X86_64==0
    param->set_format ("%d");
#endif
#endif
    DEFPARAM_LAZY_EFLAG(OF);
    DEFPARAM_EFLAG(DF);
    DEFPARAM_EFLAG(IF);
    DEFPARAM_EFLAG(TF);
    DEFPARAM_LAZY_EFLAG(SF);
    DEFPARAM_LAZY_EFLAG(ZF);
    DEFPARAM_LAZY_EFLAG(AF);
    DEFPARAM_LAZY_EFLAG(PF);
    DEFPARAM_LAZY_EFLAG(CF);


    // restore defaults
    bx_param_num_c::set_default_base (oldbase);
    bx_param_num_c::set_default_format (oldfmt);
  }
#endif

#if BX_SupportICache
  iCache.alloc(mem->len);
  iCache.fetchModeMask = 0; // KPL: fixme!!!
#endif
/* add decoder here 2003-07-02*/
  md_init_decoder();
}


BX_CPU_C::~BX_CPU_C(void)
{
  BX_INSTR_SHUTDOWN(CPU_ID);
  BX_DEBUG(( "Exit."));
}



  void
BX_CPU_C::reset(unsigned source)
{
  UNUSED(source); // either BX_RESET_HARDWARE or BX_RESET_SOFTWARE

  // general registers
  EAX = 0; // processor passed test :-)
  EBX = 0; // undefined
  ECX = 0; // undefined
  EDX = (BX_DEVICE_ID << 8) | BX_STEPPING_ID; // ???
  EBP = 0; // undefined
  ESI = 0; // undefined
  EDI = 0; // undefined
  ESP = 0; // undefined

  // all status flags at known values, use BX_CPU_THIS_PTR eflags structure
  BX_CPU_THIS_PTR lf_flags_status = 0x000000;

  // status and control flags register set
  BX_CPU_THIS_PTR setEFlags(0x2); // Bit1 is always set
  BX_CPU_THIS_PTR clear_IF ();
#if BX_CPU_LEVEL >= 3
  BX_CPU_THIS_PTR clear_RF ();
  BX_CPU_THIS_PTR clear_VM ();
#endif
#if BX_CPU_LEVEL >= 4
  BX_CPU_THIS_PTR clear_AC ();
#endif

  BX_CPU_THIS_PTR inhibit_mask = 0;
  BX_CPU_THIS_PTR debug_trap = 0;

  /* instruction pointer */
#if BX_CPU_LEVEL < 2
  BX_CPU_THIS_PTR prev_eip =
  EIP = 0x00000000;
#else /* from 286 up */
  BX_CPU_THIS_PTR prev_eip =
#if BX_SUPPORT_X86_64
  RIP = 0x0000FFF0;
#else
  EIP = 0x0000FFF0;
#endif
#endif

#if BX_SUPPORT_SSE >= 1
  for(unsigned index=0; index < BX_XMM_REGISTERS; index++)
  {
    BX_CPU_THIS_PTR xmm[index].xmm64u(0) = 0;
    BX_CPU_THIS_PTR xmm[index].xmm64u(1) = 0;
  }

  BX_CPU_THIS_PTR mxcsr.mxcsr = MXCSR_RESET;
#endif

  /* CS (Code Segment) and descriptor cache */
  /* Note: on a real cpu, CS initially points to upper memory.  After
   * the 1st jump, the descriptor base is zero'd out.  Since I'm just
   * going to jump to my BIOS, I don't need to do this.
   * For future reference:
   *   processor  cs.selector   cs.base    cs.limit    EIP
   *        8086    FFFF          FFFF0        FFFF   0000
   *        286     F000         FF0000        FFFF   FFF0
   *        386+    F000       FFFF0000        FFFF   FFF0
   */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value =     0xf000;
#if BX_CPU_LEVEL >= 2
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.index =     0x0000;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.ti = 0;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.rpl = 0;

  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.valid =     1;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.p = 1;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.dpl = 0;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.segment = 1; /* data/code segment */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.type = 3; /* read/write access */

  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.executable   = 1; /* data/stack segment */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.c_ed         = 0; /* normal expand up */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.r_w          = 1; /* writeable */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.a            = 1; /* accessed */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.base         = 0x000F0000;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit        =     0xFFFF;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled =     0xFFFF;
#endif
#if BX_CPU_LEVEL >= 3
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.g   = 0; /* byte granular */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.d_b = 0; /* 16bit default size */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.avl = 0;
#endif


  /* SS (Stack Segment) and descriptor cache */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].selector.value =     0x0000;
#if BX_CPU_LEVEL >= 2
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].selector.index =     0x0000;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].selector.ti = 0;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].selector.rpl = 0;

  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.valid =     1;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.p = 1;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.dpl = 0;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.segment = 1; /* data/code segment */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.type = 3; /* read/write access */

  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.executable   = 0; /* data/stack segment */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.c_ed         = 0; /* normal expand up */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.r_w          = 1; /* writeable */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.a            = 1; /* accessed */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.base         = 0x00000000;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.limit        =     0xFFFF;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.limit_scaled =     0xFFFF;
#endif
#if BX_CPU_LEVEL >= 3
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.g   = 0; /* byte granular */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.d_b = 0; /* 16bit default size */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.avl = 0;
#endif


  /* DS (Data Segment) and descriptor cache */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].selector.value =     0x0000;
#if BX_CPU_LEVEL >= 2
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].selector.index =     0x0000;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].selector.ti = 0;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].selector.rpl = 0;

  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.valid =     1;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.p = 1;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.dpl = 0;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.segment = 1; /* data/code segment */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.type = 3; /* read/write access */

  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.executable   = 0; /* data/stack segment */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.c_ed         = 0; /* normal expand up */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.r_w          = 1; /* writeable */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.a            = 1; /* accessed */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.base         = 0x00000000;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.limit        =     0xFFFF;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.limit_scaled =     0xFFFF;
#endif
#if BX_CPU_LEVEL >= 3
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.g   = 0; /* byte granular */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.d_b = 0; /* 16bit default size */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.avl = 0;
#endif


  /* ES (Extra Segment) and descriptor cache */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].selector.value =     0x0000;
#if BX_CPU_LEVEL >= 2
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].selector.index =     0x0000;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].selector.ti = 0;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].selector.rpl = 0;

  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.valid =     1;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.p = 1;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.dpl = 0;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.segment = 1; /* data/code segment */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.type = 3; /* read/write access */

  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.u.segment.executable   = 0; /* data/stack segment */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.u.segment.c_ed         = 0; /* normal expand up */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.u.segment.r_w          = 1; /* writeable */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.u.segment.a            = 1; /* accessed */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.u.segment.base         = 0x00000000;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.u.segment.limit        =     0xFFFF;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.u.segment.limit_scaled =     0xFFFF;
#endif
#if BX_CPU_LEVEL >= 3
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.u.segment.g   = 0; /* byte granular */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.u.segment.d_b = 0; /* 16bit default size */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.u.segment.avl = 0;
#endif


  /* FS and descriptor cache */
#if BX_CPU_LEVEL >= 3
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].selector.value =     0x0000;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].selector.index =     0x0000;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].selector.ti = 0;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].selector.rpl = 0;

  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.valid =     1;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.p = 1;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.dpl = 0;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.segment = 1; /* data/code segment */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.type = 3; /* read/write access */

  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.u.segment.executable   = 0; /* data/stack segment */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.u.segment.c_ed         = 0; /* normal expand up */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.u.segment.r_w          = 1; /* writeable */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.u.segment.a            = 1; /* accessed */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.u.segment.base         = 0x00000000;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.u.segment.limit        =     0xFFFF;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.u.segment.limit_scaled =     0xFFFF;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.u.segment.g   = 0; /* byte granular */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.u.segment.d_b = 0; /* 16bit default size */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.u.segment.avl = 0;
#endif


  /* GS and descriptor cache */
#if BX_CPU_LEVEL >= 3
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].selector.value =     0x0000;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].selector.index =     0x0000;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].selector.ti = 0;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].selector.rpl = 0;

  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.valid =     1;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.p = 1;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.dpl = 0;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.segment = 1; /* data/code segment */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.type = 3; /* read/write access */

  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.u.segment.executable   = 0; /* data/stack segment */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.u.segment.c_ed         = 0; /* normal expand up */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.u.segment.r_w          = 1; /* writeable */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.u.segment.a            = 1; /* accessed */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.u.segment.base         = 0x00000000;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.u.segment.limit        =     0xFFFF;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.u.segment.limit_scaled =     0xFFFF;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.u.segment.g   = 0; /* byte granular */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.u.segment.d_b = 0; /* 16bit default size */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.u.segment.avl = 0;
#endif


  /* GDTR (Global Descriptor Table Register) */
#if BX_CPU_LEVEL >= 2
  BX_CPU_THIS_PTR gdtr.base         = 0x00000000;  /* undefined */
  BX_CPU_THIS_PTR gdtr.limit        =     0x0000;  /* undefined */
  /* ??? AR=Present, Read/Write */
#endif

  /* IDTR (Interrupt Descriptor Table Register) */
#if BX_CPU_LEVEL >= 2
  BX_CPU_THIS_PTR idtr.base         = 0x00000000;
  BX_CPU_THIS_PTR idtr.limit        =     0x03FF; /* always byte granular */ /* ??? */
  /* ??? AR=Present, Read/Write */
#endif

  /* LDTR (Local Descriptor Table Register) */
#if BX_CPU_LEVEL >= 2
  BX_CPU_THIS_PTR ldtr.selector.value =     0x0000;
  BX_CPU_THIS_PTR ldtr.selector.index =     0x0000;
  BX_CPU_THIS_PTR ldtr.selector.ti = 0;
  BX_CPU_THIS_PTR ldtr.selector.rpl = 0;

  BX_CPU_THIS_PTR ldtr.cache.valid   = 0; /* not valid */
  BX_CPU_THIS_PTR ldtr.cache.p       = 0; /* not present */
  BX_CPU_THIS_PTR ldtr.cache.dpl     = 0; /* field not used */
  BX_CPU_THIS_PTR ldtr.cache.segment = 0; /* system segment */
  BX_CPU_THIS_PTR ldtr.cache.type    = 2; /* LDT descriptor */

  BX_CPU_THIS_PTR ldtr.cache.u.ldt.base      = 0x00000000;
  BX_CPU_THIS_PTR ldtr.cache.u.ldt.limit     =     0xFFFF;
#endif

  /* TR (Task Register) */
#if BX_CPU_LEVEL >= 2
  /* ??? I don't know what state the TR comes up in */
  BX_CPU_THIS_PTR tr.selector.value =     0x0000;
  BX_CPU_THIS_PTR tr.selector.index =     0x0000; /* undefined */
  BX_CPU_THIS_PTR tr.selector.ti    =     0;
  BX_CPU_THIS_PTR tr.selector.rpl   =     0;

  BX_CPU_THIS_PTR tr.cache.valid    = 0;
  BX_CPU_THIS_PTR tr.cache.p        = 0;
  BX_CPU_THIS_PTR tr.cache.dpl      = 0; /* field not used */
  BX_CPU_THIS_PTR tr.cache.segment  = 0;
  BX_CPU_THIS_PTR tr.cache.type     = 0; /* invalid */
  BX_CPU_THIS_PTR tr.cache.u.tss286.base             = 0x00000000; /* undefined */
  BX_CPU_THIS_PTR tr.cache.u.tss286.limit            =     0x0000; /* undefined */
#endif

  // DR0 - DR7 (Debug Registers)
#if BX_CPU_LEVEL >= 3
  BX_CPU_THIS_PTR dr0 = 0;   /* undefined */
  BX_CPU_THIS_PTR dr1 = 0;   /* undefined */
  BX_CPU_THIS_PTR dr2 = 0;   /* undefined */
  BX_CPU_THIS_PTR dr3 = 0;   /* undefined */
#endif
#if   BX_CPU_LEVEL == 3
  BX_CPU_THIS_PTR dr6 = 0xFFFF1FF0;
  BX_CPU_THIS_PTR dr7 = 0x00000400;
#elif BX_CPU_LEVEL == 4
  BX_CPU_THIS_PTR dr6 = 0xFFFF1FF0;
  BX_CPU_THIS_PTR dr7 = 0x00000400;
#elif BX_CPU_LEVEL == 5
  BX_CPU_THIS_PTR dr6 = 0xFFFF0FF0;
  BX_CPU_THIS_PTR dr7 = 0x00000400;
#elif BX_CPU_LEVEL == 6
  BX_CPU_THIS_PTR dr6 = 0xFFFF0FF0;
  BX_CPU_THIS_PTR dr7 = 0x00000400;
#else
#  error "DR6,7: CPU > 6"
#endif

#if 0
  /* test registers 3-7 (unimplemented) */
  BX_CPU_THIS_PTR tr3 = 0;   /* undefined */
  BX_CPU_THIS_PTR tr4 = 0;   /* undefined */
  BX_CPU_THIS_PTR tr5 = 0;   /* undefined */
  BX_CPU_THIS_PTR tr6 = 0;   /* undefined */
  BX_CPU_THIS_PTR tr7 = 0;   /* undefined */
#endif

#if BX_CPU_LEVEL >= 2
  // MSW (Machine Status Word), so called on 286
  // CR0 (Control Register 0), so called on 386+
  BX_CPU_THIS_PTR cr0.ts = 0; // no task switch
  BX_CPU_THIS_PTR cr0.em = 0; // emulate math coprocessor
  BX_CPU_THIS_PTR cr0.mp = 0; // wait instructions not trapped
  BX_CPU_THIS_PTR cr0.pe = 0; // real mode
  BX_CPU_THIS_PTR cr0.val32 = 0;

#if BX_CPU_LEVEL >= 3
  BX_CPU_THIS_PTR cr0.pg = 0; // paging disabled
  // no change to cr0.val32
#endif
  BX_CPU_THIS_PTR protectedMode = 0;
  BX_CPU_THIS_PTR v8086Mode = 0;
  BX_CPU_THIS_PTR realMode = 1;

#if BX_CPU_LEVEL >= 4
  BX_CPU_THIS_PTR cr0.cd = 1; // caching disabled
  BX_CPU_THIS_PTR cr0.nw = 1; // not write-through
  BX_CPU_THIS_PTR cr0.am = 0; // disable alignment check
  BX_CPU_THIS_PTR cr0.wp = 0; // disable write-protect
  BX_CPU_THIS_PTR cr0.ne = 0; // ndp exceptions through int 13H, DOS compat
  BX_CPU_THIS_PTR cr0.val32 |= 0x60000000;
#endif

  // handle reserved bits
#if BX_CPU_LEVEL == 3
  // reserved bits all set to 1 on 386
  BX_CPU_THIS_PTR cr0.val32 |= 0x7ffffff0;
#elif BX_CPU_LEVEL >= 4
  // bit 4 is hardwired to 1 on all x86
  BX_CPU_THIS_PTR cr0.val32 |= 0x00000010;
#endif
#endif // CPU >= 2


#if BX_CPU_LEVEL >= 3
  BX_CPU_THIS_PTR cr2 = 0;
  BX_CPU_THIS_PTR cr3 = 0;
#endif
#if BX_CPU_LEVEL >= 4
  BX_CPU_THIS_PTR cr4.setRegister(0);
#endif

#if BX_SUPPORT_X86_64
  BX_CPU_THIS_PTR cpu_mode = BX_MODE_IA32;
#endif


/* initialise MSR registers to defaults */
#if BX_CPU_LEVEL >= 5
  /* APIC Address, APIC enabled and BSP is default, we'll fill in the rest later */
  BX_CPU_THIS_PTR msr.apicbase = (APIC_BASE_ADDR << 12) + 0x900;
#if BX_SUPPORT_X86_64
  BX_CPU_THIS_PTR msr.lme = BX_CPU_THIS_PTR msr.lma = 0;
#endif
#endif

  BX_CPU_THIS_PTR EXT = 0;
  //BX_INTR = 0;

#if BX_SUPPORT_PAGING
#if BX_USE_TLB
  TLB_init();
#endif // BX_USE_TLB
#endif // BX_SUPPORT_PAGING

  BX_CPU_THIS_PTR eipPageBias = 0;
  BX_CPU_THIS_PTR eipPageWindowSize = 0;
  BX_CPU_THIS_PTR eipFetchPtr = NULL;

#if BX_DEBUGGER
#ifdef MAGIC_BREAKPOINT
  BX_CPU_THIS_PTR magic_break = 0;
#endif
  BX_CPU_THIS_PTR stop_reason = STOP_NO_REASON;
  BX_CPU_THIS_PTR trace = 0;
  BX_CPU_THIS_PTR trace_reg = 0;
#endif

  // Init the Floating Point Unit
  /* discard fpu init 2003-07-02 */
/*  fpu_init(); */

#if (BX_SMP_PROCESSORS > 1)
  // notice if I'm the bootstrap processor.  If not, do the equivalent of
  // a HALT instruction.
  int apic_id = local_apic.get_id ();
  if (BX_BOOTSTRAP_PROCESSOR == apic_id)
  {
    // boot normally
    BX_CPU_THIS_PTR bsp = 1;
    BX_CPU_THIS_PTR msr.apicbase |= 0x0100; /* set bit 8 BSP */
    BX_INFO(("CPU[%d] is the bootstrap processor", apic_id));
  } else {
    // it's an application processor, halt until IPI is heard.
    BX_CPU_THIS_PTR bsp = 0;
    BX_CPU_THIS_PTR msr.apicbase &= ~0x0100; /* clear bit 8 BSP */
    BX_INFO(("CPU[%d] is an application processor. Halting until IPI.", apic_id));
    debug_trap |= 0x80000000;
    async_event = 1;
  }
#else
    BX_CPU_THIS_PTR async_event=2;
#endif
  BX_CPU_THIS_PTR kill_bochs_request = 0;

  BX_INSTR_RESET(CPU_ID);
}

  void
BX_CPU_C::set_INTR(bx_bool value)
{
  BX_CPU_THIS_PTR INTR = value;
  BX_CPU_THIS_PTR async_event = 1;
}
