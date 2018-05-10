# $PIP_VERSION:$
# $PIP_license:$
# $RIKEN_copyright:$

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
