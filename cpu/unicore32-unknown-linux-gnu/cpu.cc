/////////////////////////////////////////////////////////////////////////
// $Id: cpu.cc,v 1.1.1.1 2003/09/25 03:12:53 fht Exp $
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


#if BX_SIM_ID == 0   // only need to define once
// This array defines a look-up table for the even parity-ness
// of an 8bit quantity, for optimal assignment of the parity bit
// in the EFLAGS register
const bx_bool bx_parity_lookup[256] = {
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1
  };
#endif


#if BX_SMP_PROCESSORS==1
// single processor simulation, so there's one of everything
BOCHSAPI BX_CPU_C    bx_cpu;
BOCHSAPI BX_MEM_C    bx_mem;
#else
// multiprocessor simulation, we need an array of cpus and memories
BOCHSAPI BX_CPU_C    *bx_cpu_array[BX_SMP_PROCESSORS];
BOCHSAPI BX_MEM_C    *bx_mem_array[BX_ADDRESS_SPACES];
#endif



// notes:
//
// check limit of CS?

#ifdef REGISTER_IADDR
extern void REGISTER_IADDR(bx_addr addr);
#endif

// The CHECK_MAX_INSTRUCTIONS macro allows cpu_loop to execute a few
// instructions and then return so that the other processors have a chance to
// run.  This is used only when simulating multiple processors.
// 
// If maximum instructions have been executed, return.  A count less
// than zero means run forever.
#define CHECK_MAX_INSTRUCTIONS(count) \
  if (count >= 0) {                   \
    count--; if (count == 0) return;  \
  }

#if BX_SMP_PROCESSORS==1
#  define BX_TICK1_IF_SINGLE_PROCESSOR() BX_TICK1()
#else
#  define BX_TICK1_IF_SINGLE_PROCESSOR()
#endif

// Make code more tidy with a few macros.
#if BX_SUPPORT_X86_64==0
#define RIP EIP
#define RSP ESP
#endif


#include <math.h>
#define SP	29
#define LR	30
/* next program counter */
#define SET_NPC(EXPR)           (BX_CPU_THIS_PTR regs.regs_NPC = (EXPR))
/* current program counter */
#define CPC                    (BX_CPU_THIS_PTR regs.regs_PC)
/* general purpose registers */
#define GPR(N)                  (BX_CPU_THIS_PTR regs.regs_R[N])
#define SET_GPR(N,EXPR)                                                 \
  ((void)(((N) == 31) ? setPC++ : 0), BX_CPU_THIS_PTR regs.regs_R[N] = (EXPR))
#define PSR                     (BX_CPU_THIS_PTR regs.regs_C.cpsr)
#define SET_PSR(EXPR)           (BX_CPU_THIS_PTR regs.regs_C.cpsr = (EXPR))
/* processor status register */
#define PSR                     (BX_CPU_THIS_PTR regs.regs_C.cpsr)
#define SET_PSR(EXPR)           (BX_CPU_THIS_PTR regs.regs_C.cpsr = (EXPR))

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

/* precise architected memory state accessor macros */
#define READ_BYTE(SRC, FAULT)                                           \
	  ((FAULT) = md_fault_none, read_virtual_byte(SRC,&xfer_byte),xfer_byte)
#define READ_HALF(SRC, FAULT)                                           \
	  ((FAULT) = md_fault_none, read_virtual_word(SRC,&xfer_half),xfer_half)
#define READ_WORD(SRC, FAULT)                                           \
	  ((FAULT) = md_fault_none, read_virtual_dword(SRC,&xfer_word),xfer_word)
#ifdef HOST_HAS_QWORD
#define READ_QWORD(SRC, FAULT)                                          \
	  ((FAULT) = md_fault_none, read_virtual_qword(SRC,&xfer_qword),xfer_qword)
#endif /* HOST_HAS_QWORD */

#define WRITE_BYTE(SRC, DST, FAULT)                                     \
	  ((FAULT) = md_fault_none, xfer_byte = (SRC), write_virtual_byte(DST,&xfer_byte))
#define WRITE_HALF(SRC, DST, FAULT)                                     \
	  ((FAULT) = md_fault_none, xfer_half = (SRC), write_virtual_word(DST,&xfer_half))
