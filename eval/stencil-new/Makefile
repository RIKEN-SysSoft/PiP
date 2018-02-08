
#top_builddir=../..
#top_srcdir=$(top_builddir)
#srcdir=.

##include $(top_srcdir)/build/var.mk
##include $(srcdir)/../Eval.common

PIPINCFLAGS=-I$(srcdir)/.. -I../../include
PIPCFLAGS=$(CFLAGS) -fpie -pie -O3 -std=c99 -rdynamic
PIPLDFLAGS=-L../../lib -lpip

#MPICC=mpicc
MPICCPIP=/work/gg10/share/ahori/PIP/install-pip/bin/mpicc

MPICCMPI=/work/gg10/share/ahori/PIP/install-org/bin/mpicc

BINS=stencil_mpi stencil_pip stencil_mpi_fast stencil_pip_fast stencil_piponly

OBJ=stencil.o

all: $(BINS)

stencil.o: stencil.c Makefile
	$(MPICCMPI) $(PIPCFLAGS) -c $< -o $@

stencil_mpi: stencil_pip.c Makefile $(OBJ)
	$(MPICCMPI) $(PIPCFLAGS) -lnuma $< $(OBJ) -o $@

stencil_pip: stencil_pip.c Makefile $(OBJ)
	$(MPICCPIP) $(PIPCFLAGS) $(PIPLDFLAGS) -lnuma $< $(OBJ) -o $@

stencil_piponly: stencil_pip.c Makefile $(OBJ)
	gcc -DPIP $(PIPINCFLAGS) $(PIPCFLAGS) $(PIPLDFLAGS) -pthread -lnuma $< $(OBJ) -o $@

stencil_mpi_fast: stencil_pip.c Makefile $(OBJ)
	$(MPICCMPI) -DFAST $(PIPCFLAGS) -lnuma $< $(OBJ) -o $@

stencil_pip_fast: stencil_pip.c Makefile $(OBJ)
	$(MPICCPIP) -DFAST $(PIPCFLAGS) -lnuma $< $(OBJ) -o $@

clean:
	rm -f $(BINS) $(OBJ) *~
	rm -f output*.bmp
