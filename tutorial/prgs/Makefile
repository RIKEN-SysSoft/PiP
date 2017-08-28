# $PIP_VERSION:$
# $PIP_license:$
# $RIKEN_copyright:$

CC    = cc
MPICC = mpicc

CFLAGS = -O2 -g

PIPCPPFLAGS = -I../../include
PIPCFLAGS   = -fpie
PIPLDFLAGS  = -L../../lib -lpip -lpthread -pie -rdynamic

OMPCFLAGS = -fopenmp

PIPSRCS  = hello-pip.c vars0-pip.c
OMPSRCS  = hello-omp.c vars0-omp.c
MPISRCS  = hello-mpi.c vars0-mpi.c

PIPPRGS  = hello-pip vars0-pip
OMPPRGS  = hello-omp vars0-omp
MPIPRGS  = hello-mpi vars0-mpi

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
