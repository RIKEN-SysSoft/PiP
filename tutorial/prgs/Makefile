# $PIP_VERSION:$
# $PIP_license:$
# $RIKEN_copyright:$

CC    = cc
MPICC = mpicc

CFLAGS = -O2 -g -I.

PIPCPPFLAGS = -I../../include
PIPCFLAGS   = -fpie
PIPLDFLAGS  = -L../../lib -lpip -lpthread -pie -rdynamic

OMPCFLAGS = -fopenmp

PIPSRCS  = hello-pip.c vars0-pip.c vars1-pip.c vars1-pmap-pip.c vars2-pip.c
OMPSRCS  = hello-omp.c vars0-omp.c vars1-omp.c vars1-pmap-omp.c
MPISRCS  = hello-mpi.c vars0-mpi.c vars1-mpi.c vars1-pmap-mpi.c

PIPPRGS  = hello-pip vars0-pip vars1-pip vars1-pmap-pip vars2-pip
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