#define WRITE_WORD(SRC, DST, FAULT)                                     \
	  ((FAULT) = md_fault_none, xfer_word = (SRC),write_virtual_dword(DST,&xfer_word))
#ifdef HOST_HAS_QWORD
#define WRITE_QWORD(SRC, DST, FAULT)                                    \
	  ((FAULT) = md_fault_none, xfer_qword = (SRC),write_virtual_qword(DST,&xfer_qword))
#endif /* HOST_HAS_QWORD */
	

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
  Bit8u blank_char=' ',char_mask=0x7;
  char * path;

#if 0  
#if BX_INSTRUMENTATION
  if (setjmp( BX_CPU_THIS_PTR jmp_buf_env )) 
  { 
    // only from exception function can we get here ...
    BX_INSTR_NEW_INSTRUCTION(CPU_ID);
  }
#else
  (void) setjmp( BX_CPU_THIS_PTR jmp_buf_env );
#endif

  // We get here either by a normal function call, or by a longjmp
  // back from an exception() call.  In either case, commit the
  // new EIP/ESP, and set up other environmental fields.  This code
  // mirrors similar code below, after the interrupt() call.
  BX_CPU_THIS_PTR prev_eip = RIP; // commit new EIP
  BX_CPU_THIS_PTR prev_esp = RSP; // commit new ESP
  BX_CPU_THIS_PTR EXT = 0;
  BX_CPU_THIS_PTR errorno = 0;
#endif
  /* added for debug 2003-07-15 */
  path = bx_options.rom.Opath->getptr ();
  romstart = bx_options.rom.Oaddress->get ();

/* added for debugging instructions 2003-07-14*/
  BX_CPU_THIS_PTR regs.regs_PC  = romstart;
  BX_CPU_THIS_PTR regs.regs_R[SP]  = 0x070000;
  BX_CPU_THIS_PTR regs.regs_NPC = romstart + sizeof(md_inst_t);

  fd  = open(path,O_RDONLY);
  ret = fstat(fd, &stat_buf);

  if (ret) {
    BX_PANIC(( "ROM: couldn't stat ROM image file '%s'.", path));
    return;
    }
  romsize = stat_buf.st_size; 

/*
  for (i = 0; i < 80 * 25 * 2; i += 2) {
      write_virtual_byte(0xB8000+i,&blank_char); 
      write_virtual_byte(0xB8000+i+1,&char_mask);	
  } 
*/
  
  while (1) {
#if 0
  // First check on events which occurred for previous instructions
  // (traps) and ones which are asynchronous to the CPU
  // (hardware interrupts).
  if (BX_CPU_THIS_PTR async_event) {
    if (handleAsyncEvent()) {
      // If request to return to caller ASAP.
      return;
      }
    }

#if BX_DEBUGGER
  {
  Bit32u debug_eip = BX_CPU_THIS_PTR prev_eip;
  if ( dbg_is_begin_instr_bpoint(
         BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value,
         debug_eip,
         BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.base + debug_eip,
         BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.d_b) ) {
    return;
    }
  }
#endif  // #if BX_DEBUGGER

#if BX_EXTERNAL_DEBUGGER
  if (regs.debug_state != debug_run) {
    bx_external_debugger(this);
  }
#endif

#endif 
  fault = md_fault_none;

  if (!in_flow){
 
     /* get the next instruction to execute */
     read_virtual_dword(BX_CPU_THIS_PTR regs.regs_PC,&inst);
  
     //fprintf(stderr,"		pc %8x  inst %8x\n",BX_CPU_THIS_PTR regs.regs_PC	    ,inst);   

     /* decode the instruction */
     MD_SET_OPCODE(op, inst);

//     md_print_uop(op,inst,BX_CPU_THIS_PTR regs.regs_PC,stderr);
     
     if (MD_OP_FLAGS(op) & F_CISC)
     {
     /* get instruction flow */
     nflow = md_get_flow(op, inst, flowtab);
     if (nflow > 0)
     {
       in_flow = TRUE;
       flow_index = 0;
     }
     else BX_CPU_THIS_PTR panic("could not locate CISC flow");
     }
  }

  if (in_flow)
   {
     op = flowtab[flow_index].op;
     inst = flowtab[flow_index++].inst;
     if (flow_index == nflow)     in_flow = FALSE;
   }

  if (op == NA)
       BX_CPU_THIS_PTR panic("bogus opcode detected @ 0x%08p", BX_CPU_THIS_PTR regs.regs_PC);
  if (MD_OP_FLAGS(op) & F_CISC)
       BX_CPU_THIS_PTR panic("CISC opcode decoded");

  setPC = 0;
  BX_CPU_THIS_PTR regs.regs_R[MD_REG_PC]= BX_CPU_THIS_PTR regs.regs_PC;

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

   BX_TICK1_IF_SINGLE_PROCESSOR();
   //md_print_uop(op,inst,BX_CPU_THIS_PTR regs.regs_PC,stderr);
   
   if (fault != md_fault_none)
       BX_CPU_THIS_PTR panic("fault (%d) detected @ 0x%08p", fault, BX_CPU_THIS_PTR regs.regs_PC);

   if (setPC != 0/* regs.regs_R[MD_REG_PC] != regs.regs_PC */)
        BX_CPU_THIS_PTR regs.regs_NPC = BX_CPU_THIS_PTR regs.regs_R[MD_REG_PC];
   
   if (!in_flow)
        {
          BX_CPU_THIS_PTR regs.regs_PC = BX_CPU_THIS_PTR regs.regs_NPC;
          BX_CPU_THIS_PTR regs.regs_NPC += sizeof(md_inst_t);
    }
    //loading kernels 2003--7-23
    //if (BX_CPU_THIS_PTR regs.regs_NPC > romstart+romsize) break;
  }  // while (1)
  md_print_iregs(BX_CPU_THIS_PTR regs.regs_R,stderr);
  md_print_cregs(BX_CPU_THIS_PTR regs.regs_C,stderr);
}

  unsigned
