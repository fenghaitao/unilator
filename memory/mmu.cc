/////////////////////////////////////////////////////////////////////////
// $Id: paging.cc,v 1.9 2003/11/12 08:41:05 fht Exp $
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
//  but WITHOUT ANY WARRANTY; without even the immodeied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temmodee Place, Suite 330, Boston, MA  02111-1307 USA


// Notes from merge of x86-64 enhancements: (KPL)
//   Looks like for x86-64/PAE=1/PTE with PSE=1, the
//     CR4.PSE field is not consulted by the processor?
//   Fix the PAE case to not update the page table tree entries
//     until the final protection check?  This is how it is on
//     P6 for non-PAE anyways...



#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#define LOG_THIS BX_MMU_THIS_PTR

#if BX_USE_CPU_SMF
#define BX_MMU_THIS_PTR  (&bx_mmu)->
#define this (BX_CPU(0))
#endif

#if BX_SUPPORT_PAGING

bx_mmu_c bx_mmu;

#define WriteUserOK       0x08
#define WriteSysOK        0x04
#define ReadUserOK        0x02
#define ReadSysOK         0x01


// Translate a linear address to a physical address, for
// a data access (D)
/* processor status register */
#define PSR                     (BX_CPU_THIS_PTR regs.regs_C.cpsr)
#define SET_PSR(EXPR)           (BX_CPU_THIS_PTR regs.regs_C.cpsr = (EXPR))

#define SPSR                     (*BX_CPU_THIS_PTR regs.regs_C.spsr)
#define SET_SPSR(EXPR,spsr_mode)  (BX_CPU_THIS_PTR regs.regs_C.spsr_modes[spsr_mode] = (EXPR))

#define LR	14
#define CPC                     (BX_CPU_THIS_PTR regs.regs_PC)
#define SET_NPC(EXPR)           (BX_CPU_THIS_PTR regs.regs_NPC = (EXPR))

#define CPSR			BX_CPU_THIS_PTR regs.regs_C.cpsr

  Bit32u
bx_mmu_c::check_permission(Bit8u access_AP,Bit8u access_RS,bx_bool isWrite)
{

  switch (access_AP)
    {
    case 0 : 
      switch (access_RS)
	{
	case 0:
	  return 1;
	case 1: if (isWrite){
	  return 1;
	  }
	break;
	case 2: if (isWrite){
	  return 1;
	  }else if (! InAPrivilegedMode){
	  return 1;
	  }
	break;
	case 3: BX_PANIC(("Unpredictable access permissions"));
	}
    case 1 : 
      if (InAPrivilegedMode) break;
      else {
	  return 1;
      }
    case 2 :
      if (InAPrivilegedMode) break;
      else if (isWrite){
	return 1;
      }
      break;
    case 3 : ;        
    }
    return 0;
}

  Bit32u
