/////////////////////////////////////////////////////////////////////////
// $Id: cpu.cc,v 1.11 2003/11/12 08:41:05 fht Exp $
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

#if BX_USE_CPU_SMF
#define this (BX_CPU(0))
#endif


#if BX_SMP_PROCESSORS==1
// single processor simulation, so there's one of everything
BOCHSAPI bx_cpu_stub_c *pluginCpu;
//BOCHSAPI bx_cpu_stub_c  stubCpu;
BOCHSAPI BX_CPU_C    bx_cpu;
#else
// multiprocessor simulation, we need an array of cpus and memories
BOCHSAPI BX_CPU_C    *bx_cpu_array[BX_SMP_PROCESSORS];
//BOCHSAPI BX_MEM_C    *bx_mem_array[BX_ADDRESS_SPACES];
#endif

#if BX_SMP_PROCESSORS==1
#  define BX_TICK1_IF_SINGLE_PROCESSOR() BX_TICK1()
#else
#  define BX_TICK1_IF_SINGLE_PROCESSOR()
#endif

#include <math.h>

#define SP	13
#define LR	14

//#define MAIN_ID_REG             (BX_CPU_THIS_PTR regs.regs_SYS[0])

/* next program counter */
#define SET_NPC(EXPR)           (BX_CPU_THIS_PTR regs.regs_NPC = (EXPR))
/* current program counter */
#define CPC                    (BX_CPU_THIS_PTR regs.regs_PC)
#define SET_GPR(N,EXPR)                                                 \
  ((void)(((N) == 15) ? setPC++ : 0), (*(BX_CPU_THIS_PTR regs.regs_R.regs_R_p[N])) = (EXPR))

/* processor status register */
#define PSR                     (BX_CPU_THIS_PTR regs.regs_C.cpsr)
#define SET_PSR(EXPR)           (BX_CPU_THIS_PTR regs.regs_C.cpsr = (EXPR))

#define SPSR                     (*BX_CPU_THIS_PTR regs.regs_C.spsr)
#define SET_SPSR(EXPR,spsr_mode)  (BX_CPU_THIS_PTR regs.regs_C.spsr_modes[spsr_mode] = (EXPR))

#define PSR_N                   _PSR_N(BX_CPU_THIS_PTR regs.regs_C.cpsr)
#define SET_PSR_N(EXPR)         _SET_PSR_N(BX_CPU_THIS_PTR regs.regs_C.cpsr, (EXPR))
#define PSR_C                   _PSR_C(BX_CPU_THIS_PTR regs.regs_C.cpsr)
#define SET_PSR_C(EXPR)         _SET_PSR_C(BX_CPU_THIS_PTR regs.regs_C.cpsr, (EXPR))
#define PSR_Z                   _PSR_Z(BX_CPU_THIS_PTR regs.regs_C.cpsr)
#define SET_PSR_Z(EXPR)         _SET_PSR_Z(BX_CPU_THIS_PTR regs.regs_C.cpsr, (EXPR))
#define PSR_V                   _PSR_V(BX_CPU_THIS_PTR regs.regs_C.cpsr)
#define SET_PSR_V(EXPR)         _SET_PSR_V(BX_CPU_THIS_PTR regs.regs_C.cpsr, (EXPR))

	/* floating point conversions */
	union x { float f; word_t w; };
#define DTOW(D)         ({ union x fw; fw.f = (float)(D); fw.w; })
#define WTOD(W)         ({ union x fw; fw.w = (W); (double)fw.f; })
#define QSWP(Q)                                                         \
  ((((Q) << 32) & ULL(0xffffffff00000000)) | (((Q) >> 32) & ULL(0xffffffff)))
/* floating point registers, L->word, F->single-prec, D->double-prec */
#define FPR_Q(N)                (QSWP(BX_CPU_THIS_PTR regs.regs_F.q[N]))
#define SET_FPR_Q(N,EXPR)       (BX_CPU_THIS_PTR regs.regs_F.q[N] = QSWP((EXPR)))
#define FPR_W(N)                (DTOW(BX_CPU_THIS_PTR regs.regs_F.d[N]))
#define SET_FPR_W(N,EXPR)       (BX_CPU_THIS_PTR regs.regs_F.d[N] = (WTOD(EXPR)))
#define FPR(N)                  (BX_CPU_THIS_PTR regs.regs_F.d[(N)])
#define SET_FPR(N,EXPR)         (BX_CPU_THIS_PTR regs.regs_F.d[(N)] = (EXPR))