BX_CPU_C::handleAsyncEvent(void)
{
  //
  // This area is where we process special conditions and events.
  //

  if (BX_CPU_THIS_PTR debug_trap & 0x80000000) {
    // I made up the bitmask above to mean HALT state.
#if BX_SMP_PROCESSORS==1
    BX_CPU_THIS_PTR debug_trap = 0; // clear traps for after resume
    BX_CPU_THIS_PTR inhibit_mask = 0; // clear inhibits for after resume
    // for one processor, pass the time as quickly as possible until
    // an interrupt wakes up the CPU.
#if BX_DEBUGGER
    while (bx_guard.interrupt_requested != 1)
#else
    while (1)
#endif
      {
      if (BX_CPU_INTR && BX_CPU_THIS_PTR get_IF ()) {
        break;
        }
      if (BX_CPU_THIS_PTR async_event == 2) {
        BX_INFO(("decode: reset detected in halt state"));
        break;
        }
      BX_TICK1();
      }
#else      /* BX_SMP_PROCESSORS != 1 */
    // for multiprocessor simulation, even if this CPU is halted we still
    // must give the others a chance to simulate.  If an interrupt has 
    // arrived, then clear the HALT condition; otherwise just return from
    // the CPU loop with stop_reason STOP_CPU_HALTED.
    if (BX_CPU_INTR && BX_CPU_THIS_PTR get_IF ()) {
      // interrupt ends the HALT condition
      BX_CPU_THIS_PTR debug_trap = 0; // clear traps for after resume
      BX_CPU_THIS_PTR inhibit_mask = 0; // clear inhibits for after resume
      //bx_printf ("halt condition has been cleared in %s", name);
    } else {
      // HALT condition remains, return so other CPUs have a chance
#if BX_DEBUGGER
      BX_CPU_THIS_PTR stop_reason = STOP_CPU_HALTED;
#endif
      return 1; // Return to caller of cpu_loop.
    }
#endif
  } else if (BX_CPU_THIS_PTR kill_bochs_request) {
    // setting kill_bochs_request causes the cpu loop to return ASAP.
    return 1; // Return to caller of cpu_loop.
  }


  // Priority 1: Hardware Reset and Machine Checks
  //   RESET
  //   Machine Check
  // (bochs doesn't support these)

  // Priority 2: Trap on Task Switch
  //   T flag in TSS is set
  if (BX_CPU_THIS_PTR debug_trap & 0x00008000) {
    BX_CPU_THIS_PTR dr6 |= BX_CPU_THIS_PTR debug_trap;
    /* added for linking success 2003-07-02 */
    //    exception(BX_DB_EXCEPTION, 0, 0); // no error, not interrupt
    }

  // Priority 3: External Hardware Interventions
  //   FLUSH
  //   STOPCLK
  //   SMI
  //   INIT
  // (bochs doesn't support these)

  // Priority 4: Traps on Previous Instruction
  //   Breakpoints
  //   Debug Trap Exceptions (TF flag set or data/IO breakpoint)
  if ( BX_CPU_THIS_PTR debug_trap &&
       !(BX_CPU_THIS_PTR inhibit_mask & BX_INHIBIT_DEBUG) ) {
    // A trap may be inhibited on this boundary due to an instruction
    // which loaded SS.  If so we clear the inhibit_mask below
    // and don't execute this code until the next boundary.
    // Commit debug events to DR6
    BX_CPU_THIS_PTR dr6 |= BX_CPU_THIS_PTR debug_trap;
    /* added for linking success 2003-07-02 */
    //exception(BX_DB_EXCEPTION, 0, 0); // no error, not interrupt
    }

  // Priority 5: External Interrupts
  //   NMI Interrupts
  //   Maskable Hardware Interrupts
  if (BX_CPU_THIS_PTR inhibit_mask & BX_INHIBIT_INTERRUPTS) {
    // Processing external interrupts is inhibited on this
    // boundary because of certain instructions like STI.
    // inhibit_mask is cleared below, in which case we will have
    // an opportunity to check interrupts on the next instruction
    // boundary.
    }
  else if (BX_CPU_INTR && BX_CPU_THIS_PTR get_IF () &&
           BX_DBG_ASYNC_INTR) {
    Bit8u vector;

    // NOTE: similar code in ::take_irq()
#if BX_SUPPORT_APIC
    if (BX_CPU_THIS_PTR local_apic.INTR)
      vector = BX_CPU_THIS_PTR local_apic.acknowledge_int ();
    else
      vector = DEV_pic_iac(); // may set INTR with next interrupt
#else
    // if no local APIC, always acknowledge the PIC.
    vector = DEV_pic_iac(); // may set INTR with next interrupt
#endif
    //BX_DEBUG(("decode: interrupt %u",
    //                                   (unsigned) vector));
    BX_CPU_THIS_PTR errorno = 0;
    BX_CPU_THIS_PTR EXT   = 1; /* external event */
    /* added for linking success 2003-07-02 */
//    interrupt(vector, 0, 0, 0);
    BX_INSTR_HWINTERRUPT(CPU_ID, vector,
        BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value, EIP);
    // Set up environment, as would be when this main cpu loop gets
    // invoked.  At the end of normal instructions, we always commmit
    // the new EIP/ESP values.  But here, we call interrupt() much like
    // it was a sofware interrupt instruction, and need to effect the
    // commit here.  This code mirrors similar code above.
    BX_CPU_THIS_PTR prev_eip = RIP; // commit new RIP
    BX_CPU_THIS_PTR prev_esp = RSP; // commit new RSP
    BX_CPU_THIS_PTR EXT = 0;
    BX_CPU_THIS_PTR errorno = 0;
    }
  else if (BX_HRQ && BX_DBG_ASYNC_DMA) {
    // NOTE: similar code in ::take_dma()
    // assert Hold Acknowledge (HLDA) and go into a bus hold state
    DEV_dma_raise_hlda();
    }

  // Priority 6: Faults from fetching next instruction
  //   Code breakpoint fault
  //   Code segment limit violation (priority 7 on 486/Pentium)
  //   Code page fault (priority 7 on 486/Pentium)
  // (handled in main decode loop)

  // Priority 7: Faults from decoding next instruction
  //   Instruction length > 15 bytes
  //   Illegal opcode
  //   Coprocessor not available
  // (handled in main decode loop etc)

  // Priority 8: Faults on executing an instruction
  //   Floating point execution
  //   Overflow
  //   Bound error
  //   Invalid TSS
  //   Segment not present
  //   Stack fault
  //   General protection
  //   Data page fault
  //   Alignment check
  // (handled by rest of the code)


  if (BX_CPU_THIS_PTR get_TF ()) {
    // TF is set before execution of next instruction.  Schedule
    // a debug trap (#DB) after execution.  After completion of
    // next instruction, the code above will invoke the trap.
    BX_CPU_THIS_PTR debug_trap |= 0x00004000; // BS flag in DR6
    }

  // Now we can handle things which are synchronous to instruction
  // execution.
  if (BX_CPU_THIS_PTR get_RF ()) {
    BX_CPU_THIS_PTR clear_RF ();
    }
#if BX_X86_DEBUGGER
  else {
    // only bother comparing if any breakpoints enabled
    if ( BX_CPU_THIS_PTR dr7 & 0x000000ff ) {
      Bit32u iaddr =
        BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.base +
        BX_CPU_THIS_PTR prev_eip;
      Bit32u dr6_bits;
      if ( (dr6_bits = hwdebug_compare(iaddr, 1, BX_HWDebugInstruction,
                                       BX_HWDebugInstruction)) ) {
        // Add to the list of debug events thus far.
        BX_CPU_THIS_PTR debug_trap |= dr6_bits;
        BX_CPU_THIS_PTR async_event = 1;
        // If debug events are not inhibited on this boundary,
        // fire off a debug fault.  Otherwise handle it on the next
        // boundary. (becomes a trap)
        if ( !(BX_CPU_THIS_PTR inhibit_mask & BX_INHIBIT_DEBUG) ) {
          // Commit debug events to DR6
          BX_CPU_THIS_PTR dr6 = BX_CPU_THIS_PTR debug_trap;
          exception(BX_DB_EXCEPTION, 0, 0); // no error, not interrupt
          }
        }
      }
    }
#endif

  // We have ignored processing of external interrupts and
  // debug events on this boundary.  Reset the mask so they
  // will be processed on the next boundary.
  BX_CPU_THIS_PTR inhibit_mask = 0;

  if ( !(BX_CPU_INTR ||
         BX_CPU_THIS_PTR debug_trap ||
         BX_HRQ ||
         BX_CPU_THIS_PTR get_TF ()) )
    BX_CPU_THIS_PTR async_event = 0;

  return 0; // Continue executing cpu_loop.
}




