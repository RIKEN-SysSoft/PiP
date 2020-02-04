top_builddir = ..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk

SRCS     = ctxsw.c lock.c pip_yield.c getpid.c pip_getpid.c \
	open-write-close-linux.c open-write-close-pip.c open-write-close-nocpuple.c
PROGRAMS = ctxsw lock pip_yield getpid pip_getpid \
	open-write-close-linux open-write-close-pip open-write-close-nocouple \
	aio-bare sched_yield

include $(top_srcdir)/build/rule.mk

CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/test -I.
CFLAGS +=  $(PIECFLAG)
PIPLIBDIR = $(shell (cd $(top_builddir)/lib && pwd))

LDLIBS = -L$(PIPLIBDIR) -L$(libdir) -Wl,-rpath=$(PIPLIBDIR) -lpip -ldl

LDFLAGS = -rdynamic -Wl,--dynamic-linker=$(dynamic_linker) \
	$(LDLIBS) -pthread $(PIELDFLAG) -lrt

DEPINCS = $(PIPINCDIR)/pip.h $(PIPINCDIR)/pip_blt.h  \
	$(PIPINCDIR)/pip_internal.h $(PIPINCDIR)/pip_machdep.h \
	$(PIPINCDIR)/pip_debug.h $(PIPINCDIR)/pip_util.h \
	../test.h

DEPLIBS += $(PIPLIBDIR)/libpip.so

PROGRAMS_TO_INSTALL = # nothing

%: %.c $(SRCS) $(DEPINCS) $(PIPLIB) $(DEPLIBS) Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $< -o $@

%: %.cxx $(SRCS) $(DEPINCS) $(PIPLIB) $(DEPLIBS) Makefile
	$(CXX) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS)$< -o $@

%: %.f $(SRCS) $(DEPINCS) $(PIPLIB) $(DEPLIBS) Makefile
	$(FC) $(CPPFLAGS) $(FFLAGS) $(LDFLAGS) $< -o $@

aio-bare : aio-bare.c Makefile
	$(CC) -g -O2 -lrt $< -o $@

sched_yield : sched_yield.c Makefile
	$(CC) -g -O2 -pthread $< -o $@

all : $(PROGRAMS)