/* miscellaneous register accessors */
#define FPSR                    (BX_CPU_THIS_PTR regs.regs_C.fpsr)
#define SET_FPSR(EXPR)          (BX_CPU_THIS_PTR regs.regs_C.fpsr = (EXPR))

Bit8u 	xfer_byte;
Bit16u 	xfer_half;
Bit32u 	xfer_word;
Bit64u 	xfer_qword;

#define  SYS_MODE_INST	0
#define  SYS_MODE_DATA  1
#define  USR_MODE_INST  2
#define  USR_MODE_DATA  3
#define  ACCESS_INST	0
#define  ACCESS_DATA	1

/* precise architected memory state accessor macros */
#define READ_BYTE(SRC, FAULT)                                           \
	  ((FAULT) = md_fault_none, access_error = bx_memsys.read_virtual_byte(ACCESS_DATA,SRC,&xfer_byte),xfer_byte)
#define READ_HALF(SRC, FAULT)                                           \
	  ((FAULT) = md_fault_none, access_error = bx_memsys.read_virtual_word(ACCESS_DATA,SRC,&xfer_half),xfer_half)
#define READ_WORD(SRC, FAULT)                                           \
	  ((FAULT) = md_fault_none, access_error = bx_memsys.read_virtual_dword(ACCESS_DATA,SRC,&xfer_word),xfer_word)
#ifdef HOST_HAS_QWORD
#define READ_QWORD(SRC, FAULT)                                          \
	  ((FAULT) = md_fault_none, access_error = bx_memsys.read_virtual_qword(ACCESS_DATA,SRC,&xfer_qword),xfer_qword)
#endif /* HOST_HAS_QWORD */

#define WRITE_BYTE(SRC, DST, FAULT)                                     \
	  ((FAULT) = md_fault_none, xfer_byte = (SRC), access_error = bx_memsys.write_virtual_byte(ACCESS_DATA,DST,&xfer_byte))
#define WRITE_HALF(SRC, DST, FAULT)                                     \
	  ((FAULT) = md_fault_none, xfer_half = (SRC), access_error = bx_memsys.write_virtual_word(ACCESS_DATA,DST,&xfer_half))
#define WRITE_WORD(SRC, DST, FAULT)                                     \
	  ((FAULT) = md_fault_none, xfer_word = (SRC),access_error = bx_memsys.write_virtual_dword(ACCESS_DATA,DST,&xfer_word))
#ifdef HOST_HAS_QWORD
#define WRITE_QWORD(SRC, DST, FAULT)                                    \
	  ((FAULT) = md_fault_none, xfer_qword = (SRC),access_error = bx_memsys.write_virtual_qword(ACCESS_DATA,DST,&xfer_qword))
#endif /* HOST_HAS_QWORD */
	
#define INSTR inst