// boundaries of consideration:
//
//  * physical memory boundary: 1024k (1Megabyte) (increments of...)
//  * A20 boundary:             1024k (1Megabyte)
//  * page boundary:            4k
//  * ROM boundary:             2k (dont care since we are only reading)
//  * segment boundary:         any



  void
BX_CPU_C::prefetch(void)
{
  // cs:eIP
  // prefetch QSIZE byte quantity aligned on corresponding boundary
  bx_address laddr;
  Bit32u pAddr;
  bx_address temp_rip;
  Bit32u temp_limit;
  bx_address laddrPageOffset0, eipPageOffset0;

  temp_rip   = RIP;
  temp_limit = BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled;

  laddr = BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.base +
                    temp_rip;

  if (((Bit32u)temp_rip) > temp_limit) {
    BX_PANIC(("prefetch: RIP > CS.limit"));
    }

#if BX_SUPPORT_PAGING
  if (BX_CPU_THIS_PTR cr0.pg) {
    // aligned block guaranteed to be all in one page, same A20 address
    pAddr = itranslate_linear(laddr, CPL==3);
    pAddr = A20ADDR(pAddr);
    }
  else
#endif // BX_SUPPORT_PAGING
    {
    pAddr = A20ADDR(laddr);
    }

  // check if segment boundary comes into play
  //if ((temp_limit - (Bit32u)temp_rip) < 4096) {
  //  }

  // Linear address at the beginning of the page.
  laddrPageOffset0 = laddr & 0xfffff000;
  // Calculate RIP at the beginning of the page.
  eipPageOffset0 = RIP - (laddr - laddrPageOffset0);
  BX_CPU_THIS_PTR eipPageBias = - eipPageOffset0;
  BX_CPU_THIS_PTR eipPageWindowSize = 4096; // FIXME:
  BX_CPU_THIS_PTR pAddrA20Page = pAddr & 0xfffff000;
  BX_CPU_THIS_PTR eipFetchPtr =
      BX_CPU_THIS_PTR mem->getHostMemAddr(this, BX_CPU_THIS_PTR pAddrA20Page,
                                          BX_READ);

  // Sanity checks
  if ( !BX_CPU_THIS_PTR eipFetchPtr ) {
    if ( pAddr >= BX_CPU_THIS_PTR mem->len ) {
      BX_PANIC(("prefetch: running in bogus memory"));
      }
    else {
      BX_PANIC(("prefetch: getHostMemAddr vetoed direct read, pAddr=0x%x.",
                pAddr));
      }
    }
}


  void
