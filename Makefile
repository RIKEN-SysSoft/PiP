# $PIP_VERSION:$
# $PIP_license:$
# $RIKEN_copyright:$

top_builddir = .
top_srcdir = .
srcdir = .

include $(top_srcdir)/build/var.mk

SUBDIRS = lib include bin preload util \
	test/util \
	test/basics \
	test/spawn \
	test/compat \
	test/openmp \
	test/fortran \
	eval sample

include $(top_srcdir)/build/rule.mk

install: doxygen doxygen-install
.PHONY: install

doxygen:
	-@$(RM) -r man html
	@doxy_dirs=`find . -name .doxygen_html | \
		sed 's|/\.doxygen_html$$||' | tr '\012' ' '`; \
	echo ==== man1 =====; \
	( \
	cat $(top_srcdir)/build/common.doxy; \
	echo "STRIP_FROM_PATH = $$doxy_dirs"; \
	echo "GENERATE_HTML = NO"; \
	echo "GENERATE_MAN = YES"; \
	echo "MAN_EXTENSION = 1"; \
	printf "INPUT = "; \
	find . -name .doxygen_man1 -print | while read file; do \
		dir=`dirname $$file`; \
		sed "s|^|$$dir/|" $$file; \
	done | tr '\012' ' '; \
	echo ""; \
	) | doxygen -; \
	echo ==== man3 =====; \
	( \
	cat $(top_srcdir)/build/common.doxy; \
	echo "STRIP_FROM_PATH = $$doxy_dirs"; \
	echo "GENERATE_HTML = NO"; \
	echo "GENERATE_MAN = YES"; \
	echo "MAN_EXTENSION = 3"; \
	printf "INPUT = "; \
	find . -name .doxygen_man3 -print | while read file; do \
		dir=`dirname $$file`; \
		sed "s|^|$$dir/|" $$file; \
	done | tr '\012' ' '; \
	echo ""; \
	) | doxygen -; \
	echo ==== html =====; \
	( \
	cat $(top_srcdir)/build/common.doxy; \
	echo "GENERATE_HTML = YES"; \
	echo "GENERATE_MAN = NO"; \
	echo "MAN_EXTENSION = 3"; \
	echo "STRIP_FROM_PATH = $$doxy_dirs"; \
	printf "INPUT = "; \
	find . -name .doxygen_html -print | while read file; do \
		dir=`dirname $$file`; \
		sed "s|^|$$dir/|" $$file; \
	done | tr '\012' ' '; \
	echo ""; \
	) | doxygen -
.PHONY: doxygen

doxygen-install:
	@$(MKDIR_P) $(DESTDIR)/$(mandir);
	@(cd ./man  && tar cf - . ) | (cd $(DESTDIR)/$(mandir)  && tar xf -)
	@$(MKDIR_P) $(DESTDIR)/$(htmldir);
	@(cd ./html && tar cf - . ) | (cd $(DESTDIR)/$(htmldir) && tar xf -)
.PHONY: doxygen-install

check:
	( cd test && ./test.sh )
.PHONY: check

prog-distclean:
	$(RM) config.log config.status include/config.h build/config.mk
.PHONY: prog-distclean

