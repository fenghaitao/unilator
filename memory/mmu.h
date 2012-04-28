/////////////////////////////////////////////////////////////////////////
// $Id: memory.h,v 1.2 2003/11/04 09:43:04 fht Exp $
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

class BOCHSAPI bx_mmu_c : public bx_mmu_stub_c {
public:

  BX_MEM_C * mem;

  struct {
    bx_address  rm_addr; // The address offset after resolution.
    Bit32u  paddress1;  // physical address after translation of 1st len1 bytes of data
    Bit32u  paddress2;  // physical address after translation of 2nd len2 bytes of data
    Bit32u  len1;       // Number of bytes in page 1
    Bit32u  len2;       // Number of bytes in page 2
    Bit32u  pages;      // Number of pages access spans (1 or 2).  Also used
                        //   for the case when a native host pointer is
                        //   available for the R-M-W instructions.  The host
                        //   pointer is stuffed here.  Since this field has
                        //   to be checked anyways (and thus cached), if it
                        //   is greated than 2 (the maximum possible for
                        //   normal cases) it is a native pointer and is used
                        //   for a direct write access.
    } address_xlation;

  virtual void   init_memory(BX_MEM_C * addspace);	
  virtual Bit32u access_linear(Bit32u laddr, unsigned length, unsigned pl,
		       unsigned rw, void *data);
  BX_SMF  Bit32u itranslate_linear(bx_address laddr, unsigned mode,unsigned rw);
  BX_SMF  Bit32u dtranslate_linear(bx_address laddr, unsigned mode,unsigned rw);
  BX_SMF  Bit32u check_permission(Bit8u access_AP,Bit8u access_RS,bx_bool isWrite);
#if BX_DEBUGGER || BX_DISASM || BX_INSTRUMENTATION || BX_GDBSTUB
  virtual  void dbg_xlate_linear2phy(Bit32u laddr, Bit32u *phy, bx_bool *valid);
#endif	  
};

BOCHSAPI extern bx_mmu_c bx_mmu;