void
BX_CPU_C::cpu_loop(Bit32s max_instr_count)
{
  md_inst_t inst;
  enum md_opcode op;
  enum md_fault_type fault;  
  int setPC,nflow,flow_index,in_flow=FALSE;
  struct md_uop_t flowtab[MD_MAX_FLOWLEN];

  struct stat stat_buf;
  int fd,ret,romsize,romstart,i;
  char * path;

  Bit32u access_error;

#if BX_DEBUGGER
  BX_CPU_THIS_PTR break_point = 0;
#ifdef MAGIC_BREAKPOINT
  BX_CPU_THIS_PTR magic_break = 0;
#endif
  BX_CPU_THIS_PTR stop_reason = STOP_NO_REASON;
#endif

#if BX_INSTRUMENTATION
  if (setjmp( BX_CPU_THIS_PTR jmp_buf_env )) 
  { 
    // only from exception function can we get here ...
    BX_INSTR_NEW_INSTRUCTION(CPU_ID);
  }
#else
  (void) setjmp( BX_CPU_THIS_PTR jmp_buf_env );
#endif

  while (1) {
  // First check on events which occurred for previous instructions
  // (traps) and ones which are asynchronous to the CPU
  // (hardware interrupts).
  if (!access_error && BX_CPU_THIS_PTR async_event) {
    if (handleAsyncEvent()) {
      // If request to return to caller ASAP.
      return;
      }
    }

#if BX_DEBUGGER
  {
  Bit32u debug_pc = BX_CPU_THIS_PTR regs.regs_PC;
  if ( dbg_is_begin_instr_bpoint(debug_pc) ){
    return;
    }
  }
#endif  // #if BX_DEBUGGER


  fault = md_fault_none;
  access_error = 0;

  if (!in_flow){
cpu_restart:
     if(bx_memsys.read_virtual_dword(ACCESS_INST,BX_CPU_THIS_PTR regs.regs_PC,&inst)) goto cpu_restart;
     /* decode the instruction */
     MD_SET_OPCODE(op, inst);
#if BX_DEBUGGER
    if (BX_CPU_THIS_PTR trace) {
      // print the instruction that is about to be executed.
      bx_dbg_disassemble_current (CPU_ID, 1);  // only one cpu, print time stamp
    }
#endif  // #if BX_DEBUGGER
 }


  setPC = 0;
  *BX_CPU_THIS_PTR regs.regs_R.regs_R_p[MD_REG_PC]= BX_CPU_THIS_PTR regs.regs_PC;

  /* execute the instruction */
  switch (op)
  {
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,O3,I1,I2,I3,I4)      \
   case OP:                                                             \
      SYMCAT(OP,_IMPL);                                                 \
      break;
#define DEFUOP(OP,NAME,OPFORM,RES,FLAGS,O1,O2,O3,I1,I2,I3,I4)           \
   case OP:                                                             \
      SYMCAT(OP,_IMPL);                                                 \
      break;
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT)                                 \
   case OP:                                                             \
      BX_CPU_THIS_PTR panic("attempted to execute a linking opcode");
#define CONNECT(OP)
#define DECLARE_FAULT(FAULT)                                            \
       { fault = (FAULT); break; }
#include "cpu.def"
   default:
       BX_CPU_THIS_PTR panic("attempted to execute a bogus opcode");
   }

   if (fault != md_fault_none){
       md_print_iregs(BX_CPU_THIS_PTR regs.regs_R,stderr);
       md_print_cregs(BX_CPU_THIS_PTR regs.regs_C,stderr);

       BX_CPU_THIS_PTR panic("fault (%d) detected @ 0x%08p instuction code is %08p", fault, BX_CPU_THIS_PTR regs.regs_PC,inst);
}
   if (setPC != 0 && !access_error/* regs.regs_R[MD_REG_PC] != regs.regs_PC */)
        BX_CPU_THIS_PTR regs.regs_NPC = *BX_CPU_THIS_PTR regs.regs_R.regs_R_p[MD_REG_PC];

   if (!in_flow)
        {
/*
        if (!BX_CPU_THIS_PTR regs.regs_NPC){
       	md_print_iregs(BX_CPU_THIS_PTR regs.regs_R,stderr);
       	md_print_cregs(BX_CPU_THIS_PTR regs.regs_C,stderr);
 	BX_PANIC(("current PC is %8x access_error %d",CPC,access_error));
	}
*/
          BX_CPU_THIS_PTR regs.regs_PC = BX_CPU_THIS_PTR regs.regs_NPC;
          BX_CPU_THIS_PTR regs.regs_NPC += sizeof(md_inst_t);
         }
  BX_TICK1_IF_SINGLE_PROCESSOR();

#if BX_DEBUGGER

    // note instr generating exceptions never reach this point.

    // (mch) Read/write, time break point support
    if (BX_CPU_THIS_PTR break_point) {
      switch (BX_CPU_THIS_PTR break_point) {
        case BREAK_POINT_TIME:
          BX_INFO(("[%lld] Caught time breakpoint", bx_pc_system.time_ticks()));
          BX_CPU_THIS_PTR stop_reason = STOP_TIME_BREAK_POINT;
          return;
        case BREAK_POINT_READ:
          BX_INFO(("[%lld] Caught read watch point", bx_pc_system.time_ticks()));
          BX_CPU_THIS_PTR stop_reason = STOP_READ_WATCH_POINT;
          return;
        case BREAK_POINT_WRITE:
          BX_INFO(("[%lld] Caught write watch point", bx_pc_system.time_ticks()));
          BX_CPU_THIS_PTR stop_reason = STOP_WRITE_WATCH_POINT;
          return;
        default:
          BX_PANIC(("Weird break point condition"));
        }
      }
