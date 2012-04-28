/////////////////////////////////////////////////////////////////////////
// $Id: devices.cc,v 1.1.1.1 2003/09/25 03:12:54 fht Exp $
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2002  MandrakeSoft S.A.
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


#include "bochs.h"
#define LOG_THIS bx_memsys.

bx_memsys_c bx_memsys;

// constructor for bx_memsys_c
bx_memsys_c::bx_memsys_c(void)
{
  pluginCache = &stubCache;
  pluginTlb = &stubTlb;
  pluginMmu = &stubMmu;
  pluginMem = &stubMem;
}


bx_memsys_c::~bx_memsys_c(void)
{
  // nothing needed for now
  BX_DEBUG(("Exit."));
}


  void
bx_memsys_c::init(int memsize)
{
  PLUG_load_plugin(memory, PLUGTYPE_CORE);
 // Start with registering the default (unmapped) handler
  pluginMem->init_memory (memsize);
  PLUG_load_plugin(mmu, PLUGTYPE_CORE);
 // Start with registering the default (unmapped) handler
  pluginMmu->init_memory(&bx_mem);
}


  void
bx_memsys_c::reset(unsigned type)
{
/* add CPSR reset 2003-10-20*/
/* disabling cache */
  SYS_MAIN_ID_REG = 0x4401a100;
  SYS_CONTROL_REG = 0x00000070;
}

   void    
bx_memsys_c::load_ROM(const char *path, Bit32u romaddress)
{
   pluginMem->load_ROM(path,romaddress);
}

   Bit32u  
bx_memsys_c::get_memory_in_k(void)
{
   return pluginMem->get_memory_in_k();	
}

Bit32u 
bx_memsys_c::write_virtual_byte(Bit8u mode,bx_address laddr, Bit8u *data)
{
//  BX_INSTR_MEM_DATA(CPU_ID, laddr, 1, BX_WRITE);

#if BX_SupportGuest2HostTLB
      {
      bx_address lpf;
      Bit32u tlbIndex, pageOffset;

      pageOffset = laddr & 0xfff;
      tlbIndex = BX_TLB_INDEX_OF(laddr);
      lpf = LPFOf(laddr);
      if ( (BX_CPU_THIS_PTR TLB.entry[tlbIndex].lpf == BX_TLB_LPF_VALUE(lpf)))
        {
        Bit32u accessBits;
        Bit32u hostPageAddr;
        Bit8u *hostAddr;

        // See if the TLB entry privilege level allows us write access
        // from this CPL.
        hostPageAddr = BX_CPU_THIS_PTR TLB.entry[tlbIndex].hostPageAddr;
        hostAddr = (Bit8u*) (hostPageAddr | pageOffset);
        accessBits = BX_CPU_THIS_PTR TLB.entry[tlbIndex].accessBits;
        if ( accessBits & (1 << (2 | pl)) ) {
#if BX_SupportICache
          Bit32u *pageStamp;
          pageStamp = & BX_CPU_THIS_PTR iCache.pageWriteStampTable[
              BX_CPU_THIS_PTR TLB.entry[tlbIndex].ppf>>12];
#endif
          // Current write access has privilege.
          if (hostPageAddr
#if BX_SupportICache
           && (*pageStamp & ICacheWriteStampMask)
#endif
              ) {
            *hostAddr = *data;
#if BX_SupportICache
            (*pageStamp)--;
#endif
            return;
            }
          }
        }
      }
#endif  // BX_SupportGuest2HostTLB

  return pluginMmu->access_linear(laddr, 1, mode, BX_WRITE, (void *) data);
}

