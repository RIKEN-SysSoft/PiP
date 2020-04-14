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
CFLAGS   = $(DEFAULT_CFLAGS)
CXXFLAGS = $(DEFAULT_CFLAGS)
FFLAGS   = $(DEFAULT_FFLAGS)
LDFLAGS  = $(DEFAULT_LDFLAGS)
LIBS     = $(DEFAULT_LIBS)

INCLUDE_BUILDDIR = $(top_builddir)/include
INCLUDE_SRCDIR   = $(top_srcdir)/include

INSTALL = $(DEFAULT_INSTALL)
INSTALL_PROGRAM = $(DEFAULT_INSTALL_PROGRAM)
INSTALL_SCRIPT  = $(DEFAULT_INSTALL_SCRIPT)
INSTALL_DATA    = $(DEFAULT_INSTALL_DATA)
INSTALL_DATTEST = $(DEFAULT_INSTALL_TEST)

MKDIR_P = mkdir -p
RM = rm -f
FC = gfortran

PROGRAMS = $(PROGRAM)
PROGRAMS_TO_INSTALL = $(PROGRAMS)
LIBRARIES = $(LIBRARY)

PICCFLAG  = -fPIC
PIECFLAG  = -fPIE
PIELDFLAG = -pie

GLIBCINCDIR = $(glibc_incdir)
GLIBCLIBDIR = $(glibc_libdir)
GLIBCRPATH  = -Wl,-rpath=$(GLIBCLIBDIR)

PIPINCDIR_BUILD = $(srcdir_top)/include
PIPLIBDIR_BUILD = $(srcdir_top)/lib
PIPLIB_BUILD    = $(srcdir_top)/lib/libpip.so

PIPINCDIR_INSTALL = $(prefix)/include
PIPLIBDIR_INSTALL = $(prefix)/lib
PIPLIB_INSTALL    = $(prefix)/lib/libpip.so

PIPRPATH    = -Wl,-rpath=$(PIPLIBDIR_BUILD) -Wl,-rpath=$(PIPLIBDIR_INSTALL)
PIPDYLINKER = -Wl,--dynamic-linker=$(dynamic_linker)
PIPRDYNAMIC = -rdynamic

PIPCPPFLAGS     = -I$(GLIBCINCDIR) -I$(PIPINCDIR_BUILD) -I$(PIPINCDIR_INSTALL)
PIPCFLAGS_ROOT  = $(PTHREADFLAG)
PIPCFLAGS_TASK  = $(PICCFLAG) $(PTHREADFLAG)
PIPLDLIBS       = -L$(PIPLIBDIR_BUILD) -L$(PIPLIBDIR_INSTALL) $(PIPRPATH) -L$(GLIBCLIBDIR) $(GLIBCRPATH) -lpip -ldl
PIPLDFLAGS_ROOT = $(PIPLDLIBS) $(PIPDYLINKER) $(PTHREADFLAG)
PIPLDFLAGS_TASK = $(PIELDFLAG) $(PIPRDYNAMIC) $(PIPDYLINKER) $(PIPLDLIBS) $(PTHREADFLAG) $(PICLDFLAG)
PIPLDFLAGS_BOTH = $(PIELDFLAG) $(PIPRDYNAMIC) $(PIPDYLINKER) $(PIPLDLIBS) $(PTHREADFLAG) $(PICLDFLAG)
