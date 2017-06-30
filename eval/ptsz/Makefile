# $PIP_VERSION:$
# $PIP_license:$
# $RIKEN_copyright:$

top_builddir = ../..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk
include $(srcdir)/../Eval.common

PIPINCFLAGS = -I$(srcdir)/..
PIPCFLAGS  = $(CFLAGS) $(PIEFLAG) $(PIPINCFLAGS)

DEPINCS = ../eval.h $(PIPINCDIR)/pip.h $(PIPINCDIR)/pip_machdep.h

SRCS  = mmap.c

PROGRAMS  = mmap-pip mmap-thread mmap-forkonly \
	mmap-pip-ac mmap-thread-ac mmap-forkonly-ac

PROGRAMS_TO_INSTALL = # nothing

include $(top_srcdir)/build/rule.mk

%-pip: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DPIP $(CPPFLAGS) $(PIPCFLAGS) $(PIPLDFLAGS) $< -o $@

%-thread: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DTHREAD $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -pthread $< -o $@

%-forkonly: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DFORK_ONLY $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $< -o $@

%-pip-ac: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DPIP -DACUM $(CPPFLAGS) $(PIPCFLAGS) $(PIPLDFLAGS) $< -o $@

%-thread-ac: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DTHREAD -DACUM $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -pthread $< -o $@

%-forkonly-ac: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DFORK_ONLY -DACUM $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $< -o $@

post-clean-hook:
	$(RM) *.E *.log $(PROGRAMS)

eval:
	( export LD_PRELOAD=$(build_topdir)/preload/pip_preload.so; numactl -C 1 ./mmap-pip 15 1024; numactl -C 1 ./mmap-thread 15 1024; numactl -C 1 ./mmap-forkonly 15 1024 )

eval-ac:
	( export LD_PRELOAD=$(build_topdir)/preload/pip_preload.so; numactl -C 1 ./mmap-pip-ac 15 1024; numactl -C 1 ./mmap-thread-ac 15 1024; numactl -C 1 ./mmap-forkonly-ac 15 1024 )
