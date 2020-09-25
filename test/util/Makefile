# $PIP_VERSION: Version 2.0.0$
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
# $RIKEN_copyright: Riken Center for Computational Sceience,
# System Software Development Team, 2016, 2017, 2018, 2019$

top_builddir = ../..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk
include ../Test.common

CWD = $(shell pwd)
CPPFLAGS += -I.. -I../../include  -DCWD=\"$(CWD)\" -DPIPS=\"$(CWD)/../../bin/pips\"

SRCS	= dlmopen_count.c ompnumthread.c timer.c inf.c \
	pip_mode_check.c pip_is_debug_build.c pip_task.c

PROGRAMS = dlmopen_count ompnumthread timer inf \
	pip_mode_check pip_is_debug_build pip_task

PROGRAMS_TO_INSTALL = # nothing

all: $(PROGRAMS)

debug:
	DBGFLAG=-DDEBUG $(MAKE) all

dlmopen_count: dlmopen_count.c Makefile
	$(CC) -g -O2 $(CPPFLAGS) $(DBGFLAG) $(PIP_CFLAGS_BOTH) $(PIP_LDFLAGS_BOTH) $< -o $@

ompnumthread: ompnumthread.c Makefile
	$(CC) -fopenmp -openmp $< -o $@

timer: timer.c Makefile
	$(CC) -g -O2 $(CPPFLAGS) $(DBGFLAG) $(PIP_CFLAGS_ROOT) $(PIP_LDFLAGS_ROOT) $< -o $@

pip_is_debug_build : pip_is_debug_build.c Makefile
	../scripts/pipcc -g -O2 $< -o $@

pip_mode_check : pip_mode_check.c Makefile
	../scripts/pipcc -g -O2 -I.. $(DBGFLAG) $< --piptask -o $@

pip_task : pip_task.c Makefile
	../scripts/pipcc -g -O2 -I.. $(DBGFLAG) $< --piptask -o $@

inf : inf.c Makefile
	../scripts/pipcc -g -O2 -I.. $(DBGFLAG) $< --piptask -o $@

test :
	$(MAKE) -C ../prog
	../test.sh test.list

install_test_util_dir = $(install_test_dir)/util

install-test:
	$(MKDIR_P) $(install_test_util_dir)
	$(INSTALL) -C -m 644 Makefile $(SRCS) test.list $(install_test_util_dir)
	$(INSTALL) -C -m 744 test-timer.sh path-test.sh $(install_test_util_dir)

include $(top_srcdir)/build/rule.mk
