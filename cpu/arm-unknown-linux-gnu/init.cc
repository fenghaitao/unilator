/////////////////////////////////////////////////////////////////////////
// $Id: init.cc,v 1.7 2003/10/28 13:31:24 fht Exp $
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
  { "$r0",	rt_gpr,		0 },
  { "$v0",	rt_gpr,		0 },
  { "$a0",	rt_gpr,		0 },
  { "$r1",	rt_gpr,		1 },
  { "$a1",	rt_gpr,		1 },
  { "$r2",	rt_gpr,		2 },
  { "$a2",	rt_gpr,		2 },
  { "$r3",	rt_gpr,		3 },
  { "$a3",	rt_gpr,		3 },
  { "$r4",	rt_gpr,		4 },
  { "$r5",	rt_gpr,		5 },
  { "$r6",	rt_gpr,		6 },
  { "$r7",	rt_gpr,		7 },
  { "$r8",	rt_gpr,		8 },
  { "$r9",	rt_gpr,		9 },
  { "$r10",	rt_gpr,		10 },
  { "$r11",	rt_gpr,		11 },
  { "$fp",	rt_gpr,		11 },
  { "$r12",	rt_gpr,		12 },
  { "$ip",	rt_gpr,		12 },
  { "$r13",	rt_gpr,		13 },
  { "$sp",	rt_gpr,		13 },
  { "$r14",	rt_gpr,		14 },
  { "$lr",	rt_gpr,		14 },
  { "$r15",	rt_gpr,		15 },
  { "$pc",	rt_gpr,		15 },

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
//      if (md_mask2op[msk + offset])
//              BX_INFO(("doubly defined opcode, inst=`%s', index=%d",name, msk + offset));
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
#define CONNECT(OP)                                                     \
     offset = max_offset+1;                                            \
     BX_CPU_THIS_PTR md_opoffset[OP] = offset;						
#include "cpu.def"
    if (max_offset >=MD_MAX_MASK)
    BX_CPU_THIS_PTR panic("MASK_MAX is too small, index==%d", max_offset);
}

#if 0
#if   BX_DEBUGGER
#define CONNECT(OP)                                                     \
      offset = max_offset+1;                                            \
    if (md_opoffset[OP])  	                                        \
      BX_INFO(("doubly defined opoffset, inst=`%s', op=%d, offset=%d",  \
            #OP, (int)(OP), offset));                                  \
     BX_CPU_THIS_PTR md_opoffset[OP] = offset;						
#else
#endif
#endif

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

void BX_CPU_C::init()
//void BX_CPU_C::init(BX_MEM_C *addrspace)
{
  BX_DEBUG(( "Init $Id: init.cc,v 1.7 2003/10/28 13:31:24 fht Exp $"));
  // BX_CPU_C constructor
  BX_CPU_THIS_PTR set_INTR (0);
#if BX_SUPPORT_APIC
  local_apic.init ();
#endif

//  mem = addrspace;
  sprintf (name, "CPU %p", this);

#if BX_WITH_WX
  //future extension 2004-02-23  	
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

#define PSR                     (BX_CPU_THIS_PTR regs.regs_C.cpsr)
#define SP	13

  void
BX_CPU_C::reset(unsigned source)
{
  UNUSED(source); // either BX_RESET_HARDWARE or BX_RESET_SOFTWARE
  int romstart;	
 
  BX_CPU_THIS_PTR regs.regs_C.cpsr = 0x0d3;
 
  switch_mode(); 

  romstart = bx_options.rom.Oaddress->get ();
  BX_CPU_THIS_PTR regs.regs_PC  = romstart;
  *BX_CPU_THIS_PTR regs.regs_R.regs_R_p[SP]  = romstart + 0x040000;
  BX_CPU_THIS_PTR regs.regs_NPC = romstart + sizeof(md_inst_t);

#if BX_DEBUGGER
#ifdef MAGIC_BREAKPOINT
  BX_CPU_THIS_PTR magic_break = 0;
#endif
  BX_CPU_THIS_PTR stop_reason = STOP_NO_REASON;
  BX_CPU_THIS_PTR trace = 0;
  BX_CPU_THIS_PTR trace_reg = 0;
#endif
}

  void
BX_CPU_C::set_INTR(bx_bool value)
{
  BX_CPU_THIS_PTR INTR = value;
  BX_CPU_THIS_PTR async_event = 1;
}

  void
BX_CPU_C::set_HRQ()
{
  BX_CPU_THIS_PTR async_event = 1;
}

