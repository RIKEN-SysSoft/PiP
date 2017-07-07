# $PIP_VERSION:$
# $PIP_license:$
# $RIKEN_copyright:$

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
