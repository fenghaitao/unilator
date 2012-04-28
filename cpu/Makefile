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


.SUFFIXES: .cc
target = arm-unknown-linux-gnu
srcdir = .
VPATH = 
top_builddir    = ..
top_srcdir      = ..

SHELL = /bin/sh



CXX = g++
CXXFLAGS = -g -O2 -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES  $(X_CFLAGS)

LDFLAGS = 
LIBS =  -lm
X_LIBS = 
X_PRE_LIBS =  -lSM -lICE
RANLIB = ranlib

all: libcpu.a

libcpu.a: 
	rm -f  libcpu.a
	ln -sf $(target)/cpu.h cpu.h
	cd $(target) &&	make libcpu.a

$(OBJS): $(BX_INCLUDES)

$(OBJS64): $(BX_INCLUDES)

clean:
	cd $(target) &&	make clean

dist-clean: clean
	cd $(target) &&	make dist-clean
	rm -f  Makefile