bx_mmu_c::dtranslate_linear(bx_address laddr, unsigned mode, unsigned rw)
{
  bx_address   lpf;
  Bit32u   ppf, poffset, TLB_index, error_code, paddress;
  Bit32u   pde, pde_addr;
  bx_bool  isWrite;
  Bit32u   accessBits, combined_access;
  unsigned priv_index;
  Bit32u   pte, pte_addr;

  Bit8u	   access_AP,access_RS;
  int	   i;

  lpf       = laddr & 0xfffff000; // linear page frame

  isWrite = (rw>=BX_WRITE); // write or r-m-w

  // Get page dir entry
  pde_addr = (SYS_MMUTTB_REG & 0xffffc000) |
             (( (laddr & 0xfff00000) >> 20)<<2);

  BX_MMU_THIS_PTR mem->readPhysicalPage(this, pde_addr, 4, &pde);
//  fprintf(stderr,"SYS_MMUTTB_REG=%8x\n",SYS_MMUTTB_REG);
//  fprintf(stderr,"pde=%8x\n",pde);
  
  switch (pde &0x03)
  {
     case 0:
    	{	// Page Directory Entry NOT present
//	fprintf(stderr,"data unmapped first level page \n");
	SYS_FSR_REG = 0x05;
	goto page_fault_not_present;
	}
     case 1:
	{
	pte_addr = (pde & 0xfffffc00) |((laddr & 0x000ff000) >> 10);
	BX_MMU_THIS_PTR mem->readPhysicalPage(this, pte_addr, 4, &pte);
	switch (pte &0x03)
	{
	   case 0 : 
//	     fprintf(stderr,"data unmapped seconde level page \n");
	     SYS_FSR_REG = 0x07;
	     goto page_fault_not_present;
	   case 1 : 
	     BX_PANIC(("large page not supported pc is %8x",CPC));
	     access_RS = (SYS_CONTROL_REG >>8) & 0x3;
  	     poffset   = (laddr>>14) &0x3; // physical offset
	     access_AP = (pte >>(4+2*poffset)) & 0x3;
	     if (check_permission(access_AP,access_RS,isWrite)) {
	        paddress = (pte&0xffff0000) | (laddr & 0x0000ffff);	
	        BX_PANIC(("large page data checkpemission failed pc is %8x",CPC));
                SYS_FSR_REG = 0x0f;
  	        goto page_fault_access;
	 	}	
	     paddress = (pte&0xffff0000) | (laddr & 0x0000ffff);	
	     break;
	   case 2 :
	     access_RS = (SYS_CONTROL_REG >>8) & 0x3;
  	     poffset   = (laddr>>10) &0x3; // physical offset
	     access_AP = (pte >>(4+2*poffset)) & 0x3;
	     if (check_permission(access_AP,access_RS,isWrite)) {
	         paddress = (pte&0xfffff000) | (laddr & 0x00000fff);	
//	         BX_PANIC(("small page data checkpemission failed pc is %8x laddr is %8x",CPC,paddress));
                   SYS_FSR_REG = 0x0f;
  	           goto page_fault_access;
	     }
	     paddress = (pte&0xfffff000) | (laddr & 0x00000fff);	
	     break;
	   case 3 : BX_PANIC(("tiny page not supported"));;        
	}
	break;
	}
     case 2:
	access_AP = (pde >> 10) &0x03;
	access_RS = (SYS_CONTROL_REG >>8) &0x03;
        if (check_permission(access_AP,access_RS,isWrite)) {
		paddress = (pde&0xfff00000) | (laddr & 0x000fffff);	
		BX_PANIC(("section data checkpemission failed pc is %8x cpsr is %8x laddr is %8x,paddress is %8x",CPC,CPSR,laddr,paddress));
    		SYS_FSR_REG = 0x0d;
		goto page_fault_access;
	}	
	paddress = (pde&0xfff00000) | (laddr & 0x000fffff);	
	break;
     case 3:
	BX_PANIC(("tiny page not supported"));
	//pte_addr = (pde & 0xfffff000) |((laddr & 0x003ff000) >> 10);
	//BX_MMU_THIS_PTR mem->readPhysicalPage(this, pte_addr, 4, &pte);

	goto page_fault_not_present;
  }

  return(paddress);


page_fault_access:
page_fault_not_present:

    SYS_FAR_REG = laddr;
    SET_SPSR(PSR,md_cpu_abt);                                       
//    BX_INFO(("data CPSR is %2x,CPC is %2x",PSR,CPC));
    SET_PSR((PSR & 0xffffff40)|0x97 );
    BX_CPU_THIS_PTR switch_mode();
    (*(BX_CPU_THIS_PTR regs.regs_R.regs_R_p[LR])) = CPC + 8;
    SET_NPC(0x010);                                                  

   /* added for linking success 2003-07-02 */
  //exception(BX_PF_EXCEPTION, error_code, 0);
//  BX_PANIC(("data page fault for address  %8x,PC is %8x,CPSR is %8x\n",laddr,BX_MMU_THIS_PTR regs.regs_PC,BX_MMU_THIS_PTR regs.regs_C.cpsr));
  return(0); // keep compiler happy
}


// Translate a linear address to a physical address, for
// an instruction fetch access (I)

  Bit32u