#ifdef MAGIC_BREAKPOINT
    // (mch) Magic break point support
/*
    if (BX_CPU_THIS_PTR magic_break) {
      if (bx_dbg.magic_break_enabled) {
        BX_DEBUG(("Stopped on MAGIC BREAKPOINT"));
        BX_CPU_THIS_PTR stop_reason = STOP_MAGIC_BREAK_POINT;
        return;
        }
      else {
        BX_CPU_THIS_PTR magic_break = 0;
        BX_CPU_THIS_PTR stop_reason = STOP_NO_REASON;
        BX_DEBUG(("Ignoring MAGIC BREAKPOINT"));
        }
      }
*/
#endif

    {
      // check for icount or control-C.  If found, set guard reg and return.
    Bit32u debug_pc = BX_CPU_THIS_PTR regs.regs_PC;
    if ( dbg_is_end_instr_bpoint(debug_pc) ) {
      return;
      }
    }	
#endif  // #if BX_DEBUGGER
#if BX_GDBSTUB
    {
    unsigned int reason;
    if ((reason = bx_gdbstub_check(BX_CPU_THIS_PTR regs.regs_PC)) != GDBSTUB_STOP_NO_REASON) {
      return;
      }
    }
#endif

  }  // while (1)
}



  unsigned
BX_CPU_C::handleAsyncEvent(void)
{
  //
  if (BX_CPU_THIS_PTR inhibit_mask & BX_INHIBIT_INTERRUPTS) {
    // Processing external interrupts is inhibited on this
    // boundary because of certain instructions like STI.
    // inhibit_mask is cleared below, in which case we will have
    // an opportunity to check interrupts on the next instruction
    // boundary.
    }
  else if (BX_CPU_INTR && (!(((PSR)>>7)&0x1)) &&
           BX_DBG_ASYNC_INTR) {
    Bit8u vector;

    //BX_INFO(("interrupt happened current PC is%8x\n",CPC)); 
   // if no local APIC, always acknowledge the PIC.
    vector = DEV_pic_iac(); // may set INTR with next interrupt
    SET_SPSR(PSR,md_cpu_irq);                                       

    SET_PSR((PSR & 0xffffff40)|0x92 );                              
    switch_mode();                                                  

    xfer_word = 1<<11; /* enable IN3 */
    bx_memsys.write_virtual_dword(1,0xfe000188,&xfer_word);
    xfer_word = 1<<11; /* IN3 */
    bx_memsys.write_virtual_dword(1,0xfe000184,&xfer_word);
    xfer_byte = vector; /* PCIIACK */
    bx_memsys.write_virtual_byte(1,0xfc000000,&xfer_byte);

    (*(BX_CPU_THIS_PTR regs.regs_R.regs_R_p[LR])) = CPC + 4;
    CPC = 0x018;       


    BX_INSTR_HWINTERRUPT(CPU_ID, vector,
        BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value, EIP);
    // Set up environment, as would be when this main cpu loop gets
    // invoked.  At the end of normal instructions, we always commmit
    // the new EIP/ESP values.  But here, we call interrupt() much like
    // it was a sofware interrupt instruction, and need to effect the
    // commit here.  This code mirrors similar code above.
   }
  BX_CPU_THIS_PTR inhibit_mask = 0;

  if ( !(BX_CPU_INTR) || BX_HRQ )
    BX_CPU_THIS_PTR async_event = 0;

  return 0; // Continue executing cpu_loop.
}


#if BX_DEBUGGER
extern unsigned int dbg_show_mask;

  bx_bool
