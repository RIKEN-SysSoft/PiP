# $PIP_VERSION:$
# $PIP_license:$
# $RIKEN_copyright:$

top_builddir = ../..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk
include $(srcdir)/../Eval.common

DEPINCS = $(srcdir)/../eval.h $(PIPINCDIR)/pip.h

SRCS  = spawn.c

PROGRAMS  = foo spawn-pip spawn-forkexec spawn-vforkexec spawn-posix spawn-thread

PROGRAMS_TO_INSTALL = # nothing

include $(top_srcdir)/build/rule.mk

foo: foo.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -O2 -fpie -pie -rdynamic $< -o $@

%-pip: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DPIP $(CPPFLAGS) $(CFLAGS) $(PIPLDFLAGS) $< -o $@

%-forkexec: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DFORK_EXEC $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $< -o $@

%-vforkexec: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DVFORK_EXEC $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $< -o $@

%-posix: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DPOSIX_SPAWN $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $< -o $@

%-thread: %.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -DTHREAD $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -pthread $< -o $@

post-clean-hook:
	$(RM) *.E *.log

eval:
	numactl -C 0 ./spawn-pip 50
	numactl -C 0 ./spawn-forkexec 50
	numactl -C 0 ./spawn-vforkexec 50
	numactl -C 0 ./spawn-posix 50
	numactl -C 0 ./spawn-thread 50
