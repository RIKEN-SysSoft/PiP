# $PIP_VERSION: Version 1.0$
# $PIP_license: <Simplified BSD License>
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# The views and conclusions contained in the software and documentation
# are those of the authors and should not be interpreted as representing
# official policies, either expressed or implied, of the PiP project.$
# $RIKEN_copyright: 2018 Riken Center for Computational Sceience,
# 	  System Software Devlopment Team. All rights researved$

top_builddir = .
top_srcdir = .
srcdir = .

include $(top_srcdir)/build/var.mk

SUBDIRS = lib include preload util gdbif bin

include $(top_srcdir)/build/rule.mk

MAN7_SRCS = README.md pip-overview.c

doc: doxygen
.PHONY: doc

install: doxygen-install
.PHONY: install

debug:
	CPPFLAGS+="-DDEBUG" $(MAKE) clean all;

### build test programs and run
.PHONY: test-progs
test-progs:
	$(MAKE) -C test test-progs

.PHONY: test
test: all
	$(MAKE) test-progs
	$(MAKE) -C test test

.PHONY: testclean
testclean:
	$(MAKE) -C test testclean

TEST_MKS = build/config.mk build/var.mk build/rule.mk

### install test programs and run
CONFIG_MKS = build/config.mk build/var-install.mk
RULES_MKS  = build/rule.mk

install_test_dirtop = $(prefix)/pip-test
install_test_dir = $(prefix)/pip-test/test
test_build_dir = $(install_test_dirtop)/build

.PHONY: install-test-prepare
install-test-prepare: install
	-$(RM) -r $(install_test_dirtop)
	$(MKDIR_P) $(install_test_dirtop)
	ln -f -s $(prefix)/bin $(install_test_dirtop)
	$(MKDIR_P) $(test_build_dir)
	cat $(CONFIG_MKS) > $(test_build_dir)/var.mk
	$(INSTALL) -C -m 644 $(RULES_MKS) $(test_build_dir)

.PHONY: install-test-programs
install-test-programs: install-test-prepare
	install_test_dir=$(install_test_dir) $(MAKE) -C test do-install-test

.PHONY: do-install-test
do-install-test: install-test-programs
	install_test_dir=$(install_test_dir) $(MAKE) -C $(install_test_dir) test

.PHONY: install-test
install-test: do-install-test

### doxygen

doxygen:
	-@$(RM) -r man html latex
	@doxy_dirs=$$(find . -name .doxygen_html -print | while read file; do \
		dir=$$(dirname $$file); \
		while read src; do \
			[ -f $$dir/$$src ] && echo $$dir || \
			echo $$top_srcdir/$$dir; \
		done <$$file; done | sort -u | tr '\012' ' ' ); \
	( \
	echo "man1 NO  NO YES 1"; \
	echo "man3 YES NO YES 3"; \
	echo "man7 YES NO YES 7"; \
	echo "html YES YES NO 3"; \
	) | while read type repeat_brief html man man_ext; do \
		echo ==== $$type =====; \
		( \
		cat $(top_srcdir)/build/common.doxy; \
		echo "REPEAT_BRIEF = $$repeat_brief"; \
		echo "STRIP_FROM_PATH = $$doxy_dirs"; \
		echo "GENERATE_LATEX = $$html"; \
		echo "GENERATE_HTML = $$html"; \
		echo "GENERATE_MAN = $$man"; \
		echo "MAN_EXTENSION = $$man_ext"; \
		echo "COMPACT_LATEX = NO"; \
		echo "LATEX_BATCHMODE = YES"; \
		echo 'ALIASES = "synopsis=@par Synopsis:\n" "description=@par Description:\n" "notes=@par Notes:\n" \
				"example=@par Example:\n" "bugs=@par Bugs:\n" "environment=@par Environment:\n"'; \
		printf "INPUT = "; \
		find . -name .doxygen_$$type -print | while read file; do \
			dir=$$(dirname $$file); \
			while read src; do \
				[ -f $$dir/$$src ] && echo $$dir/$$src || \
				echo $$top_srcdir/$$dir/$$src; \
			done <$$file; \
		done | tr '\012' ' '; \
		echo ""; \
		) | doxygen -; \
	done
	if which mandb; then mandb ./man -c -u; fi
	cp latex-style/*.sty latex/
	( cd latex; make )
.PHONY: doxygen

doxygen-install:
	$(MKDIR_P) $(mandir);
	(cd ./man  && tar cf - . ) | (cd $(mandir)  && tar xf -)
	$(MKDIR_P) $(htmldir);
	(cd ./html && tar cf - . ) | (cd $(htmldir) && tar xf -)
	$(MKDIR_P) $(pdfdir);
	cp ./latex/refman.pdf $(pdfdir)/libpip-manpages.pdf
.PHONY: doxygen-install

# clean generated documents before "git commit"
docclean:
	git checkout man html latex
.PHONE: docclean

docveryclean:
	$(RM) -r man html latex
.PHONE: docveryclean

post-distclean-hook:
	$(RM) -r man html latex
.PHONY: post-distclean-hook

###

check:
	$(MAKE) test
.PHONY: check

prog-distclean:
	$(RM) config.log config.status include/config.h build/config.mk
.PHONY: prog-distclean

.PHONY: TAGS
TAGS:
	ctags -Re

post-install-hook:
	$(MAKE) -C sample

post-clean-hook:
	$(RM) test.log.* test.out.*
	$(MAKE) -C test clean

post-veryclean-hook:
	$(RM) config.sh lib/fcontext.mk
	$(MAKE) -C sample veryclean
	$(MAKE) -C test veryclean