bx_mmu_c::itranslate_linear(bx_address laddr, unsigned mode,unsigned rw)
{
  bx_address   lpf;
  Bit32u   ppf, poffset, TLB_index, error_code, paddress;
  Bit32u   pde, pde_addr;
  bx_bool  isWrite;
  Bit32u   accessBits, combined_access;
  unsigned priv_index;
  Bit32u   pte, pte_addr;

  Bit8u	   access_AP,access_RS;
  int	   i;

  lpf       = laddr & 0xfffff000; // linear page frame
  poffset   = laddr & 0x00000fff; // physical offset

  isWrite = (rw>=BX_WRITE); // write or r-m-w

  // Get page dir entry
  pde_addr = (SYS_MMUTTB_REG & 0xffffc000) |
             (( (laddr & 0xfff00000) >> 20)<<2);

  BX_MMU_THIS_PTR mem->readPhysicalPage(this, pde_addr, 4, &pde);
//  fprintf(stderr,"SYS_MMUTTB_REG=%8x\n",SYS_MMUTTB_REG);
//  fprintf(stderr,"pde=%8x\n",pde);
  
  switch (pde &0x03)
  {
     case 0:
    	{	// Page Directory Entry NOT present
//	BX_PANIC(("prefetch unmapped first level page pc is %8x\n",CPC));
	SYS_FSR_REG = 0x05;
	goto page_fault_not_present;
	}
     case 1:
	{
	pte_addr = (pde & 0xfffffc00) |((laddr & 0x000ff000) >> 10);
	BX_MMU_THIS_PTR mem->readPhysicalPage(this, pte_addr, 4, &pte);
	switch (pte &0x03)
	{
	   case 0 : 
//	     fprintf(stderr,"prefetch unmapped second level page \n");
	     SYS_FSR_REG = 0x07;
	     goto page_fault_not_present;
	   case 1 : 
	     BX_PANIC(("large page not supported pc is %8x",CPC));
	     access_RS = (SYS_CONTROL_REG >>8) & 0x3;
  	     poffset   = (laddr>>14) &0x3; // physical offset
	     access_AP = (pte >>(4+2*poffset)) & 0x3;
	     if (check_permission(access_AP,access_RS,isWrite)) {
	     	   paddress = (pte&0xffff0000) | (laddr & 0x0000ffff);	
	           BX_PANIC(("itranslate second page pc is %8x,paddress is %8x",CPC,paddress));
                   SYS_FSR_REG = 0x0f;
  		   goto page_fault_access;
	     }  	
	     paddress = (pte&0xffff0000) | (laddr & 0x0000ffff);	
	     break;
	   case 2 :
	     access_RS = (SYS_CONTROL_REG >>8) & 0x3;
	     poffset   = (laddr >>10)&0x3; // physical offset
	     access_AP = (pte >>(4+2*poffset)) & 0x3;
	     if (check_permission(access_AP,access_RS,isWrite)) {
	        paddress = (pte&0xfffff000) | (laddr & 0x00000fff);	
	        BX_PANIC(("itranslate second page pc is %8x,paddress is %8x",CPC,paddress));
                SYS_FSR_REG = 0x0f;
  	        goto page_fault_access;
	     }	
	     paddress = (pte&0xfffff000) | (laddr & 0x00000fff);	
	     break;
	   case 3 : BX_PANIC(("tiny page not supported"));;        
	}
	break;
	}
     case 2:
	access_AP = (pde >> 10) &0x03;
	access_RS = (SYS_CONTROL_REG >>8) &0x03;
        if (check_permission(access_AP,access_RS,isWrite)){ 
	     BX_PANIC(("itranslate first section pc is %8x",CPC));
	     SYS_FSR_REG = 0x0d;
	    goto page_fault_access;
	}	
	paddress = (pde&0xfff00000) | (laddr & 0x000fffff);	
	break;
     case 3:
	BX_PANIC(("tiny page not supported"));
	//pte_addr = (pde & 0xfffff000) |((laddr & 0x003ff000) >> 10);
	//BX_MMU_THIS_PTR mem->readPhysicalPage(this, pte_addr, 4, &pte);

	goto page_fault_not_present;
  }
 
  if (paddress == 0x50008000) BX_PANIC(("prefetch laddr is %8x PC is %8x",laddr,CPC));
  return(paddress);

page_fault_access:
page_fault_not_present:

   /* added for linking success 2003-07-02 */
  //exception(BX_PF_EXCEPTION, error_code, 0);
    SYS_FAR_REG = laddr;
//    BX_INFO(("prefetch CPSR is %2x,CPC is %2x",PSR,CPC));
    SET_SPSR(PSR,md_cpu_abt);                                       

    SET_PSR((PSR & 0xffffff40)|0x97 );
    BX_CPU_THIS_PTR switch_mode();
    (*(BX_CPU_THIS_PTR regs.regs_R.regs_R_p[LR])) = CPC + 4;
    CPC = 0x0c;

// BX_PANIC(("fetch page fault for address  %8x,PC is %8x,CPSR is %8x\n",laddr,BX_MMU_THIS_PTR regs.regs_PC,BX_MMU_THIS_PTR regs.regs_C.cpsr));
    return(0); // keep compiler happy
}