Bit32u 
bx_memsys_c::write_virtual_word(Bit8u mode,bx_address laddr, Bit16u *data)
{

//      BX_INSTR_MEM_DATA(CPU_ID, laddr, 2, BX_WRITE);

#if BX_SupportGuest2HostTLB
      {
      bx_address lpf;
      Bit32u tlbIndex, pageOffset;

      pageOffset = laddr & 0xfff;
      if (pageOffset <= 0xffe) { // Make sure access does not span 2 pages.
        tlbIndex = BX_TLB_INDEX_OF(laddr);
        lpf = LPFOf(laddr);
        if ( (BX_CPU_THIS_PTR TLB.entry[tlbIndex].lpf == BX_TLB_LPF_VALUE(lpf))
             ) {
          Bit32u accessBits;
          Bit32u  hostPageAddr;
          Bit16u *hostAddr;

          // See if the TLB entry privilege level allows us write access
          // from this CPL.
          hostPageAddr = BX_CPU_THIS_PTR TLB.entry[tlbIndex].hostPageAddr;
          hostAddr = (Bit16u*) (hostPageAddr | pageOffset);
          accessBits = BX_CPU_THIS_PTR TLB.entry[tlbIndex].accessBits;
          if ( accessBits & (1 << (2 | pl)) ) {
#if BX_SupportICache
            Bit32u *pageStamp;
            pageStamp = & BX_CPU_THIS_PTR iCache.pageWriteStampTable[
                BX_CPU_THIS_PTR TLB.entry[tlbIndex].ppf>>12];
#endif
            // Current write access has privilege.
            if (hostPageAddr
#if BX_SupportICache
              && (*pageStamp & ICacheWriteStampMask)
#endif
                ) {
              WriteHostWordToLittleEndian(hostAddr, *data);
#if BX_SupportICache
              (*pageStamp)--;
#endif
              return;
              }
            }
          }
        }
      }
#endif  // BX_SupportGuest2HostTLB

   return pluginMmu->access_linear(laddr, 2, mode, BX_WRITE, (void *) data);
}

Bit32u 
bx_memsys_c::write_virtual_dword(Bit8u mode,bx_address laddr, Bit32u *data)
{
  //BX_INSTR_MEM_DATA(CPU_ID, laddr, 4, BX_WRITE);

#if BX_SupportGuest2HostTLB
      {
      bx_address lpf;
      Bit32u tlbIndex, pageOffset;

      pageOffset = laddr & 0xfff;
      if (pageOffset <= 0xffc) { // Make sure access does not span 2 pages.
        tlbIndex = BX_TLB_INDEX_OF(laddr);
        lpf = LPFOf(laddr);
        if ( (BX_CPU_THIS_PTR TLB.entry[tlbIndex].lpf == BX_TLB_LPF_VALUE(lpf))
             ) {
          Bit32u accessBits;
          Bit32u  hostPageAddr;
          Bit32u *hostAddr;

          // See if the TLB entry privilege level allows us write access
          // from this CPL.
          hostPageAddr = BX_CPU_THIS_PTR TLB.entry[tlbIndex].hostPageAddr;
          hostAddr = (Bit32u*) (hostPageAddr | pageOffset);
          accessBits = BX_CPU_THIS_PTR TLB.entry[tlbIndex].accessBits;
          if ( accessBits & (1 << (2 | pl)) ) {
#if BX_SupportICache
            Bit32u *pageStamp;
            pageStamp = & BX_CPU_THIS_PTR iCache.pageWriteStampTable[
                BX_CPU_THIS_PTR TLB.entry[tlbIndex].ppf>>12];
#endif
            // Current write access has privilege.
            if (hostPageAddr
#if BX_SupportICache
             && (*pageStamp & ICacheWriteStampMask)
#endif
                ) {
              WriteHostDWordToLittleEndian(hostAddr, *data);
#if BX_SupportICache
              (*pageStamp)--;
#endif
              return;
              }
            }
          }
        }
      }
#endif  // BX_SupportGuest2HostTLB
/* debugging */
   return pluginMmu->access_linear(laddr, 4, mode, BX_WRITE, (void *) data);
}