BX_CPU_C::dbg_is_begin_instr_bpoint(Bit32u laddr)
{
  //fprintf (stderr, "begin_instr_bp: checking cs:eip %04x:%08x\n", cs, eip);
  BX_CPU_THIS_PTR guard_found.laddr = laddr;
#if 0
  if( (BX_CPU_THIS_PTR show_flag) & (dbg_show_mask)) {
    int rv;
    if((rv = bx_dbg_symbolic_output()))
      return rv;
  }
#endif
  // see if debugger is looking for iaddr breakpoint of any type
  if (bx_guard.guard_for & BX_DBG_GUARD_IADDR_ALL) {
#if BX_DBG_SUPPORT_VIR_BPOINT
    if (bx_guard.guard_for & BX_DBG_GUARD_IADDR_VIR) {
      if (BX_CPU_THIS_PTR guard_found.icount!=0) {
        for (unsigned i=0; i<bx_guard.iaddr.num_virtual; i++) {
          if ( (bx_guard.iaddr.vir[i].cs  == cs) &&
               (bx_guard.iaddr.vir[i].eip == eip) ) {
            BX_CPU_THIS_PTR guard_found.guard_found = BX_DBG_GUARD_IADDR_VIR;
            BX_CPU_THIS_PTR guard_found.iaddr_index = i;
            return(1); // on a breakpoint
            }
          }
        }
      }
#endif
#if BX_DBG_SUPPORT_LIN_BPOINT
    if (bx_guard.guard_for & BX_DBG_GUARD_IADDR_LIN) {
      if (BX_CPU_THIS_PTR guard_found.icount!=0) {
        for (unsigned i=0; i<bx_guard.iaddr.num_linear; i++) {
          if ( bx_guard.iaddr.lin[i].addr == BX_CPU_THIS_PTR guard_found.laddr ) {
            BX_CPU_THIS_PTR guard_found.guard_found = BX_DBG_GUARD_IADDR_LIN;
            BX_CPU_THIS_PTR guard_found.iaddr_index = i;
            return(1); // on a breakpoint
            }
          }
        }
      }
#endif
#if BX_DBG_SUPPORT_PHY_BPOINT
    if (bx_guard.guard_for & BX_DBG_GUARD_IADDR_PHY) {
      Bit32u phy;
      bx_bool valid;
      bx_memsys.pluginMmu->dbg_xlate_linear2phy(BX_CPU_THIS_PTR guard_found.laddr,
                              &phy, &valid);
      // The "guard_found.icount!=0" condition allows you to step or
      // continue beyond a breakpoint.  Bryce tried removing it once,
      // and once you get to a breakpoint you are stuck there forever.
      // Not pretty.
      if (valid && (BX_CPU_THIS_PTR guard_found.icount!=0)) {
        for (unsigned i=0; i<bx_guard.iaddr.num_physical; i++) {
          if ( bx_guard.iaddr.phy[i].addr == phy ) {
            BX_CPU_THIS_PTR guard_found.guard_found = BX_DBG_GUARD_IADDR_PHY;
            BX_CPU_THIS_PTR guard_found.iaddr_index = i;
            return(1); // on a breakpoint
            }
          }
        }
      }
#endif
    }
  return(0); // not on a breakpoint
}

  bx_bool
BX_CPU_C::dbg_is_end_instr_bpoint(Bit32u laddr)
{
  //fprintf (stderr, "end_instr_bp: checking for icount or ^C\n");
  BX_CPU_THIS_PTR guard_found.icount++;

  // convenient point to see if user typed Ctrl-C
  if (bx_guard.interrupt_requested &&
      (bx_guard.guard_for & BX_DBG_GUARD_CTRL_C)) {
    BX_CPU_THIS_PTR guard_found.guard_found = BX_DBG_GUARD_CTRL_C;
    return(1);
    }

  // see if debugger requesting icount guard
  if (bx_guard.guard_for & BX_DBG_GUARD_ICOUNT) {
    if (BX_CPU_THIS_PTR guard_found.icount >= bx_guard.icount) {
      BX_CPU_THIS_PTR guard_found.pc = laddr;
      BX_CPU_THIS_PTR guard_found.guard_found = BX_DBG_GUARD_ICOUNT;
      return(1);
      }
    }

  return(0); // no breakpoint
}


  void
BX_CPU_C::dbg_take_irq(void)
{
  unsigned vector;

  // NOTE: similar code in ::cpu_loop()

}

  void
BX_CPU_C::dbg_force_interrupt(unsigned vector)
{
  // Used to force slave simulator to take an interrupt, without
  // regard to IF

  if ( setjmp(BX_CPU_THIS_PTR jmp_buf_env) == 0 ) {
    // normal return from setjmp setup
    BX_CPU_THIS_PTR errorno = 0;
//    BX_CPU_THIS_PTR EXT   = 1; // external event
    BX_CPU_THIS_PTR async_event = 1; // probably don't need this
//    interrupt(vector, 0, 0, 0);
    }
}

  void
BX_CPU_C::dbg_take_dma(void)
{
  // NOTE: similar code in ::cpu_loop()
  if ( BX_HRQ ) {
    BX_CPU_THIS_PTR async_event = 1; // set in case INTR is triggered
    DEV_dma_raise_hlda();
    }
}
#endif  // #if BX_DEBUGGER

