VPATH=$(srcdir)

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
CP = cp -f
MV = mv
FC = gfortran

PROGRAMS = $(PROGRAM)
PROGRAMS_TO_INSTALL = $(PROGRAMS)
LIBRARIES = $(LIBRARY)

PIC_CFLAG     = -fPIC
PIE_CFLAG     = -fPIE
PIC_LDFLAG    = -pic
PIE_LDFLAG    = -pie
RDYNAMIC_FLAG = -rdynamic
PTHREAD_FLAG  = -pthread
DYLINKER_FLAG = -Wl,--dynamic-linker=$(dynamic_linker)

GLIBC_INCDIR  = $(glibc_incdir)
GLIBC_LIBDIR  = $(glibc_libdir)
GLIBC_LDFLAGS = -L$(glibc_libdir) -Wl,-rpath=$(GLIBCLIBDIR)

PIP_INCDIR    = $(prefix)/include
PIP_LIBDIR    = $(prefix)/lib
PIP_LIBS      = $(prefix)/lib/libpip.so $(prefix)/lib/libpip_init.so

PIP_CPPFLAGS  = -I$(PIP_INCDIR) -I$(GLIBC_INCDIR)
PIP_LDFLAGS   = -L$(PIP_LIBDIR) -Wl,-rpath=$(PIP_LIBDIR) $(GLIBC_LDFLAGS) -lpip -ldl $(PTHREAD_FLAG)

PIP_CFLAGS_ROOT  = $(PIP_CPPFLAGS) $(PTHREAD_FLAG)
PIP_LDFLAGS_ROOT = $(PIP_LDFLAGS) $(DYLINKER_FLAG) $(PTHREAD_FLAG)

PIP_CFLAGS_TASK  = $(PIP_CPPFLAGS) $(PIC_CFLAG)
PIP_LDFLAGS_TASK = $(PIP_LDFLAGS) $(RDYNAMIC_FLAG) $(PTHREAD_FLAG) $(PIE_LDFLAG)

PIP_CFLAGS_BOTH  = $(PIP_CPPFLAGS) $(PIC_CFLAG) $(PTHREAD_FLAG)
PIP_LDFLAGS_BOTH = $(PIP_LDFLAGS) $(RDYNAMIC_FLAG) $(DYLINKER_FLAG) $(PTHREAD_FLAG) $(PIE_LDFLAG)