BX_CPU_C::boundaryFetch(bxInstruction_c *i)
{
    unsigned j;
    Bit8u fetchBuffer[16]; // Really only need 15
    bx_address eipBiased, remainingInPage;
    Bit8u *fetchPtr;
    unsigned ret;

    eipBiased = RIP + BX_CPU_THIS_PTR eipPageBias;
    remainingInPage = (BX_CPU_THIS_PTR eipPageWindowSize - eipBiased);
    if (remainingInPage > 15) {
      BX_PANIC(("fetch_decode: remaining > max ilen"));
      }
    fetchPtr = BX_CPU_THIS_PTR eipFetchPtr + eipBiased;

    // Read all leftover bytes in current page up to boundary.
    for (j=0; j<remainingInPage; j++) {
      fetchBuffer[j] = *fetchPtr++;
      }

    // The 2nd chunk of the instruction is on the next page.
    // Set RIP to the 0th byte of the 2nd page, and force a
    // prefetch so direct access of that physical page is possible, and
    // all the associated info is updated.
    RIP += remainingInPage;
    prefetch();
    if (BX_CPU_THIS_PTR eipPageWindowSize < 15) {
      BX_PANIC(("fetch_decode: small window size after prefetch"));
      }

    // We can fetch straight from the 0th byte, which is eipFetchPtr;
    fetchPtr = BX_CPU_THIS_PTR eipFetchPtr;

    // read leftover bytes in next page
    for (; j<15; j++) {
      fetchBuffer[j] = *fetchPtr++;
      }
#if BX_SUPPORT_X86_64
    if (BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_64) {
      ret = fetchDecode64(fetchBuffer, i, 15);
      }
    else
#endif
      {
/* added for linking success 2003-07-02 */
       //  ret = fetchDecode(fetchBuffer, i, 15);
      }
    // Restore EIP since we fudged it to start at the 2nd page boundary.
    RIP = BX_CPU_THIS_PTR prev_eip;
    if (ret==0)
      BX_PANIC(("fetchDecode: cross boundary: ret==0"));

// Since we cross an instruction boundary, note that we need a prefetch()
// again on the next instruction.  Perhaps we can optimize this to
// eliminate the extra prefetch() since we do it above, but have to
// think about repeated instructions, etc.
BX_CPU_THIS_PTR eipPageWindowSize = 0; // Fixme

  BX_INSTR_OPCODE(CPU_ID, fetchBuffer, i->ilen(),
                  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.d_b);
}