static char *md_cond[16] =
  { "eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc",
    "hi", "ls", "ge", "lt", "gt", "le", "", "nv" };

static char *md_shift_type[4] = { "lsl", "lsr", "asr", "ror" };
static char *md_pu_code[4] = { "da", "ia", "db", "ib" };
static char *md_ef_size[4] = { "s", "d", "e", "??" };
static char *md_gh_rndmode[4] = { "", "p", "m", "z" };
static char *md_fp_imm[8] =
  { "0.0", "1.0", "2.0", "3.0", "4.0", "5.0", "0.5", "10.0" };

int print_urd = 0;

#if BX_DEBUGGER
void 
BX_CPU_C::dbg_dump_cpu()
{
  dbg_printf (
    "r0: %08X\tr1: %08X\tr2: %08X\tr3: %08X\nr4: %08X\tr5: %08X\tr6: %08X\tr7: %08X\nr8: %08X\tr9: %08X\tr10: %08X\tr11: %08X\nr12: %08X\tsp: %08X\tlr: %08X\tpc:%08X\n",
    GPR(0),
    GPR(1),
    GPR(2),
    GPR(3),
    GPR(4),
    GPR(5),
    GPR(6),
    GPR(7),
    GPR(8),
    GPR(9),
    GPR(10),
    GPR(11),
    GPR(12),
    GPR(13),
    GPR(14),
    GPR(15)
    );
    dbg_printf ("CPSR: 0x%08x\t SPSR:0x%08x\n", BX_CPU_THIS_PTR regs.regs_C.cpsr,*(BX_CPU_THIS_PTR regs.regs_C.spsr));

}

void
BX_CPU_C::dbg_print_insn(md_inst_t inst, /* instruction to disassemble */
	      md_addr_t pc               /* addr of inst, used for PC-rels */)
{
  enum md_opcode op;

  /* decode the instruction, assumes predecoded text segment */
  MD_SET_OPCODE(op, inst);

  /* disassemble the instruction */
  if (op <= OP_NA || op >= OP_MAX) 
    {
      /* bogus instruction */
		dbg_printf("<invalid inst: 0x%08x>, op = %d\n", inst, op);
    }
  else if ((((inst >> 25) & 0x7) == 0x3) && ((inst >> 4) & 0x1)) {
  		dbg_printf("<undefined inst:0x%08x>",inst);
	 }
  else
    {
      md_print_ifmt(MD_OP_NAME(op), inst, pc);
      dbg_printf( "  ");
      md_print_ifmt(MD_OP_FORMAT(op), inst, pc);
      dbg_printf( "\n");
    }
}

