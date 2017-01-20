# $PIP_VERSION:$
# $PIP_license:$
# $RIKEN_copyright:$

top_builddir = ../..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk
include $(srcdir)/../Test.common

CXXFLAGS += $(CFLAGS)
LDFLAGS +=

SRCS  = cxx.cxx

PROGRAMS  = cxx

PROGRAMS_TO_INSTALL = # nothing

include $(top_srcdir)/build/rule.mk
