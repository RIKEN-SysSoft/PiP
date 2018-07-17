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
include $(srcdir)/../Eval.common

#XPMEM_DIR = /home/ahori/work/ARM64/XPMEM/xpmem-master
XPMEM_DIR = /work/ahori/XPMEM/xpmem-eval
XPMINCFLAGS = -I. -I.. -I$(XPMEM_DIR)/include

PIPINCFLAGS = -I. -I.. -I`pwd`/../../include
PIPCFLAGS  = $(CFLAGS) $(PIEFLAG) $(PIPINCFLAGS)

DEPINCS = ./xpmem_eval.h ../eval.h $(PIPINCDIR)/pip.h

SRCS  = eval.c pip.c shmem.c xpmem.c

PROGRAMS = eval-pip pip shmem pip-ht eval-xpmem xpmem xpmem-ht
PROGS-NOXPMEM = eval-pip pip shmem

all: $(PROGRAMS)

PROGRAMS_TO_INSTALL = # nothing

noxpmem: $(PROGS-NOXPMEM)

eval-pip: eval.c xpmem_eval.h Makefile
	$(CC) -O3 $(CPPFLAGS) $(PIPCFLAGS) $(PIPLDFLAGS) eval.c -o $@ -lrt

eval-xpmem: eval.c xpmem_eval.h Makefile
	$(CC) -O3 $(XPMINCFLAGS) $(XPMEM_DIR)/lib/libxpmem.o eval.c -o eval-xpmem -lrt

pip: pip.c xpmem_eval.h Makefile
	$(CC) -O3 $(PIPINCFLAGS) $(PIPLDFLAGS) pip.c -o pip -pthread -lrt

xpmem: xpmem.c xpmem_eval.h Makefile
	$(CC) -O3 $(XPMINCFLAGS) $(XPMEM_DIR)/lib/libxpmem.o xpmem.c -o xpmem -lrt

shmem: shmem.c xpmem_eval.h Makefile
	$(CC) -O3 -I. -I.. shmem.c -o shmem -lrt

pip-ht: pip.c xpmem_eval.h Makefile
	$(CC) -O3 $(PIPINCFLAGS) $(PIPLDFLAGS) pip.c -o pip-ht -pthread -DHUGETLB -lrt

xpmem-ht: xpmem.c xpmem_eval.h Makefile
	$(CC) -O3 $(XPMINCFLAGS) $(XPMEM_DIR)/lib/libxpmem.o xpmem.c -o xpmem-ht  -DHUGETLB -lrt

clean:
	rm -f $(PROGRAMS) *~

eval:
	echo PIP; ./pip; echo XPMEM; ./xpmem; echo SHMEM; ./shmem