#if BX_DEBUGGER || BX_DISASM || BX_INSTRUMENTATION || BX_GDBSTUB
  void
bx_mmu_c::dbg_xlate_linear2phy(Bit32u laddr, Bit32u *phy, bx_bool *valid)
{
  Bit32u   lpf, ppf, poffset, TLB_index, paddress;
  Bit32u   pde, pde_addr;
  Bit32u   pte, pte_addr;
  bx_bool  isWrite;

  Bit8u	   access_AP,access_RS;

  if (!PAGING_ENABLE) {
    *phy = laddr;
    *valid = 1;
    return;
    }
#if 0
  // see if page is in the TLB first
  if (BX_MMU_THIS_PTR TLB.entry[TLB_index].lpf == BX_TLB_LPF_VALUE(lpf)) {
    paddress        = BX_MMU_THIS_PTR TLB.entry[TLB_index].ppf | poffset;
    *phy = paddress;
    *valid = 1;
    return;
    }
#endif
  // Get page dir entry
  pde_addr = (SYS_MMUTTB_REG & 0xffffc000) |
             (( (laddr & 0xfff00000) >> 20)<<2);

  BX_MMU_THIS_PTR mem->readPhysicalPage(this, pde_addr, 4, &pde);

  switch (pde &0x03)
  {
     case 0 : goto page_fault;
     case 1 : 
       pte_addr = (pde & 0xfffffc00) |((laddr & 0x000ff000) >> 10);
       BX_MMU_THIS_PTR mem->readPhysicalPage(this, pte_addr, 4, &pte);
       if ( (pte == 0x02) ) {
             access_RS = (SYS_CONTROL_REG >>8) & 0x3;
             poffset   = (laddr >>10)&0x3; // physical offset
             access_AP = (pte >>(4+2*poffset)) & 0x3;
	     isWrite   = 0;
             if (check_permission(access_AP,access_RS,isWrite)) {
                goto page_fault;
             }
 	   paddress = (pte&0xffff0000) | (laddr & 0x0000ffff);
	   break;
        // Page Table Entry NOT present
    	}
    	goto page_fault;
     case 2 :
        access_AP = (pde >> 10) &0x03;
        access_RS = (SYS_CONTROL_REG >>8) &0x03;
        if (check_permission(access_AP,access_RS,isWrite)){
            goto page_fault;
        }
        paddress = (pde&0xfff00000) | (laddr & 0x000fffff);
        break; 	
     case 3 : goto page_fault;
  }

  *phy = paddress;
  *valid = 1;
  return;

page_fault:
  *phy = 0;
  *valid = 0;
  return;
}
#endif



  Bit32u
