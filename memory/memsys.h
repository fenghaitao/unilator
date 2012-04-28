/////////////////////////////////////////////////////////////////////////
// $Id: iodev.h,v 1.1.1.1 2003/09/25 03:12:54 fht Exp $
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


class BOCHSAPI bx_memmodel_c : public logfunctions {
  public:
  virtual ~bx_memmodel_c () {};
  virtual void init(void) {};
  virtual void reset(unsigned type) {};
  virtual void mem_load_state () {};
  virtual void mem_save_state () {};
};

class BOCHSAPI bx_cache_stub_c : public bx_memmodel_c {
  public:
  virtual ~bx_cache_stub_c () {};
};

class BOCHSAPI bx_tlb_stub_c : public bx_memmodel_c {
  public:
  virtual ~bx_tlb_stub_c () {};
};
class bx_mem_stub_c;

class BOCHSAPI bx_mmu_stub_c : public bx_memmodel_c {
  public:
  virtual ~bx_mmu_stub_c () {};
  virtual Bit32u access_linear(Bit32u laddr, unsigned length, unsigned mode,
		      unsigned rw, void *data){};
  virtual void init_memory(BX_MEM_C * addspace){};
#if BX_DEBUGGER || BX_DISASM || BX_INSTRUMENTATION || BX_GDBSTUB
  virtual  void dbg_xlate_linear2phy(Bit32u laddr, Bit32u *phy, bx_bool *valid){};
#endif
  
};

class BOCHSAPI bx_mem_stub_c : public bx_memmodel_c {
  public:
  virtual ~bx_mem_stub_c () {};
  virtual void    init_memory(int memsize){};
  virtual void    readPhysicalPage(bx_cpu_stub_c *cpu, Bit32u addr,		                                      unsigned len, void *data){};
  virtual void    writePhysicalPage(bx_cpu_stub_c *cpu, Bit32u addr,		                                       unsigned len, void *data){};
  virtual void    load_ROM(const char *path, Bit32u romaddress){};
  virtual void    load_program(const char *path, Bit32u romaddress){};
  virtual Bit32u  get_memory_in_k(void){};
#if BX_DEBUGGER
  virtual bx_bool dbg_fetch_mem(Bit32u addr, unsigned len, Bit8u *buf){return 0;};
  virtual bx_bool dbg_set_mem(Bit32u addr, unsigned len, Bit8u *buf){return 0;};
  virtual bx_bool dbg_crc32(unsigned long (*f)(unsigned char *buf, int len),
			        Bit32u addr1, Bit32u addr2, Bit32u *crc){return 0;};
#endif
	
};

#define BX_USE_MEMSYS_SMF	1
#if BX_USE_MEMSYS_SMF == 0
// normal member functions.  This can ONLY be used within BX_CPU_C classes.
// Anyone on the outside should use the BX_CPU macro (defined in bochs.h)
// instead.
#  define BX_MEMSYS_THIS_PTR  this->
#  define BX_MEMSYS_THIS      this
#else
// static member functions.  With SMF, there is only one CPU by definition.
#  define BX_MEMSYS_THIS_PTR  (&bx_memsys)->
#  define BX_MEMSYS_THIS      bx_memsys
#endif

/* added by fht 2003-10-21 */

#define SYS_NUM_MREGS           16
typedef word_t md_sys_t[SYS_NUM_MREGS];

/* SYS CONTROL REGISTER */
#define SYS_MAIN_ID_REG     (BX_MEMSYS_THIS_PTR regs_SYS[0])
#define SYS_CONTROL_REG     (BX_MEMSYS_THIS_PTR regs_SYS[1])
#define SYS_MMUTTB_REG     (BX_MEMSYS_THIS_PTR regs_SYS[2])
#define SYS_MMUDAC_REG     (BX_MEMSYS_THIS_PTR regs_SYS[3])
#define SYS_FSR_REG     (BX_MEMSYS_THIS_PTR regs_SYS[5])
#define SYS_FAR_REG     (BX_MEMSYS_THIS_PTR regs_SYS[6])
 
#define PAGING_ENABLE     (SYS_CONTROL_REG & 0x01) 


class BOCHSAPI bx_memsys_c : public logfunctions {
public:
  md_sys_t regs_SYS;            /* sys control coprocessor register file */

  bx_memsys_c(void);
  ~bx_memsys_c(void);
  void init(int memsize);
  void reset(unsigned type);

  bx_tlb_stub_c    *pluginTlb;
  bx_cache_stub_c  *pluginCache;
  bx_mmu_stub_c    *pluginMmu;
  bx_mem_stub_c    *pluginMem;

  bx_tlb_stub_c    stubTlb;
  bx_cache_stub_c  stubCache;
  bx_mmu_stub_c    stubMmu;
  bx_mem_stub_c    stubMem;

  void    load_ROM(const char *path, Bit32u romaddress);
  void    load_program(const char *path, Bit32u romaddress);
  Bit32u  get_memory_in_k(void);

  Bit32u write_virtual_byte(Bit8u mode,bx_address laddr, Bit8u *data);
  Bit32u write_virtual_word(Bit8u mode,bx_address laddr, Bit16u *data);
  Bit32u write_virtual_dword(Bit8u mode,bx_address laddr, Bit32u *data);
  Bit32u write_virtual_qword(Bit8u mode,bx_address laddr, Bit64u *data);

  Bit32u read_virtual_byte(Bit8u mode,bx_address laddr, Bit8u *data);
  Bit32u read_virtual_word(Bit8u mode,bx_address laddr, Bit16u *data);
  Bit32u read_virtual_dword(Bit8u mode,bx_address laddr, Bit32u *data);
  Bit32u read_virtual_qword(Bit8u mode,bx_address laddr, Bit64u *data);
};

#if BX_PROVIDE_CPU_MEMORY==1

#if BX_SMP_PROCESSORS==1
BOCHSAPI extern bx_memsys_c    bx_memsys;
#else
BOCHSAPI extern bx_memsys_c   *bx_memsys_array[BX_ADDRESS_SPACES];
#endif  /* BX_SMP_PROCESSORS */

#endif  /* BX_PROVIDE_CPU_MEMORY==1 */

#if BX_SUPPORT_CACHE
#  include "memory/tlb.h"
#  include "memory/cache.h"
#endif

#include "memory/mmu.h"
#include "memory/memory.h"

