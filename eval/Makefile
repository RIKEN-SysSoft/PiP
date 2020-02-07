top_builddir = ..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk

SRCS     = ctxsw.c lock.c pip_yield.c getpid.c pip_getpid.c \
	open-write-close-linux.c open-write-close-pip.c open-write-close-nocpuple.c
PROGRAMS = ctxsw lock pip_yield getpid pip_getpid \
	open-write-close-linux open-write-close-pip open-write-close-nocouple \
	linux-bare aio-bare sched_yield

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
	../test/test.h ./open-write-close.h

DEPLIBS += $(PIPLIBDIR)/libpip.so

PROGRAMS_TO_INSTALL = # nothing

%: %.c IMB_cpu_exploit.o $(DEPINCS) $(DEPLIBS) Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) IMB_cpu_exploit.o $(LDFLAGS) $< -o $@

%: %.cxx $(DEPINCS) $(DEPLIBS) Makefile
	$(CXX) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS)$< -o $@

%: %.f $(DEPINCS) $(DEPLIBS) Makefile
	$(FC) $(CPPFLAGS) $(FFLAGS) $(LDFLAGS) $< -o $@

IMB_cpu_exploit.o : IMB_cpu_exploit.c Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

open-write-close-pip : open-write-close-pip.c IMB_cpu_exploit.o $(DEPINCS) $(DEPLIBS) Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) IMB_cpu_exploit.o $(LDFLAGS) $< -o $@

aio-bare : aio-bare.c open-write-close.h IMB_cpu_exploit.o Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) IMB_cpu_exploit.o -lrt $< -o $@

linux-bare : linux-bare.c open-write-close.h Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

sched_yield : sched_yield.c Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

all : $(PROGRAMS)