void BX_CPU_C::md_print_ifmt(char *fmt,
	      md_inst_t inst,		/* instruction to disassemble */
	      md_addr_t pc )		/* addr of inst, used for PC-rels */
{

  int i;
  char *s = fmt;
  print_urd = 0;

  while (*s)
    {
      if (*s == '%')
	{
	  s++;
	  switch (*s)
	    {
	    case 'c':
	      dbg_printf("%s",md_cond[COND]);
	      break;

	    case 'd':
              print_urd = 1;
	      dbg_printf("r%d", RD);
	      break;

	    case 'v':
              print_urd = 2;
	      if (URD < 16)
		dbg_printf("r%d", URD);
	      else
		dbg_printf( "tmp%d", URD-16);
	      break;

	    case 'n':
	      dbg_printf( "r%d", RN);
	      break;
	    case 'u':
	      if (URN < 16)
		dbg_printf( "r%d", URN);
	      else
		dbg_printf( "tmp%d", URN-16);
	      break;

	    case 's':
	      dbg_printf( "r%d", RS);
	      break;
	    case 'w':
	      dbg_printf( "r%d", RM);
	      break;
	    case 'm':
	      if (!SHIFT_REG && !SHIFT_SHAMT)
		dbg_printf( "r%d", RM);
	      else if (SHIFT_REG && !SHIFT_REG_PAD)
		dbg_printf(
			"r%d, %s r%d", RM, md_shift_type[SHIFT_TYPE], RS);
	      else if (SHIFT_REG && SHIFT_REG_PAD)
		dbg_printf( "%s r%d (invalid pad!!!)",
			md_shift_type[SHIFT_TYPE], RS);
	      else
		dbg_printf(
			"r%d, %s #%d", RM, md_shift_type[SHIFT_TYPE], SHIFT_SHAMT);
	      break;

	    case 'D':
	      dbg_printf( "f%d", FD);
	      break;
	    case 'N':
	      dbg_printf( "f%d", FN);
	      break;
	    case 'M':
	      dbg_printf( "f%d", FM);
	      break;

	    case 'i':
	      dbg_printf( "%d (%d >>> %d)",
		      ROTR(ROTIMM, ROTAMT << 1), ROTIMM, ROTAMT);
	      break;
	    case 'o':
	      dbg_printf( "%d", OFS);
	      break;
	    case 'h':
	      dbg_printf( "%d", HOFS);
	      break;
	    case 'O':
	      dbg_printf( "%d", FPOFS);
	      break;
	    case 'I':
	      dbg_printf( "%s", md_fp_imm[FPIMMBITS]);
	      break;

	    case 'a':
	      dbg_printf( "%s", md_pu_code[LDST_PU]);
	      break;
	    case 'R':
	      {
		int first = TRUE;

		dbg_printf( "{");
		for (i=0; i < 16; i++)
		  {
		    if (REGLIST & (1 << i))
		      {
			if (!first)
			  dbg_printf( ",");
			dbg_printf( "r%d", i);
			first = FALSE;
		      }
		  }
		dbg_printf( "}");
	      }
	      break;
	    case 't':
	      dbg_printf( "%s", md_ef_size[EF_SIZE]);
	      break;
	    case 'T':
	      dbg_printf( "%s", md_ef_size[LDST_EF_SIZE]);
	      break;
	    case 'r':
	      dbg_printf( "%s", md_gh_rndmode[GH_RNDMODE]);
	      break;
	    case 'C':
	      dbg_printf( "%d", FCNT ? FCNT : 4);
	      break;

	    case 'j':
	      dbg_printf( "0x%p", pc + BOFS + 8);
	      break;

	    case 'S':
	      dbg_printf( "0x%08x", SYSCODE);
	      break;
	    case 'P':
	      dbg_printf( "%d", RS);
	      break;
	    case 'p':
	      dbg_printf( "%d", CPOPC);
	      break;
	    case 'g':
	      dbg_printf( "%d", CPEXT);
	      break;

	    default:
	      BX_PANIC(("unknown disassembler escape code `%c'", *s));
	    }
	}
      else
	dbg_printf("%c",*s);
	//fputc(*s, stream);
      s++;
    }
}
#endif

/* returns a register name string */
char *
BX_CPU_C::md_reg_name(enum md_reg_type rt, int reg)
{
  int i;

  for (i=0; BX_CPU_C::md_reg_names[i].str != NULL; i++)
    {
      if (BX_CPU_C::md_reg_names[i].file == rt 
		&& BX_CPU_C::md_reg_names[i].reg == reg)
        return BX_CPU_C::md_reg_names[i].str;
    }

  /* no found... */
  return NULL;
}

/* print integer REG(S) to STREAM */
void
BX_CPU_C::md_print_ireg(md_gpr_t regs, int reg, FILE *stream)
{
    fprintf(stream, "%4s: %16d/0x%08x",
            md_reg_name(rt_gpr, reg), *regs.regs_R_p[reg], 
			*regs.regs_R_p[reg]);
}

void
BX_CPU_C::md_print_iregs(md_gpr_t regs, FILE *stream)
{
  int i;

#define ARCH_NUM_IREGS 16
  for (i=0; i < ARCH_NUM_IREGS; i += 2)
    {
      md_print_ireg(regs, i, stream);
      fprintf(stream, "  ");
      md_print_ireg(regs, i+1, stream);
      fprintf(stream, "\n");
    }
} 

void
BX_CPU_C::md_print_creg(md_ctrl_t regs, int reg, FILE *stream)
{
  /* index is only used to iterate over these registers, hence no enums... */
  switch (reg)
    {
    case 0:
      fprintf(stream, "CPSR: 0x%08x", regs.cpsr);
      break;

    case 1:
      fprintf(stream, "SPSR: 0x%08x", *regs.spsr);
      break;

    default:
      BX_PANIC(("bogus control register index"));
    }
}


