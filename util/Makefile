# $PIP_VERSION:$
# $PIP_license:$
# $RIKEN_copyright:$

top_builddir = ..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk

PROGRAMS_TO_INSTALL = piplnlibs
MAN1_SRCS = piplnlibs
PROGRAMS = pip_gdbif_offsets
SRCS = pip_gdbif_offsets.c
OBJS = pip_gdbif_offsets.o
CPPFLAGS = -I$(PIPINCDIR)
DEPINC = \
	   $(PIPINCDIR)/pip_gdbif.h		\
	   $(PIPINCDIR)/pip_machdep.h	 	\
	   $(PIPINCDIR)/pip_machdep_aarch64.h 	\
	   $(PIPINCDIR)/pip_machdep_x86_64.h 	\
	   $(PIPINCDIR)/pip_queue.h

all: $(PROGRAMS)

pip_gdbif_offsets: pip_gdbif_offsets.o
pip_gdbif_offsets.o: $(DEPINC)

include $(top_srcdir)/build/rule.mk
