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

.PHONY: check-installed-prepare
check-installed-prepare: install
	-$(RM) -r $(install_test_dirtop)
	$(MKDIR_P) $(install_test_dirtop)
	ln -f -s $(prefix)/bin $(install_test_dirtop)
	$(MKDIR_P) $(test_build_dir)
	cat $(CONFIG_MKS) > $(test_build_dir)/var.mk
	$(INSTALL) -C -m 644 $(RULES_MKS) $(test_build_dir)

.PHONY: check-installed-programs
check-installed-programs: check-installed-prepare
	install_test_dir=$(install_test_dir) $(MAKE) -C test do-install-test

.PHONY: do-check-installed
do-check-installed: check-installed-programs
	install_test_dir=$(install_test_dir) $(MAKE) -C $(install_test_dir) test

.PHONY: check-installed
check-installed: do-check-installed

### doc

doxygen : doc
	$(MAKE) -C doc
.PHONY: doc

doc-install :
	$(MAKE) -C doc install
.PHONY: doc-install

doc-reset:
	$(MAKE) -C doc doc-reset
.PHONE: doc-reset

docclean:
	$(MAKE) -C doc docclean
.PHONE: docclean

post-distclean-hook:
	$(MAKE) -C doc post-distclean-hook
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
