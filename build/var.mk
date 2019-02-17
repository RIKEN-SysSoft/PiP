VPATH=$(srcdir)

include $(top_builddir)/build/config.mk

sbindir = $(default_sbindir)
bindir = $(default_bindir)
libdir = $(default_libdir)
libexecdir = $(default_libexecdir)
includedir = $(default_includedir)
exec_includedir = $(default_exec_includedir)
datadir = $(default_datadir)
datarootdir = $(default_datarootdir)
mandir = $(default_mandir)
docdir = $(default_docdir)
htmldir = $(default_htmldir)
sysconfdir = $(default_sysconfdir)
localedir = $(default_localedir)

PTHREADFLAG = -pthread

CC = $(DEFAULT_CC)
CFLAGS = $(DEFAULT_CFLAGS) $(PTHREADFLAG)
LDFLAGS = $(DEFAULT_LDFLAGS)
LIBS = $(DEFAULT_LIBS)
INCLUDE_BUILDDIR = $(top_builddir)/include
INCLUDE_SRCDIR = $(top_srcdir)/include

INSTALL = $(DEFAULT_INSTALL)
INSTALL_PROGRAM = $(DEFAULT_INSTALL_PROGRAM)
INSTALL_SCRIPT = $(DEFAULT_INSTALL_SCRIPT)
INSTALL_DATA = $(DEFAULT_INSTALL_DATA)
MKDIR_P = mkdir -p
RM = rm -f
FC = gfortran

PROGRAMS = $(PROGRAM)
PROGRAMS_TO_INSTALL = $(PROGRAMS)
LIBRARIES = $(LIBRARY)

PICCFLAG   = -fPIC
PIECFLAG  = -fpie
PIELDFLAG = -pie

PIPINCDIR = $(top_srcdir)/include
PIPLIBDIR = $(top_builddir)/lib
PIPLIB = $(top_builddir)/lib/libpip.so

PIPLDLIB = -L$(top_builddir)/lib -Wl,-rpath=$(libdir) -lpip

PIPLDFLAGS = -pthread -pie -rdynamic -Wl,--dynamic-linker=$(dynamic_linker) \
	$(PIPLDLIB) -ldl $(PIELDFLAG)
