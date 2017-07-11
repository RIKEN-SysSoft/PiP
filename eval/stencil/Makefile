

top_builddir = ../..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk
include $(srcdir)/../Eval.common

PIPINCFLAGS = -I$(srcdir)/..
PIPCFLAGS  = $(CFLAGS) $(PIEFLAG) $(PIPINCFLAGS) -std=c99

MPICC=mpicc
MPICFLAGS=-std=c99
BINS=stencil_mpi stencil_pip

ICC = icc

all: $(BINS)

stencil_mpi: stencil_pip.c Makefile
	$(MPICC) -O3 $(CPPFLAGS) $(PIPCFLAGS) $< -o $@

stencil_pip: stencil_pip.c Makefile
	$(MPICC) -DPIP -O3 $(CPPFLAGS) $(PIPCFLAGS) $(PIPLDFLAGS) $< -o $@

clean:
	rm -f $(BINS) $(OBJS) *~
	rm -f output*.bmp
