# $PIP_VERSION:$
# $PIP_license:$
# $RIKEN_copyright:$

top_builddir = .
top_srcdir = .
srcdir = .

include $(top_srcdir)/build/var.mk

SUBDIRS = lib include bin preload util sample \
	test/util \
	test/basics \
	test/spawn \
	test/compat \
	test/openmp \
	test/pthread \
	test/fortran

include $(top_srcdir)/build/rule.mk

install: doxygen doxygen-install
.PHONY: install

### doxygen

doxygen:
	-@$(RM) -r man html
	@doxy_dirs=$$(find . -name .doxygen_html | while read file; do \
		dir=$$(dirname $$file); \
		while read src; do \
			[ -f $$dir/$$src ] && echo $$dir || \
			echo $$srcdir/$$dir; \
		done <$$file; done | sort -u | tr '\012' ' ' ); \
	( \
	echo "man1 NO  NO YES 1"; \
	echo "man3 YES NO YES 3"; \
	echo "html YES YES NO 3"; \
	) | while read type repeat_brief html man man_ext; do \
		echo ==== $$type =====; \
		( \
		cat $(top_srcdir)/build/common.doxy; \
		echo "REPEAT_BRIEF = $$repeat_brief"; \
		echo "STRIP_FROM_PATH = $$doxy_dirs"; \
		echo "GENERATE_HTML = $$html"; \
		echo "GENERATE_MAN = $$man"; \
		echo "MAN_EXTENSION = $$man_ext"; \
		printf "INPUT = "; \
		find . -name .doxygen_$$type -print | while read file; do \
			dir=`dirname $$file`; \
			while read src; do \
				[ -f $$dir/$$src ] && echo $$dir/$$src || \
				echo $$srcdir/$$dir/$$src; \
			done <$$file; \
		done | tr '\012' ' '; \
		echo ""; \
		) | doxygen -; \
	done
.PHONY: doxygen

doxygen-install:
	@$(MKDIR_P) $(DESTDIR)/$(mandir);
	@(cd ./man  && tar cf - . ) | (cd $(DESTDIR)/$(mandir)  && tar xf -)
	@$(MKDIR_P) $(DESTDIR)/$(htmldir);
	@(cd ./html && tar cf - . ) | (cd $(DESTDIR)/$(htmldir) && tar xf -)
.PHONY: doxygen-install

post-distclean-hook:
	$(RM) -r man html
.PHONY: post-distclean-hook

###

check:
	( cd test && ./test.sh -A )
.PHONY: check

eval:
	( cd eval && make && ./eval.sh )
.PHONY: eval

prog-distclean:
	$(RM) config.log config.status include/config.h build/config.mk
.PHONY: prog-distclean

.PHONY: TAGS

TAGS:
	ctags -Re