void
BX_CPU_C::md_print_cregs(md_ctrl_t regs, FILE *stream)
{
  md_print_creg(regs, 0, stream);
  fprintf(stream, "  ");
  md_print_creg(regs, 1, stream);
  fprintf(stream, "  \n");
}

md_cpu_modes

BX_CPU_C::cpu_mode()
{
   md_inst_t inst;
   switch ((PSR)&0x1f) 
    {
	case 0x10: return md_cpu_usr;
	case 0x11: return md_cpu_fiq;
	case 0x12: return md_cpu_irq;
	case 0x13: return md_cpu_svc;
	case 0x17: return md_cpu_abt;
	case 0x1b: return md_cpu_abt;
	default : 
           md_print_iregs(BX_CPU_THIS_PTR regs.regs_R,stderr);
	   md_print_cregs(BX_CPU_THIS_PTR regs.regs_C,stderr);
			
     	   bx_memsys.read_virtual_dword(1,BX_CPU_THIS_PTR regs.regs_PC,&inst);
	   fprintf(stderr,"instruction is %8x\n",inst);
           BX_PANIC(("Unknown cpu modes"));
    }	
}

void
BX_CPU_C::switch_mode()
{
 int i;

 for (i=0;i<UNBANKED_NUM_IREGS;i++)
    BX_CPU_THIS_PTR regs.regs_R.regs_R_p[i] =
         &(BX_CPU_THIS_PTR regs.regs_R.regs_R_unbanked[i]);
  BX_CPU_THIS_PTR regs.regs_R.regs_R_p[8] =
         &BX_CPU_THIS_PTR regs.regs_R.r8[is_fiq_mode];
  BX_CPU_THIS_PTR regs.regs_R.regs_R_p[9] =
         &BX_CPU_THIS_PTR regs.regs_R.r9[is_fiq_mode];
  BX_CPU_THIS_PTR regs.regs_R.regs_R_p[10] =
         &BX_CPU_THIS_PTR regs.regs_R.r10[is_fiq_mode];
  BX_CPU_THIS_PTR regs.regs_R.regs_R_p[11] =
         &BX_CPU_THIS_PTR regs.regs_R.r11[is_fiq_mode];
  BX_CPU_THIS_PTR regs.regs_R.regs_R_p[12] =
         &BX_CPU_THIS_PTR regs.regs_R.r12[is_fiq_mode];
  BX_CPU_THIS_PTR regs.regs_R.regs_R_p[13] =
         &BX_CPU_THIS_PTR regs.regs_R.r13[cpu_mode()];
  BX_CPU_THIS_PTR regs.regs_R.regs_R_p[14] =
         &BX_CPU_THIS_PTR regs.regs_R.r14[cpu_mode()];
  BX_CPU_THIS_PTR regs.regs_R.regs_R_p[15] =
         &BX_CPU_THIS_PTR regs.regs_R.r15;

  for (i=16;i<MD_NUM_IREGS;i++)
    BX_CPU_THIS_PTR regs.regs_R.regs_R_p[i] =
         &BX_CPU_THIS_PTR regs.regs_R.ucodes_R[i-16];

  BX_CPU_THIS_PTR regs.regs_C.spsr =
         &BX_CPU_THIS_PTR regs.regs_C.spsr_modes[cpu_mode()];

}

word_t *
BX_CPU_C::unbanked_regs(int num)
{
   switch (num)
    {
     case 8: return &BX_CPU_THIS_PTR regs.regs_R.r8[0];
     case 9: return &BX_CPU_THIS_PTR regs.regs_R.r9[0];
     case 10: return &BX_CPU_THIS_PTR regs.regs_R.r10[0];
     case 11: return &BX_CPU_THIS_PTR regs.regs_R.r11[0];
     case 12: return &BX_CPU_THIS_PTR regs.regs_R.r12[0];
     case 13: return &BX_CPU_THIS_PTR regs.regs_R.r13[0];
     case 14: return &BX_CPU_THIS_PTR regs.regs_R.r14[0];
     case 15: return &BX_CPU_THIS_PTR regs.regs_R.r15 + 12;
     default : return &BX_CPU_THIS_PTR regs.regs_R.regs_R_unbanked[num];
    }
}

  int
libcpu_LTX_plugin_init(plugin_t *plugin, plugintype_t type, int argc, char *argv[])
{
  pluginCpu = &bx_cpu;
  return(0); // Success
}

