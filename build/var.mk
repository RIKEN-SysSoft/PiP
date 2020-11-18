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
htmldir = ${datarootdir}/html
pdfdir = $(datarootdir)/pdf
slidedir = $(datarootdir)/slides
sysconfdir = $(default_sysconfdir)
localedir = $(default_localedir)

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

PIP_INCDIR    = $(srcdir_top)/include
PIP_LIBDIR    = $(srcdir_top)/lib
PIP_LIBS      = $(srcdir_top)/lib/libpip.so $(srcdir_top)/lib/libpip_init.so
PIP_BINDIR    = $(srcdir_top)/bin

PIP_CPPFLAGS  = -I$(PIP_INCDIR) -I$(GLIBC_INCDIR)
PIP_LDFLAGS   = -L$(PIP_LIBDIR) -Wl,-rpath=$(PIP_LIBDIR) $(GLIBC_LDFLAGS) -lpip -ldl $(PTHREAD_FLAG)

PIP_CFLAGS_ROOT  = $(PIP_CPPFLAGS) $(PTHREAD_FLAG)
PIP_LDFLAGS_ROOT = $(PIP_LDFLAGS) $(DYLINKER_FLAG) $(PTHREAD_FLAG)

PIP_CFLAGS_TASK  = $(PIP_CPPFLAGS) $(PIC_CFLAG)
PIP_LDFLAGS_TASK = $(PIP_LDFLAGS) $(RDYNAMIC_FLAG) $(PTHREAD_FLAG) $(PIE_LDFLAG)

PIP_CFLAGS_BOTH  = $(PIP_CPPFLAGS) $(PIC_CFLAG) $(PTHREAD_FLAG)
PIP_LDFLAGS_BOTH = $(PIP_LDFLAGS) $(RDYNAMIC_FLAG) $(DYLINKER_FLAG) $(PTHREAD_FLAG) $(PIE_LDFLAG)

PIPCC = $(PIP_BINDIR)/pipcc

DEPINCS = $(PIP_INCDIR)/build.h			\
	  $(PIP_INCDIR)/pip_config.h		\
	  $(PIP_INCDIR)/pip.h			\
	  $(PIP_INCDIR)/pip_clone.h		\
	  $(PIP_INCDIR)/pip_dlfcn.h		\
	  $(PIP_INCDIR)/pip_internal.h		\
	  $(PIP_INCDIR)/pip_machdep.h 		\
	  $(PIP_INCDIR)/pip_machdep_aarch64.h 	\
	  $(PIP_INCDIR)/pip_machdep_x86_64.h 	\
	  $(PIP_INCDIR)/pip_gdbif.h		\
	  $(PIP_INCDIR)/pip_gdbif_func.h	\
	  $(PIP_INCDIR)/pip_gdbif_queue.h	\
	  $(PIP_INCDIR)/pip_util.h		\
	  $(PIP_INCDIR)/pip_debug.h

DEPLIBS = $(PIP_LIBDIR)/libpip.so 		\
	  $(PIP_LIBDIR)/libpip_init.so

DEPS = $(DEPINCS) $(DEPLIBS)