bx_mmu_c::access_linear(bx_address laddr, unsigned length, unsigned mode,
    unsigned rw, void *data)
{
  Bit32u pageOffset;
  unsigned xlate_rw;
  if (rw==BX_RW) {
    xlate_rw = BX_RW;
    rw = BX_READ;
    }
  else {
    xlate_rw = rw;
    }

  pageOffset = laddr & 0x00000fff;

/*  if (BX_MMU_THIS_PTR cr0.pg) { */
  if (PAGING_ENABLE) {
    if ( (pageOffset + length) <= 4096 ) {
      // Access within single page.
      if (mode) BX_MMU_THIS_PTR address_xlation.paddress1 =
          dtranslate_linear(laddr, mode, xlate_rw);
      else BX_MMU_THIS_PTR address_xlation.paddress1 =
          itranslate_linear(laddr, mode, xlate_rw);
      BX_MMU_THIS_PTR address_xlation.pages     = 1;

/* debugging fht 2003-11-06*/
      if (!BX_MMU_THIS_PTR address_xlation.paddress1)  return 1;
	
      if (rw == BX_READ) {
        BX_INSTR_LIN_READ(CPU_ID, laddr, BX_MMU_THIS_PTR address_xlation.paddress1, length);
        BX_MMU_THIS_PTR mem->readPhysicalPage(this,
            BX_MMU_THIS_PTR address_xlation.paddress1, length, data);
        }
      else {
        BX_INSTR_LIN_WRITE(CPU_ID, laddr, BX_MMU_THIS_PTR address_xlation.paddress1, length);
        BX_MMU_THIS_PTR mem->writePhysicalPage(this,
            BX_MMU_THIS_PTR address_xlation.paddress1, length, data);
        }
      return 0;
      }
    else {
      // access across 2 pages
      if (mode) BX_MMU_THIS_PTR address_xlation.paddress1 =
          dtranslate_linear(laddr, mode, xlate_rw);
      else BX_MMU_THIS_PTR address_xlation.paddress1 =
          itranslate_linear(laddr, mode, xlate_rw);
/* debugging fht 2003-11-06*/
      if (!BX_MMU_THIS_PTR address_xlation.paddress1)  return 1;

      BX_MMU_THIS_PTR address_xlation.len1 = 4096 - pageOffset;
      BX_MMU_THIS_PTR address_xlation.len2 = length -
          BX_MMU_THIS_PTR address_xlation.len1;
      BX_MMU_THIS_PTR address_xlation.pages     = 2;
      if (mode) BX_MMU_THIS_PTR address_xlation.paddress2 =
          dtranslate_linear(laddr + BX_MMU_THIS_PTR address_xlation.len1,
                            mode, xlate_rw);
      else    BX_MMU_THIS_PTR address_xlation.paddress2 =
          itranslate_linear(laddr + BX_MMU_THIS_PTR address_xlation.len1,
                            mode, xlate_rw);

/* debugging fht 2003-11-06*/
      if (!BX_MMU_THIS_PTR address_xlation.paddress2)  return 1;

#ifdef BX_LITTLE_ENDIAN
      if (rw == BX_READ) {
        BX_INSTR_LIN_READ(CPU_ID, laddr,
                          BX_MMU_THIS_PTR address_xlation.paddress1,
                          BX_MMU_THIS_PTR address_xlation.len1);
        BX_MMU_THIS_PTR mem->readPhysicalPage(this, BX_MMU_THIS_PTR address_xlation.paddress1,
                             BX_MMU_THIS_PTR address_xlation.len1, data);
        BX_INSTR_LIN_READ(CPU_ID, laddr + BX_MMU_THIS_PTR address_xlation.len1,
                          BX_MMU_THIS_PTR address_xlation.paddress2,
                          BX_MMU_THIS_PTR address_xlation.len2);
        BX_MMU_THIS_PTR mem->readPhysicalPage(this, BX_MMU_THIS_PTR address_xlation.paddress2,
                             BX_MMU_THIS_PTR address_xlation.len2,
                             ((Bit8u*)data) + BX_MMU_THIS_PTR address_xlation.len1);
        }
      else {
        BX_INSTR_LIN_WRITE(CPU_ID, laddr,
                           BX_MMU_THIS_PTR address_xlation.paddress1,
                           BX_MMU_THIS_PTR address_xlation.len1);
        BX_MMU_THIS_PTR mem->writePhysicalPage(this, BX_MMU_THIS_PTR address_xlation.paddress1,
                              BX_MMU_THIS_PTR address_xlation.len1, data);
        BX_INSTR_LIN_WRITE(CPU_ID, laddr + BX_MMU_THIS_PTR address_xlation.len1,
                          BX_MMU_THIS_PTR address_xlation.paddress2,
                          BX_MMU_THIS_PTR address_xlation.len2);
        BX_MMU_THIS_PTR mem->writePhysicalPage(this, BX_MMU_THIS_PTR address_xlation.paddress2,
                              BX_MMU_THIS_PTR address_xlation.len2,
                              ((Bit8u*)data) + BX_MMU_THIS_PTR address_xlation.len1);
        }

#else // BX_BIG_ENDIAN
      if (rw == BX_READ) {
        BX_INSTR_LIN_READ(CPU_ID, laddr,
                          BX_MMU_THIS_PTR address_xlation.paddress1,
                          BX_MMU_THIS_PTR address_xlation.len1);
        BX_MMU_THIS_PTR mem->readPhysicalPage(this, BX_MMU_THIS_PTR address_xlation.paddress1,
                             BX_MMU_THIS_PTR address_xlation.len1,
                             ((Bit8u*)data) + (length - BX_MMU_THIS_PTR address_xlation.len1));
        BX_INSTR_LIN_READ(CPU_ID, laddr + BX_MMU_THIS_PTR address_xlation.len1,
                          BX_MMU_THIS_PTR address_xlation.paddress2,
                          BX_MMU_THIS_PTR address_xlation.len2);
        BX_MMU_THIS_PTR mem->readPhysicalPage(this, BX_MMU_THIS_PTR address_xlation.paddress2,
                             BX_MMU_THIS_PTR address_xlation.len2, data);
        }
      else {
        BX_INSTR_LIN_WRITE(CPU_ID, laddr,
                           BX_MMU_THIS_PTR address_xlation.paddress1,
                           BX_MMU_THIS_PTR address_xlation.len1);
        BX_MMU_THIS_PTR mem->writePhysicalPage(this, BX_MMU_THIS_PTR address_xlation.paddress1,
                              BX_MMU_THIS_PTR address_xlation.len1,
                              ((Bit8u*)data) + (length - BX_MMU_THIS_PTR address_xlation.len1));
        BX_INSTR_LIN_WRITE(CPU_ID, laddr + BX_MMU_THIS_PTR address_xlation.len1,
                          BX_MMU_THIS_PTR address_xlation.paddress2,
                          BX_MMU_THIS_PTR address_xlation.len2);
        BX_MMU_THIS_PTR mem->writePhysicalPage(this, BX_MMU_THIS_PTR address_xlation.paddress2,
                              BX_MMU_THIS_PTR address_xlation.len2, data);
        }
#endif

      return 0;
      }
    }

  else {
    // Paging off.
    if ( (pageOffset + length) <= 4096 ) {
      // Access within single page.
      BX_MMU_THIS_PTR address_xlation.paddress1 = laddr;
      BX_MMU_THIS_PTR address_xlation.pages     = 1;
      if (rw == BX_READ) {
#if BX_SupportGuest2HostTLB
        Bit32u lpf, tlbIndex;
#endif

        BX_INSTR_LIN_READ(CPU_ID, laddr, laddr, length);
#if BX_SupportGuest2HostTLB
        tlbIndex = BX_TLB_INDEX_OF(laddr);
        lpf = laddr & 0xfffff000;
        if (BX_MMU_THIS_PTR TLB.entry[tlbIndex].lpf == BX_TLB_LPF_VALUE(lpf)) {
          BX_MMU_THIS_PTR mem->readPhysicalPage(this, laddr, length, data);
          return 0;
          }
        // We haven't seen this page, or it's been bumped before.

        BX_MMU_THIS_PTR TLB.entry[tlbIndex].lpf = BX_TLB_LPF_VALUE(lpf);
        BX_MMU_THIS_PTR TLB.entry[tlbIndex].ppf = lpf;
        // Request a direct write pointer so we can do either R or W.
        BX_MMU_THIS_PTR TLB.entry[tlbIndex].hostPageAddr = (Bit32u)
            BX_MMU_THIS_PTR mem->getHostMemAddr(this, A20ADDR(lpf), BX_WRITE);
        if (!BX_MMU_THIS_PTR TLB.entry[tlbIndex].hostPageAddr) {
          // Direct write vetoed.  Try requesting only direct reads.
          BX_MMU_THIS_PTR TLB.entry[tlbIndex].hostPageAddr = (Bit32u)
              BX_MMU_THIS_PTR mem->getHostMemAddr(this, A20ADDR(lpf), BX_READ);
          if (BX_MMU_THIS_PTR TLB.entry[tlbIndex].hostPageAddr) {
            // Got direct read pointer OK.
            BX_MMU_THIS_PTR TLB.entry[tlbIndex].accessBits =
                (ReadSysOK | ReadUserOK);
            }
          else
            BX_MMU_THIS_PTR TLB.entry[tlbIndex].accessBits = 0;
          }
        else {
          // Got direct write pointer OK.  Mark for any operation to succeed.
          BX_MMU_THIS_PTR TLB.entry[tlbIndex].accessBits =
              (ReadSysOK | ReadUserOK | WriteSysOK | WriteUserOK);
          }
#endif  // BX_SupportGuest2HostTLB

        // Let access fall through to the following for this iteration.
        BX_MMU_THIS_PTR mem->readPhysicalPage(this, laddr, length, data);
        }
      else { // Write
#if BX_SupportGuest2HostTLB
        Bit32u lpf, tlbIndex;
#endif

        BX_INSTR_LIN_WRITE(CPU_ID, laddr, laddr, length);
#if BX_SupportGuest2HostTLB
        tlbIndex = BX_TLB_INDEX_OF(laddr);
        lpf = laddr & 0xfffff000;
        if (BX_MMU_THIS_PTR TLB.entry[tlbIndex].lpf == BX_TLB_LPF_VALUE(lpf)) {
          BX_MMU_THIS_PTR mem->writePhysicalPage(this, laddr, length, data);
          return 0;
          }
        // We haven't seen this page, or it's been bumped before.

        BX_MMU_THIS_PTR TLB.entry[tlbIndex].lpf = BX_TLB_LPF_VALUE(lpf);
        BX_MMU_THIS_PTR TLB.entry[tlbIndex].ppf = lpf;
        // TLB.entry[tlbIndex].ppf field not used for PG==0.
        // Request a direct write pointer so we can do either R or W.
        BX_MMU_THIS_PTR TLB.entry[tlbIndex].hostPageAddr = (Bit32u)
            BX_MMU_THIS_PTR mem->getHostMemAddr(this, A20ADDR(lpf), BX_WRITE);
        if (BX_MMU_THIS_PTR TLB.entry[tlbIndex].hostPageAddr) {
          // Got direct write pointer OK.  Mark for any operation to succeed.
          BX_MMU_THIS_PTR TLB.entry[tlbIndex].accessBits =
              (ReadSysOK | ReadUserOK | WriteSysOK | WriteUserOK);
          }
        else
          BX_MMU_THIS_PTR TLB.entry[tlbIndex].accessBits = 0;
#endif  // BX_SupportGuest2HostTLB

        BX_MMU_THIS_PTR mem->writePhysicalPage(this, laddr, length, data);
        }
      }
    else {
      // Access spans two pages.
      BX_MMU_THIS_PTR address_xlation.paddress1 = laddr;
      BX_MMU_THIS_PTR address_xlation.len1 = 4096 - pageOffset;
      BX_MMU_THIS_PTR address_xlation.len2 = length -
          BX_MMU_THIS_PTR address_xlation.len1;
      BX_MMU_THIS_PTR address_xlation.pages     = 2;
      BX_MMU_THIS_PTR address_xlation.paddress2 = laddr +
          BX_MMU_THIS_PTR address_xlation.len1;

#ifdef BX_LITTLE_ENDIAN
      if (rw == BX_READ) {
        BX_INSTR_LIN_READ(CPU_ID, laddr,
                          BX_MMU_THIS_PTR address_xlation.paddress1,
                          BX_MMU_THIS_PTR address_xlation.len1);
        BX_MMU_THIS_PTR mem->readPhysicalPage(this,
            BX_MMU_THIS_PTR address_xlation.paddress1,
            BX_MMU_THIS_PTR address_xlation.len1, data);
        BX_INSTR_LIN_READ(CPU_ID, laddr + BX_MMU_THIS_PTR address_xlation.len1,
                          BX_MMU_THIS_PTR address_xlation.paddress2,
                          BX_MMU_THIS_PTR address_xlation.len2);
        BX_MMU_THIS_PTR mem->readPhysicalPage(this,
            BX_MMU_THIS_PTR address_xlation.paddress2,
            BX_MMU_THIS_PTR address_xlation.len2,
            ((Bit8u*)data) + BX_MMU_THIS_PTR address_xlation.len1);
        }
      else {
        BX_INSTR_LIN_WRITE(CPU_ID, laddr,
                           BX_MMU_THIS_PTR address_xlation.paddress1,
                           BX_MMU_THIS_PTR address_xlation.len1);
        BX_MMU_THIS_PTR mem->writePhysicalPage(this,
            BX_MMU_THIS_PTR address_xlation.paddress1,
            BX_MMU_THIS_PTR address_xlation.len1, data);
        BX_INSTR_LIN_WRITE(CPU_ID, laddr + BX_MMU_THIS_PTR address_xlation.len1,
            BX_MMU_THIS_PTR address_xlation.paddress2,
            BX_MMU_THIS_PTR address_xlation.len2);
        BX_MMU_THIS_PTR mem->writePhysicalPage(this,
            BX_MMU_THIS_PTR address_xlation.paddress2,
            BX_MMU_THIS_PTR address_xlation.len2,
            ((Bit8u*)data) + BX_MMU_THIS_PTR address_xlation.len1);
        }

#else // BX_BIG_ENDIAN
      if (rw == BX_READ) {
        BX_INSTR_LIN_READ(CPU_ID, laddr,
                          BX_MMU_THIS_PTR address_xlation.paddress1,
                          BX_MMU_THIS_PTR address_xlation.len1);
        BX_MMU_THIS_PTR mem->readPhysicalPage(this,
            BX_MMU_THIS_PTR address_xlation.paddress1,
            BX_MMU_THIS_PTR address_xlation.len1,
            ((Bit8u*)data) + (length - BX_MMU_THIS_PTR address_xlation.len1));
        BX_INSTR_LIN_READ(CPU_ID, laddr + BX_MMU_THIS_PTR address_xlation.len1,
                          BX_MMU_THIS_PTR address_xlation.paddress2,
                          BX_MMU_THIS_PTR address_xlation.len2);
        BX_MMU_THIS_PTR mem->readPhysicalPage(this,
            BX_MMU_THIS_PTR address_xlation.paddress2,
            BX_MMU_THIS_PTR address_xlation.len2, data);
        }
      else {
        BX_INSTR_LIN_WRITE(CPU_ID, laddr,
                           BX_MMU_THIS_PTR address_xlation.paddress1,
                           BX_MMU_THIS_PTR address_xlation.len1);
        BX_MMU_THIS_PTR mem->writePhysicalPage(this,
            BX_MMU_THIS_PTR address_xlation.paddress1,
            BX_MMU_THIS_PTR address_xlation.len1,
            ((Bit8u*)data) + (length - BX_MMU_THIS_PTR address_xlation.len1));
        BX_INSTR_LIN_WRITE(CPU_ID, laddr + BX_MMU_THIS_PTR address_xlation.len1,
                          BX_MMU_THIS_PTR address_xlation.paddress2,
                          BX_MMU_THIS_PTR address_xlation.len2);
        BX_MMU_THIS_PTR mem->writePhysicalPage(this,
            BX_MMU_THIS_PTR address_xlation.paddress2,
            BX_MMU_THIS_PTR address_xlation.len2, data);
        }
#endif
      }
    return 0;
    }
}
#else   // BX_SUPPORT_PAGING
  void
bx_mmu_c::access_linear(Bit32u laddr, unsigned length, unsigned mode,
    unsigned rw, void *data)
{
  /* perhaps put this check before all code which calls this function,
   * so we don't have to here
   */
    if (rw == BX_READ)
      BX_MMU_THIS_PTR mem->readPhysicalPage(this, laddr, length, data);
    else
      BX_MMU_THIS_PTR mem->writePhysicalPage(this, laddr, length, data);
    return;
/*    } */

  BX_PANIC(("access_linear: paging not supported"));
}

#endif  // BX_SUPPORT_PAGING

int
libmmu_LTX_plugin_init(plugin_t *plugin, plugintype_t type, int argc, char *argv[])
{
  bx_memsys.pluginMmu = &bx_mmu;
  return(0); // Success
}

 void
bx_mmu_c::init_memory(BX_MEM_C * addspace)
{
  mem=addspace;
}

