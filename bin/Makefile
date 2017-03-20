# $PIP_VERSION:$
# $PIP_license:$
# $RIKEN_copyright:$

top_builddir = ..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk

PROGRAM = piprun
PROGRAMS = $(PROGRAM) pmap
MAN1_SRCS = piprun.c pmap.c

CFLAGS += -pthread
CPPFLAGS += -I$(PIPINCDIR) -DPROGRAM=\"$(PROGRAM)\"
LDLIBS += $(PIPLDLIB) -ldl

DEPINCS = $(PIPINCDIR)/pip.h $(PIPINCDIR)/pip_debug.h

include $(top_srcdir)/build/rule.mk

piprun: piprun.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) \
	-Wl,--dynamic-linker=$(dynamic_linker) $< $(LDLIBS) -o $@

pmap: pmap.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) $(CPPFLAGS) -O2 -fpie -pie -rdynamic $< -o $@

post-clean-hook:
	$(RM) *.E *.log
