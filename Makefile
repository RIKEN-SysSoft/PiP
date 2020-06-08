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

doc: doxygen
.PHONY: doc

install: doxygen-install
.PHONY: install

debug:
	CPPFLAGS+="-DDEBUG" make clean all;

### build test programs and run
.PHONY: test-progs
test-progs:
	make -C test test-progs

.PHONY: test
test: all
	make test-progs
	make -C test test

.PHONY: testclean
testclean:
	make -C test testclean

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
	$(MKDIR_P) $(test_build_dir)
	cat $(CONFIG_MKS) > $(test_build_dir)/var.mk
	$(INSTALL) -C -m 744 $(RULES_MKS) $(test_build_dir)
	$(MKDIR_P) $(install_test_dir)
	ln -f -s $(prefix)/bin $(install_test_dirtop)

.PHONY: install-test-programs
install-test-programs: install-test-prepare
	install_test_dir=$(install_test_dir) make -C test install-test

.PHONY: do-install-test
do-install-test: install-test-programs
	install_test_dir=$(install_test_dir) make -C $(install_test_dir) install-test-all
	install_test_dir=$(install_test_dir) make -C $(install_test_dir) test

.PHONY: install-test
install-test: do-install-test

### doxygen

doxygen:
	-@$(RM) -r man html latex
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
		echo "GENERATE_LATEX = $$html"; \
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
	$(MKDIR_P) $(DESTDIR)/$(mandir);
	(cd ./man  && tar cf - . ) | (cd $(DESTDIR)/$(mandir)  && tar xf -)
	$(MKDIR_P) $(DESTDIR)/$(htmldir);
	(cd ./html && tar cf - . ) | (cd $(DESTDIR)/$(htmldir) && tar xf -)
.PHONY: doxygen-install

# clean generated documents before "git commit"
docclean:
	git checkout html latex man

post-distclean-hook:
	$(RM) -r man html
.PHONY: post-distclean-hook

###

check:
	make test
.PHONY: check

eval:
	( cd eval && make && ./eval.sh )
.PHONY: eval

prog-distclean:
	$(RM) config.log config.status include/config.h build/config.mk
.PHONY: prog-distclean

.PHONY: TAGS

tags:
	ctags -Re

post-install-hook:
	make -C sample

post-clean-hook:
	$(RM) test.log.* test.out.*
	make -C test clean
	make -C test/scripts clean
	make -C test/util clean
	make -C test/prog clean
	make -C test/basics clean
	make -C test/compat clean
	make -C test/pthread clean
	make -C test/openmp clean
	make -C	test/cxx clean
	make -C test/fortran clean
	make -C test/issues clean
	make -C test/blt clean

post-veryclean-hook:
	$(RM) config.sh lib/fcontext.mk
	make -C sample veryclean
	make -C test veryclean
	make -C test/scripts veryclean
	make -C test/util veryclean
	make -C test/prog veryclean
	make -C test/basics veryclean
	make -C test/compat veryclean
	make -C test/pthread veryclean
	make -C test/openmp veryclean
	make -C	test/cxx veryclean
	make -C test/fortran veryclean
	make -C test/issues veryclean
	make -C test/blt veryclean
