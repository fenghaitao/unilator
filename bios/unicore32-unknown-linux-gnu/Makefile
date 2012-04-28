#hacking Makefile 2003-07-20
.EXPORT_ALL_VARIABLES:

TEXTADDR	= 0x30000

TOPDIR		:= $(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi)

CROSS_COMPILE	:= unicore32-linux-
HPATH		= $(TOPDIR)/include
FINDHPATH	= $(HPATH)/bios

CFLAGS          :=-Wall -msoft-float -fomit-frame-pointer -fno-builtin -I$(HPATH)
#CFLAGS          +=-mapcs-32 -mcpu=strongarm110

LDFLAGS		= -p

HOSTCC		=gcc
HOSTCFLAGS	=-Wall -Wstrict-prototypes -O2 -fomit-frame-pointer

CC		=$(CROSS_COMPILE)gcc 
LD		=$(CROSS_COMPILE)ld
AR		=$(CROSS_COMPILE)ar
STRIP		=$(CROSS_COMPILE)strip
NM              =$(CROSS_COMPILE)nm
OBJCOPY		=$(CROSS_COMPILE)objcopy
OBJDUMP		=$(CROSS_COMPILE)objdump

SUBDIRS		= init lib drivers
BIOSOBJS	= init/crt0.o init/init.o
DRIVERS		= drivers/drivers.o drivers/block/block.a drivers/net/net.a \
		  drivers/scsi/scsi.a drivers/char/char.a drivers/isa/isa.a
LIBS		= lib/lib.a $(shell $(CC) $(CFLAGS) --print-libgcc-file-name)

all:		bios.img scripts/mkboot

depend dep: dep-files

bios.img:	bios.elf
		$(OBJCOPY) -O binary  bios.elf $@
		$(OBJDUMP) -D bios.elf > bios.dump

bios.elf:	subdirs elfbios.lds 
		$(LD) $(LDFLAGS) -T elfbios.lds -o $@ $(BIOSOBJS) $(DRIVERS) $(LIBS)
		$(NM) $@ | sort > $@.syms

elfbios.lds:    elfbios.lds.in Makefile
		@sed 's/TEXTADDR/$(TEXTADDR)/' <$< >$@


clean:
		$(RM) `find . -name '*.[oas]' -print`
		$(RM) `find . -name '.*.flags' -print`
		$(RM) bios bios.a.out bios.dump bios.elf bios.img bios.elf.syms elfbios.lds

mrproper:	clean
		$(RM) `find . -name '.depend' -print`
		$(RM) scripts/mkdep

subdirs:	$(patsubst %, _dir_%,$(SUBDIRS))

$(patsubst %, _dir_%, $(SUBDIRS)) : dummy
		$(MAKE) -C $(patsubst _dir_%, %, $@)

dep-files: scripts/mkdep
		scripts/mkdep `find $(FINDHPATH) -follow -name \*.h -print` > .hdepend
		$(MAKE) $(patsubst %,_sfdep_%,$(SUBDIRS)) _FASTDEP_ALL_SUB_DIRS="$(SUBDIRS)"

scripts/mkdep: scripts/mkdep.c
		$(HOSTCC) $(HOSTCFLAGS) -o scripts/mkdep scripts/mkdep.c

scripts/mkboot: scripts/mkboot.c
		$(HOSTCC) $(HOSTCFLAGS) -o scripts/mkboot scripts/mkboot.c

dummy:

include Rules.make
