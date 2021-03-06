# Copyright (C) 2001  MandrakeSoft S.A.
#
#   MandrakeSoft S.A.
#   43, rue d'Aboukir
#   75002 Paris - France
#   http://www.linux-mandrake.com/
#   http://www.mandrakesoft.com/
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

# Makefile for the BIOS component of bochs


.SUFFIXES: .cc

srcdir = .


SHELL = /bin/sh



CXX = g++
CXXFLAGS = -g -O2 -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES

LDFLAGS = 
LIBS =  -lm
RANLIB = ranlib

BX_INCDIRS = -I.. -I$(srcdir)/.. -I../iodev -I$(srcdir)/../iodev
LOCAL_CXXFLAGS =

#
# -------- end configurable options --------------------------
#


.cc.o:
	$(CXX) -c $(BX_INCDIRS) $(CXXFLAGS) $(LOCAL_CXXFLAGS) $< -o $@


bios: BIOS-bochs-latest BIOS-bochs-2-processors BIOS-bochs-4-processors BIOS-bochs-8-processors 

clean:
	rm -f  *.o *.a *.s rombios.bin _rombios*_.c
	rm -f  as86-sym.txt ld86-sym.txt 
	rm -f  rombios*.txt rombios*.sym usage

dist-clean: clean
	rm -f  Makefile

bios-clean:
	rm -f  BIOS-bochs-*

BIOS-bochs-latest: rombios.c
	gcc -DBX_SMP_PROCESSORS=1 -E $< > _rombios_.c
	bcc -o rombios.s -C-c -D__i86__ -0 -S _rombios_.c
	sed -e 's/^\.text//' -e 's/^\.data//' rombios.s > _rombios_.s
	as86 _rombios_.s -b tmp.bin -u- -w- -g -0 -j -O -l rombios.txt
	-perl ${srcdir}/makesym.perl < rombios.txt > rombios.sym
	mv tmp.bin BIOS-bochs-latest
	rm -f _rombios_.s

BIOS-bochs-2-processors: rombios.c
	gcc -DBX_SMP_PROCESSORS=2 -E $< > _rombios2_.c
	bcc -o rombios2.s -C-c -D__i86__ -0 -S _rombios2_.c
	sed -e 's/^\.text//' -e 's/^\.data//' rombios2.s > _rombios2_.s
	as86 _rombios2_.s -b tmp2.bin -u- -w- -g -0 -j -O -l rombios2.txt
	-perl ${srcdir}/makesym.perl < rombios2.txt > rombios2.sym
	mv tmp2.bin BIOS-bochs-2-processors
	rm -f _rombios2_.s

BIOS-bochs-4-processors: rombios.c
	gcc -DBX_SMP_PROCESSORS=4 -E $< > _rombios4_.c
	bcc -o rombios4.s -C-c -D__i86__ -0 -S _rombios4_.c
	sed -e 's/^\.text//' -e 's/^\.data//' rombios4.s > _rombios4_.s
	as86 _rombios4_.s -b tmp4.bin -u- -w- -g -0 -j -O -l rombios4.txt
	-perl ${srcdir}/makesym.perl < rombios4.txt > rombios4.sym
	mv tmp4.bin BIOS-bochs-4-processors
	rm -f _rombios4_.s

BIOS-bochs-8-processors: rombios.c
	gcc -DBX_SMP_PROCESSORS=8 -E $< > _rombios8_.c
	bcc -o rombios8.s -C-c -D__i86__ -0 -S _rombios8_.c
	sed -e 's/^\.text//' -e 's/^\.data//' rombios8.s > _rombios8_.s
	as86 _rombios8_.s -b tmp8.bin -u- -w- -g -0 -j -O -l rombios8.txt
	-perl ${srcdir}/makesym.perl < rombios8.txt > rombios8.sym
	mv tmp8.bin BIOS-bochs-8-processors
	rm -f _rombios8_.s

