top_builddir = ..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk

SRCS     = ctxsw.c lock.c pip_couple.c pip_yield.c
PROGRAMS = ctxsw lock pip_couple pip_yield

include $(top_srcdir)/build/rule.mk

CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/test -I.
CFLAGS +=  $(PIECFLAG)
PIPLIBDIR = $(shell (cd $(top_builddir)/lib && pwd))

LDLIBS = -L$(PIPLIBDIR) -L$(libdir) -Wl,-rpath=$(PIPLIBDIR) -lpip -ldl -lpapi

LDFLAGS = -rdynamic -Wl,--dynamic-linker=$(dynamic_linker) \
	$(LDLIBS) -pthread $(PIELDFLAG)

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

all : $(PROGRAMS)