#if BX_EXTERNAL_DEBUGGER

  void
BX_CPU_C::ask (int level, const char *prefix, const char *fmt, va_list ap)
{
  char buf1[1024];
  vsprintf (buf1, fmt, ap);
  printf ("%s %s\n", prefix, buf1);
  trap_debugger(1);
  //this->logfunctions::ask(level,prefix,fmt,ap);
}

  void
BX_CPU_C::trap_debugger (bx_bool callnow)
{
  regs.debug_state = debug_step;
  if (callnow) {
    bx_external_debugger(this);
  }
}

#endif  // #if BX_EXTERNAL_DEBUGGER


#if BX_DEBUGGER
extern unsigned int dbg_show_mask;

  bx_bool
BX_CPU_C::dbg_is_begin_instr_bpoint(Bit32u cs, Bit32u eip, Bit32u laddr,
                                    Bit32u is_32)
{
  //fprintf (stderr, "begin_instr_bp: checking cs:eip %04x:%08x\n", cs, eip);
  BX_CPU_THIS_PTR guard_found.cs  = cs;
  BX_CPU_THIS_PTR guard_found.eip = eip;
  BX_CPU_THIS_PTR guard_found.laddr = laddr;
  BX_CPU_THIS_PTR guard_found.is_32bit_code = is_32;

  // BW mode switch breakpoint
  // instruction which generate exceptions never reach the end of the
  // loop due to a long jump. Thats why we check at start of instr.
  // Downside is that we show the instruction about to be executed
  // (not the one generating the mode switch).
  if (BX_CPU_THIS_PTR mode_break && 
      (BX_CPU_THIS_PTR debug_vm != BX_CPU_THIS_PTR getB_VM ())) {
    BX_INFO(("Caught vm mode switch breakpoint"));
    BX_CPU_THIS_PTR debug_vm = BX_CPU_THIS_PTR getB_VM ();
    BX_CPU_THIS_PTR stop_reason = STOP_MODE_BREAK_POINT;
    return 1;
  }

  if( (BX_CPU_THIS_PTR show_flag) & (dbg_show_mask)) {
    int rv;
    if((rv = bx_dbg_symbolic_output()))
      return rv;
  }

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
      dbg_xlate_linear2phy(BX_CPU_THIS_PTR guard_found.laddr,
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
BX_CPU_C::dbg_is_end_instr_bpoint(Bit32u cs, Bit32u eip, Bit32u laddr,
                                  Bit32u is_32)
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
      BX_CPU_THIS_PTR guard_found.cs  = cs;
      BX_CPU_THIS_PTR guard_found.eip = eip;
      BX_CPU_THIS_PTR guard_found.laddr = laddr;
      BX_CPU_THIS_PTR guard_found.is_32bit_code = is_32;
      BX_CPU_THIS_PTR guard_found.guard_found = BX_DBG_GUARD_ICOUNT;
      return(1);
      }
    }

