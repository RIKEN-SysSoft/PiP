# $PIP_VERSION:$
# $PIP_license:$
# $RIKEN_copyright:$

top_builddir = ../..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk
include $(srcdir)/../Test.common

SRCS  = gethostname.c gethostbyname.c initfini.c iftask.c gettimeofday.c \
	ctype.c main.c exit.c brk.c pf.c fork.c

PROGRAMS = gethostname gethostbyname dlopen initfini iftask gettimeofday \
	ctype main exit brk pf fork

PROGRAMS_TO_INSTALL = # nothing

include $(top_srcdir)/build/rule.mk

post-clean-hook:
	$(RM) *.E *.log
