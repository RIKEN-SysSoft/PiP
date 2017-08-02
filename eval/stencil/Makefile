

top_builddir = ../..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk
include $(srcdir)/../Eval.common

PIPINCFLAGS = -I$(srcdir)/..
PIPCFLAGS  = $(CFLAGS) $(PIEFLAG) $(PIPINCFLAGS) -std=c99

MPICC=mpicc
BINS=stencil_mpi stencil_pip stencil_mpi_fast stencil_pip_fast

OBJ=stencil.o

all: $(BINS)

stencil.o: stencil.c Makefile
	$(MPICC) -O3 $(CPPFLAGS) -std=c99 -fpie -c $< -o $@

stencil_mpi: stencil_pip.c Makefile $(OBJ)
	$(MPICC) -O3 $(CPPFLAGS) $(PIPCFLAGS) -lnuma $< $(OBJ) -o $@

stencil_pip: stencil_pip.c Makefile $(OBJ)
	$(MPICC) -DPIP -O3 $(CPPFLAGS) $(PIPCFLAGS) $(PIPLDFLAGS) -lnuma $< $(OBJ) -o $@

stencil_mpi_fast: stencil_pip.c Makefile $(OBJ)
	$(MPICC) -O3 -DFAST $(CPPFLAGS) $(PIPCFLAGS) -lnuma $< $(OBJ) -o $@

stencil_pip_fast: stencil_pip.c Makefile $(OBJ)
	$(MPICC) -DPIP -DFAST -O3 $(CPPFLAGS) $(PIPCFLAGS) $(PIPLDFLAGS) -lnuma $< $(OBJ) -o $@

clean:
	rm -f $(BINS) $(OBJ) *~
	rm -f output*.bmp
