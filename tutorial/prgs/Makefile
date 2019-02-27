# $PIP_VERSION: Version 1.0.1$
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

CC    = cc
MPICC = mpicc

CFLAGS = -O2 -g -I.

PIPLIBDIR = `pwd`/../../lib

PIPCPPFLAGS = -I../../include
PIPCFLAGS   = -fpie -fopenmp
PIPLDFLAGS  = -L$(PIPLIBDIR) -lpip -lpthread -pie -rdynamic \
	-Wl,-rpath=$(PIPLIBDIR)

OMPCFLAGS = -fopenmp

PIPSRCS  = hello-pip.c vars0-pip.c vars1-pip.c vars1-pmap-pip.c vars2-pip.c \
	spawn-pip.c root-pip.c import-pip.c export-pip.c getaddr-pip.c \
	hello-omp-pip.c malloc-pip.c funcall-pip.c rand-pip.c
OMPSRCS  = hello-omp.c vars0-omp.c vars1-omp.c vars1-pmap-omp.c rand-omp.c
MPISRCS  = hello-mpi.c vars0-mpi.c vars1-mpi.c vars1-pmap-mpi.c rand-mpi.c

PIPPRGS  = hello-pip vars0-pip vars1-pip vars1-pmap-pip vars2-pip \
	spawn-pip root-pip import-pip export-pip getaddr-pip \
	hello-omp-pip malloc-pip funcall-pip rand-pip
OMPPRGS  = hello-omp vars0-omp vars1-omp vars1-pmap-omp rand-omp
MPIPRGS  = hello-mpi vars0-mpi vars1-mpi vars1-pmap-mpi rand-mpi

ASMS      = asm.s

PROGRAMS = $(PIPPRGS) $(OMPPRGS) $(MPIPRGS) $(ASMS)

all: $(PROGRAMS)

%-pip: %-pip.c $(PIPSRCS) Makefile
	$(CC) $(CFLAGS) $(PIPCPPFLAGS) $(PIPCFLAGS) $(PIPLDFLAGS) $< -o $@

%-omp: %-omp.c $(OMPSRCS) Makefile
	$(CC) $(CFLAGS) $(OMPCFLAGS) $< -o $@

%-mpi: %-mpi.c $(MPISRCS) Makefile
	$(MPICC) $(CFLAGS)  $< -o $@

asm.s: funcall-pip
	objdump -d $< > $@

clean:
	$(RM) *~ $(PROGRAMS)