#if (BX_NUM_SIMULATORS >= 2)
  // if async event pending, acknowlege them
  if (bx_guard.async_changes_pending.which) {
    if (bx_guard.async_changes_pending.which & BX_DBG_ASYNC_PENDING_A20)
      bx_dbg_async_pin_ack(BX_DBG_ASYNC_PENDING_A20,
                           bx_guard.async_changes_pending.a20);
    if (bx_guard.async_changes_pending.which) {
      BX_PANIC(("decode: async pending unrecognized."));
      }
    }
#endif
  return(0); // no breakpoint
}


  void
BX_CPU_C::dbg_take_irq(void)
{
  unsigned vector;

  // NOTE: similar code in ::cpu_loop()

  if ( BX_CPU_INTR && BX_CPU_THIS_PTR get_IF () ) {
    if ( setjmp(BX_CPU_THIS_PTR jmp_buf_env) == 0 ) {
      // normal return from setjmp setup
      vector = DEV_pic_iac(); // may set INTR with next interrupt
      BX_CPU_THIS_PTR errorno = 0;
      BX_CPU_THIS_PTR EXT   = 1; // external event
      BX_CPU_THIS_PTR async_event = 1; // set in case INTR is triggered
      interrupt(vector, 0, 0, 0);
      }
    }
}

  void
