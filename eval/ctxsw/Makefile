# $PIP_VERSION:$
# $PIP_license:$
# $RIKEN_copyright:$

top_builddir = ../..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk
include $(srcdir)/../Eval.common

PIPINCFLAGS = -I$(srcdir)/..
PIPCFLAGS   = $(PIEFLAG) $(PIPINCFLAGS)

DEPINCS = ../eval.h $(PIPINCDIR)/pip.h $(PIPINCDIR)/pip_machdep.h

SRCS  = ctxsw.c futex.c

PROGRAMS  = ctxsw-pip ctxsw-thread ctxsw-forkonly \
	futex-pip futex-thread futex-forkonly

PROGRAMS_TO_INSTALL = # nothing

include $(top_srcdir)/build/rule.mk

%-pip: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DPIP $(CPPFLAGS) $(PIPCFLAGS) -O3 -lrt -g $(PIPLDFLAGS) $< -o $@

%-thread: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DTHREAD -pthread $(CPPFLAGS) -O3 -lrt -g $(LDFLAGS) $< -o $@

%-forkonly: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DFORK_ONLY $(CPPFLAGS) -O3 -lrt -g $(LDFLAGS) $< -o $@

post-clean-hook:
	$(RM) *.E *.log

eval:
	( export LD_PRELOAD=$(PIPLIBDIR)/../preload/pip_preload.so; \
	echo -n "PIP:     "; numactl -C 0 ./ctxsw-pip      15 1024; \
	echo -n "PTHREAD: "; numactl -C 0 ./ctxsw-thread   15 1024; \
	echo -n "FORK:    "; numactl -C 0 ./ctxsw-forkonly 15 1024 )
