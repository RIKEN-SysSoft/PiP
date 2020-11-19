
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
# $PIP_VERSION: Version 1.2.0$
#
# $Author: Atsushi Hori (R-CCS) mailto: ahori@riken.jp or ahori@me.com
# $

top_builddir = .
top_srcdir = .
srcdir = .

include $(top_srcdir)/build/var.mk

SUBDIRS = lib include gdbif bin preload sample \
	test/util \
	test/basics \
	test/spawn \
	test/compat \
	test/openmp \
	test/pthread \
	test/fortran \
	test/errno

include $(top_srcdir)/build/rule.mk

doc: doc-install
.PHONY: doc

install: doc-install
.PHONY: install

debug:
	CPPFLAGS="-DDEBUG" make all;

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

doc-install:
	$(MKDIR_P) $(DESTDIR)/$(mandir);
	(cd ./man  && tar cf - . ) | (cd $(DESTDIR)/$(mandir)  && tar xf -)
	$(MKDIR_P) $(DESTDIR)/$(htmldir);
	(cd ./html && tar cf - . ) | (cd $(DESTDIR)/$(htmldir) && tar xf -)
.PHONY: doc-install

# clean generated documents before "git commit"
docclean:
	git checkout html latex man

###

check:
	( cd test && ./test.sh -A )
.PHONY: check

eval:
	( cd eval && make && ./eval.sh )
.PHONY: eval

post-distclean-hook:
	$(RM) config.log config.status include/pip_config.h
	$(RM) build/config.mk release/version.conf

.PHONY: TAGS

TAGS:
	ctags -Re
