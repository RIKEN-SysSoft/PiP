
# $PIP_license: <Simplified BSD License>
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#     Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#
#     Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.
# $
# $RIKEN_copyright: Riken Center for Computational Sceience (R-CCS),
# System Software Development Team, 2016-2020
# $
# $PIP_VERSION: Version 2.0.0$
#
# $Author: Atsushi Hori (R-CCS) mailto: ahori@riken.jp or ahori@me.com
# $

top_builddir = .
top_srcdir = .
srcdir = .

include $(top_srcdir)/build/var.mk

SUBDIRS = include/pip lib preload gdbif bin

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

### doc

doc-install:
	$(MAKE) -C doc
.PHONY: doc-install

doc: doc-install
.PHONY: doc

doc-reset:
	$(MAKE) -C doc doc-reset
.PHONE: doc-reset

doc-clean:
	$(MAKE) -C doc clean
.PHONE: docclean

post-documents-hook:
	$(MAKE) -C doc documents

###

check:
	$(MAKE) test
.PHONY: check

post-clean-hook:
	$(RM) test.log.* test.out.*
	$(MAKE) -C test clean

post-veryclean-hook: subdir-veryclean

post-distclean-hook:
	$(MAKE) -C doc post-distclean-hook
	$(RM) config.log config.status include/pip_config.h
	$(RM) release/version.conf
	$(RM) build/config.mk
.PHONY: post-distclean-hook

.PHONY: TAGS
TAGS:
	ctags -Re