BX_CPU_C::dbg_force_interrupt(unsigned vector)
{
  // Used to force slave simulator to take an interrupt, without
  // regard to IF

  if ( setjmp(BX_CPU_THIS_PTR jmp_buf_env) == 0 ) {
    // normal return from setjmp setup
    BX_CPU_THIS_PTR errorno = 0;
    BX_CPU_THIS_PTR EXT   = 1; // external event
    BX_CPU_THIS_PTR async_event = 1; // probably don't need this
    interrupt(vector, 0, 0, 0);
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

static char *md_shift_type[4] = { "lsl", "lsr", "asr", "ror" };
static char *md_pu_code[4] = { "da", "ia", "db", "ib" };
int print_urd = 0;

void BX_CPU_C::md_print_ifmt(char *fmt,
	      md_inst_t inst,		/* instruction to disassemble */
	      md_addr_t pc,		/* addr of inst, used for PC-rels */
	      FILE *stream)
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
	    case 'd':
	      print_urd = 1;
	      fprintf(stream, "r%d", RD);
	      break;

	    case 'v':
	      print_urd = 1;
	      if (URD < 32)
		fprintf(stream, "r%d", URD);
	      else
		fprintf(stream, "tmp%d", URD-32);
	      break;

	    case 'n':
	      fprintf(stream, "r%d", RN);
	      break;
	   
	    case 'u':
	      if (URN < 32)
		fprintf(stream, "r%d", URN);
	      else
		fprintf(stream, "tmp%d", URN-32);
	      break;

	    case 's':
	      fprintf(stream, "r%d", RS);
	      break;
	    case 'w':
	      fprintf(stream, "r%d", RM);
	      break;
	    case 'm':
	      if (!SHIFT_REG && !SHIFT_SHAMT)
		fprintf(stream, "r%d", RM);
	      else if (SHIFT_REG && !SHIFT_REG_PAD)
		fprintf(stream,
			"r%d, %s r%d", RM, md_shift_type[SHIFT_TYPE], RS);
	      else if (SHIFT_REG && SHIFT_REG_PAD)
		fprintf(stream, "%s r%d (invalid pad!!!)",
			md_shift_type[SHIFT_TYPE], RS);
	      else
		fprintf(stream,
			"r%d, %s #%d", RM, md_shift_type[SHIFT_TYPE], SHIFT_SHAMT);
	      break;

	    case 'D':
	      fprintf(stream, "f%d", FD);
	      break;
	    case 'N':
	      fprintf(stream, "f%d", FN);
	      break;
	    case 'M':
	      fprintf(stream, "f%d", FM);
	      break;

	    case 'i':
	      fprintf(stream, "%d (%d >>> %d)",
		      ROTR(ROTIMM, ROTAMT), ROTIMM, ROTAMT);
	      break;
	    case 'o':
	      fprintf(stream, "%d", OFS);
	      break;
	    case 'h':
	      fprintf(stream, "%d", HOFS);
	      break;
	    case 'O':
	      fprintf(stream, "%d", FPOFS);
	      break;

	    case 'a':
	      fprintf(stream, "%s", md_pu_code[LDST_PU]);
	      break;
	    case 'R':
	      {
		int first = TRUE;

		fprintf(stream, "{");
		for (i=0; i < 16; i++)
		  {
		    if (REGLIST & (1 << i))
		      {
			if (!first)
			  fprintf(stream, ",");
			fprintf(stream, "r%d", i);
			first = FALSE;
		      }
		  }
		fprintf(stream, "}");
	      }
	      break;
	    case 'C':
	      fprintf(stream, "%d", FCNT ? FCNT : 4);
	      break;

	    case 'j':
	      fprintf(stream, "0x%p", pc + BOFS + 4);
	      break;

	    case 'S':
	      fprintf(stream, "0x%08x", SYSCODE);
	      break;
	    case 'P':
	      fprintf(stream, "%d", RS);
	      break;
	    case 'p':
	      fprintf(stream, "%d", CPOPC);
	      break;
	    case 'g':
	      fprintf(stream, "%d", CPEXT);
	      break;

	    default:
	      BX_CPU_THIS_PTR panic("unknown disassembler escape code `%c'", *s);
	    }
	}
      else
	fputc(*s, stream);
      s++;
    }
}

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
            md_reg_name(rt_gpr, reg), regs[reg], 
			regs[reg]);
}

void
BX_CPU_C::md_print_iregs(md_gpr_t regs, FILE *stream)
{
  int i;

  for (i=0; i < 32; i += 2)
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
      fprintf(stream, "SPSR: 0x%08x", regs.spsr);
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

/* disassemble an Arm instruction */
void
BX_CPU_C::md_print_uop(enum md_opcode op,
	      md_inst_t inst,		/* instruction to disassemble */
	      md_addr_t pc,		/* addr of inst, used for PC-rels */
	      FILE *stream)		/* output stream */
{
  /* use stderr as default output stream */
  if (!stream)
    stream = stderr;

  /* disassemble the instruction */
  if (op <= OP_NA || op >= OP_MAX) 
    {
      /* bogus instruction */
		fprintf(stream, "<invalid inst: 0x%08x>, op = %d", inst, op);
    }
  else if (((inst >> 29) == 0x4) && ((inst >> 8) & 0x1)) {
  		fprintf(stream, "<undefined>");
	 }
  else
    {
      md_print_ifmt(MD_OP_NAME(op), inst, pc, stream);
      fprintf(stream, "  ");
      md_print_ifmt(MD_OP_FORMAT(op), inst, pc, stream);
      fprintf(stream, "  ");
      if (print_urd)
	{
	  if (URD < 32)
	     fprintf(stream,"r%d=%x", URD,BX_CPU_THIS_PTR regs.regs_R[URD]);
	  else
	    fprintf(stream, "tmp%d=%x", URD-32,BX_CPU_THIS_PTR regs.regs_R[URD]);
	}
      fprintf(stream,"\n");
    }
}

