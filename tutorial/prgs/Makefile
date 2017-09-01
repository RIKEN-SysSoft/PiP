# $PIP_VERSION:$
# $PIP_license:$
# $RIKEN_copyright:$

CC    = cc
MPICC = mpicc

CFLAGS = -O2 -g -I.

PIPLIBDIR = `pwd`/../../lib

PIPCPPFLAGS = -I../../include
PIPCFLAGS   = -fpie
PIPLDFLAGS  = -L$(PIPLIBDIR) -lpip -lpthread -pie -rdynamic \
	-Wl,-rpath=$(PIPLIBDIR)

OMPCFLAGS = -fopenmp

PIPSRCS  = hello-pip.c vars0-pip.c vars1-pip.c vars1-pmap-pip.c vars2-pip.c \
	spawn-pip.c root-pip.c import-pip.c export-pip.c getaddr-pip.c
OMPSRCS  = hello-omp.c vars0-omp.c vars1-omp.c vars1-pmap-omp.c
MPISRCS  = hello-mpi.c vars0-mpi.c vars1-mpi.c vars1-pmap-mpi.c

PIPPRGS  = hello-pip vars0-pip vars1-pip vars1-pmap-pip vars2-pip \
	spawn-pip root-pip import-pip export-pip getaddr-pip
OMPPRGS  = hello-omp vars0-omp vars1-omp vars1-pmap-omp
MPIPRGS  = hello-mpi vars0-mpi vars1-mpi vars1-pmap-mpi

PROGRAMS = $(PIPPRGS) $(OMPPRGS) $(MPIPRGS)

all: $(PROGRAMS)

%-pip: %-pip.c $(PIPSRCS) Makefile
	$(CC) $(CFLAGS) $(PIPCPPFLAGS) $(PIPCFLAGS) $(PIPLDFLAGS) $< -o $@

%-omp: %-omp.c $(OMPSRCS) Makefile
	$(CC) $(CFLAGS) $(OMPCFLAGS) $< -o $@

%-mpi: %-mpi.c $(MPISRCS) Makefile
	$(MPICC) $(CFLAGS)  $< -o $@

clean:
	$(RM) *~ $(PROGRAMS)