Bit32u 
bx_memsys_c::write_virtual_qword(Bit8u mode,bx_address laddr, Bit64u *data)
{
      //BX_INSTR_MEM_DATA(CPU_ID, laddr, 8, BX_WRITE);

#if BX_SupportGuest2HostTLB
      {
      bx_address lpf;
      Bit32u tlbIndex, pageOffset;

      pageOffset = laddr & 0xfff;
      if (pageOffset <= 0xff8) { // Make sure access does not span 2 pages.
        tlbIndex = BX_TLB_INDEX_OF(laddr);
        lpf = LPFOf(laddr);
        if ( (BX_CPU_THIS_PTR TLB.entry[tlbIndex].lpf == BX_TLB_LPF_VALUE(lpf))
             ) {
          Bit32u accessBits;
          Bit32u  hostPageAddr;
          Bit64u *hostAddr;

          // See if the TLB entry privilege level allows us write access
          // from this CPL.
          hostPageAddr = BX_CPU_THIS_PTR TLB.entry[tlbIndex].hostPageAddr;
          hostAddr = (Bit64u*) (hostPageAddr | pageOffset);
          accessBits = BX_CPU_THIS_PTR TLB.entry[tlbIndex].accessBits;
          if ( accessBits & (1 << (2 | pl)) ) {
#if BX_SupportICache
            Bit32u *pageStamp;
            pageStamp = & BX_CPU_THIS_PTR iCache.pageWriteStampTable[
                BX_CPU_THIS_PTR TLB.entry[tlbIndex].ppf>>12];
#endif
            // Current write access has privilege.
            if (hostPageAddr
#if BX_SupportICache
             && (*pageStamp & ICacheWriteStampMask)
#endif
                ) {
              WriteHostQWordToLittleEndian(hostAddr, *data);
#if BX_SupportICache
              (*pageStamp)--;
#endif
              return;
              }
            }
          }
        }
      }
#endif  // BX_SupportGuest2HostTLB
  return pluginMmu->access_linear(laddr, 8, mode, BX_WRITE, (void *) data);
}

Bit32u 
bx_memsys_c::read_virtual_byte(Bit8u mode,bx_address laddr, Bit8u *data)
{
      //BX_INSTR_MEM_DATA(CPU_ID, laddr, 1, BX_READ);

#if BX_SupportGuest2HostTLB
      {
      bx_address lpf;
      Bit32u tlbIndex, pageOffset;

      pageOffset = laddr & 0xfff;
      tlbIndex = BX_TLB_INDEX_OF(laddr);
      lpf = LPFOf(laddr);
      if (BX_CPU_THIS_PTR TLB.entry[tlbIndex].lpf == BX_TLB_LPF_VALUE(lpf)) {
        // See if the TLB entry privilege level allows us read access
        // from this CPL.
        Bit32u accessBits;
        Bit32u hostPageAddr;
        Bit8u *hostAddr;

        hostPageAddr = BX_CPU_THIS_PTR TLB.entry[tlbIndex].hostPageAddr;
        hostAddr = (Bit8u*) (hostPageAddr | pageOffset);
        accessBits = BX_CPU_THIS_PTR TLB.entry[tlbIndex].accessBits;
        if ( accessBits & (1<<pl) ) { // Read this pl OK.
          if (hostPageAddr) {
            *data = *hostAddr;
            return;
            }
          }
        }
      }
#endif  // BX_SupportGuest2HostTLB

  return (pluginMmu->access_linear(laddr, 1, mode, BX_READ, (void *) data));
}

