# $PIP_VERSION:$
# $PIP_license:$
# $RIKEN_copyright:$

top_builddir = ..
top_srcdir = $(top_builddir)
srcdir = .

HEADERS = pip.h pip_clone.h pip_debug.h pip_machdep.h \
	  pip_machdep_x86_64.h pip_machdep_aarch64.h
MAN3_SRCS = pip.h

include $(top_srcdir)/build/var.mk
include $(top_srcdir)/build/rule.mk

clean:
	rm -f pip.h
