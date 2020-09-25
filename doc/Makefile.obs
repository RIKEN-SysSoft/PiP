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

top_builddir = ..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/build/var.mk

### doxygen

doxygen:
	-@$(RM) -r man latex
	@doxy_dirs=$$(find .. -name .doxygen_latex -print | while read file; do \
		dir=$$(dirname $$file); \
		while read src; do \
			[ -f $$dir/$$src ] && echo $$dir || \
			echo $$top_srcdir/$$dir; \
		done <$$file; done | sort -u | tr '\012' ' ' ); \
	( echo "man7"; \
	  echo "man1"; \
	  echo "man3"; \
	  echo "latex"; \
	) | while read type; do \
		echo ==== $$type =====; \
		( \
		cat "config-common.doxy"; \
		echo ""; \
		cat "config-$$type.doxy"; \
		echo ""; \
		echo "STRIP_FROM_PATH = $$doxy_dirs"; \
		printf "INPUT = "; \
		find .. -name .doxygen_$$type -print | while read file; do \
			dir=$$(dirname $$file); \
			while read src; do \
				[ -f $$dir/$$src ] && echo $$dir/$$src || \
				echo $$top_srcdir/$$dir/$$src; \
			done <$$file; \
		done | tr '\012' ' '; \
		echo ""; \
		) | doxygen -; \
	done
	./markdown.sh "## Overview" 7 "-" > overview.in
	./markdown.sh "## Commands" 1 "-" > commands.in
	./markdown.sh "## Functions" 3 "-" > functions.in
	cat README.md.in overview.in commands.in functions.in Author.md.in > $(top_srcdir)/README.md
	echo ==== README.md =====; \
	( \
		cat "config-common.doxy"; \
		echo ""; \
		cat "config-man7.doxy"; \
		echo ""; \
		echo "STRIP_FROM_PATH = $(top_srcdir)"; \
		printf "INPUT = $(top_srcdir)/README.md"; \
		echo ""; \
	) | doxygen -; \
	if which mandb; then mandb ./man -c -u; fi
	$(MAKE) -C latex-inuse

doc: doxygen

all: doc

install:
	$(MKDIR_P) $(mandir);
	(cd ./man  && tar cf - . ) | (cd $(mandir)  && $(RM) -r * && tar xf -)
	$(MKDIR_P) $(pdfdir);
	cp ./latex-inuse/libpip-manpages.pdf $(pdfdir)/

doc-reset:
	git checkout man latex latex-inuse
.PHONE: doc-reset

clean:
	$(RM) overview.in commannds.in functions.in
	$(RM) *~ \#*\#
	$(MAKE) -C latex-inuse clean

veryclean: clean
	$(RM) overview.in commannds.in functions.in
	$(RM) -r man latex