Bit32u 
bx_memsys_c::read_virtual_word(Bit8u mode,bx_address laddr, Bit16u *data)
{

      //BX_INSTR_MEM_DATA(CPU_ID, laddr, 2, BX_READ);

#if BX_SupportGuest2HostTLB
      {
      bx_address lpf;
      Bit32u tlbIndex, pageOffset;

      pageOffset = laddr & 0xfff;
      if (pageOffset <= 0xffe) { // Make sure access does not span 2 pages.
        tlbIndex = BX_TLB_INDEX_OF(laddr);
        lpf = LPFOf(laddr);
        if (BX_CPU_THIS_PTR TLB.entry[tlbIndex].lpf == BX_TLB_LPF_VALUE(lpf)) {
          // See if the TLB entry privilege level allows us read access
          // from this CPL.
          Bit32u accessBits;
          Bit32u hostPageAddr;
          Bit16u *hostAddr;

          hostPageAddr = BX_CPU_THIS_PTR TLB.entry[tlbIndex].hostPageAddr;
          hostAddr = (Bit16u*) (hostPageAddr | pageOffset);
          accessBits = BX_CPU_THIS_PTR TLB.entry[tlbIndex].accessBits;
          if ( accessBits & (1<<pl) ) { // Read this pl OK.
            if (hostPageAddr) {
              ReadHostWordFromLittleEndian(hostAddr, *data);
              return;
              }
            }
          }
        }
      }
#endif  // BX_SupportGuest2HostTLB

      return pluginMmu->access_linear(laddr, 2, mode, BX_READ, (void *) data);
}

Bit32u 
bx_memsys_c::read_virtual_dword(Bit8u mode,bx_address laddr, Bit32u *data)
{

      //BX_INSTR_MEM_DATA(CPU_ID, laddr, 4, BX_READ);

#if BX_SupportGuest2HostTLB
      {
      bx_address lpf;
      Bit32u tlbIndex, pageOffset;

      pageOffset = laddr & 0xfff;
      if (pageOffset <= 0xffc) { // Make sure access does not span 2 pages.
        tlbIndex = BX_TLB_INDEX_OF(laddr);
        lpf = LPFOf(laddr);
        if (BX_CPU_THIS_PTR TLB.entry[tlbIndex].lpf == BX_TLB_LPF_VALUE(lpf)) {
          // See if the TLB entry privilege level allows us read access
          // from this CPL.
          Bit32u accessBits;
          Bit32u hostPageAddr;
          Bit32u *hostAddr;

          hostPageAddr = BX_CPU_THIS_PTR TLB.entry[tlbIndex].hostPageAddr;
          hostAddr = (Bit32u*) (hostPageAddr | pageOffset);
          accessBits = BX_CPU_THIS_PTR TLB.entry[tlbIndex].accessBits;
          if ( accessBits & (1<<pl) ) { // Read this pl OK.
            if (hostPageAddr) {
              ReadHostDWordFromLittleEndian(hostAddr, *data);
              return;
              }
            }
          }
        }
      }
#endif  // BX_SupportGuest2HostTLB

      return pluginMmu->access_linear(laddr, 4, mode, BX_READ, (void *) data);
}

Bit32u 
bx_memsys_c::read_virtual_qword(Bit8u mode,bx_address laddr, Bit64u *data)
{

      //BX_INSTR_MEM_DATA(CPU_ID, laddr, 8, BX_READ);

#if BX_SupportGuest2HostTLB
      {
      bx_address lpf;
      Bit32u tlbIndex, pageOffset;

      pageOffset = laddr & 0xfff;
      if (pageOffset <= 0xff8) { // Make sure access does not span 2 pages.
        tlbIndex = BX_TLB_INDEX_OF(laddr);
        lpf = LPFOf(laddr);
        if (BX_CPU_THIS_PTR TLB.entry[tlbIndex].lpf == BX_TLB_LPF_VALUE(lpf)) {
          // See if the TLB entry privilege level allows us read access
          // from this CPL.
          Bit32u accessBits;
          Bit32u hostPageAddr;
          Bit64u *hostAddr;

          hostPageAddr = BX_CPU_THIS_PTR TLB.entry[tlbIndex].hostPageAddr;
          hostAddr = (Bit64u*) (hostPageAddr | pageOffset);
          accessBits = BX_CPU_THIS_PTR TLB.entry[tlbIndex].accessBits;
          if ( accessBits & (1<<pl) ) { // Read this pl OK.
            if (hostPageAddr) {
              ReadHostQWordFromLittleEndian(hostAddr, *data);
              return;
              }
            }
          }
        }
      }
#endif  // BX_SupportGuest2HostTLB

      return pluginMmu->access_linear(laddr, 8, mode, BX_READ, (void *) data);
}


