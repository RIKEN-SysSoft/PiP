# $PIP_VERSION:$
# $PIP_license:$
# $RIKEN_copyright:$

top_builddir = .
top_srcdir = .
srcdir = .

include $(top_srcdir)/build/var.mk

SUBDIRS = lib bin preload \
	test/util \
	test/basics \
	test/spawn \
	test/compat \
	test/openmp \
	test/fortran \
	eval sample

include $(top_srcdir)/build/rule.mk

check:
	( cd test && ./test.sh )

prog-distclean:
	$(RM) config.log config.status include/config.h build/config.mk
