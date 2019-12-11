# $PIP_VERSION: Version 1.0$
# $PIP_license: <Simplified BSD License>
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# The views and conclusions contained in the software and documentation
# are those of the authors and should not be interpreted as representing
# official policies, either expressed or implied, of the PiP project.$
# $RIKEN_copyright: 2018 Riken Center for Computational Sceience,
# 	  System Software Devlopment Team. All rights researved$

top_builddir = ../..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk

CWD = $(shell pwd)
CFLAGS += -DCWD=\"$(CWD)\" -DPIPS=\"$(CWD)/../../bin/pips\"

SRCS	= dlmopen_count.c ompnumthread.c timer.c \
	pip_mode.c pip_is_debug_build.c \
	pip_task.c pip_blt.c \
	inf.c

PROGRAMS = dlmopen_count ompnumthread timer \
	pip_mode pip_is_debug_build \
	pip_task pip_blt \
	inf

PROGRAMS_TO_INSTALL = # nothing

all: $(PROGRAMS)

pip_mode: pip_mode.c

dlmopen_count: dlmopen_count.c

ompnumthread: ompnumthread.c
	$(CC) -fopenmp -openmp -O2 $< -o$@

inf : inf.c Makefile
	../scripts/pipcc --pipdir ../.. inf.c --piptask -o inf

debug:
	CPPFLAGS+="-DDEBUG" make all

test :
	../test.sh test.list

include $(top_srcdir)/build/rule.mk
include ../Test.common
