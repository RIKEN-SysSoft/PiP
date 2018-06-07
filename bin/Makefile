# $PIP_VERSION:$
# $PIP_license:$
# $RIKEN_copyright:$

top_builddir = ..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk

PROGRAM = piprun
PROGRAMS_TO_INSTALL = $(PROGRAM) pmap
PROGRAMS = $(PROGRAMS_TO_INSTALL) hello
#AH180607#MAN1_SRCS = piprun.c pmap.c hello.c
MAN1_SRCS = piprun.c pmap.c

CFLAGS += -pthread
CPPFLAGS += -I$(PIPINCDIR) -DPROGRAM=\"$(PROGRAM)\"
LDLIBS += $(PIPLDLIB) -ldl

DEPINCS = $(PIPINCDIR)/pip.h $(PIPINCDIR)/pip_util.h

include $(top_srcdir)/build/rule.mk

piprun: piprun.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(LDLIBS) \
	-Wl,--dynamic-linker=$(dynamic_linker) $< -o $@

pmap: pmap.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -I$(PIPINCDIR) $(LDFLAGS) $(LDLIBS) -O2 \
	-fpie -pie -rdynamic $< -o $@ -pthread

hello: hello.c $(SRCS) $(DEPINCS) $(PIPLIB) Makefile
	$(CC) -I$(PIPINCDIR) $(LDFLAGS) $(LDLIBS) -O2 \
	-fpie -pie -rdynamic $< -o $@ -pthread

post-clean-hook:
	$(RM) *.E *.log
