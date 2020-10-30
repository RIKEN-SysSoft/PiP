# $PIP_VERSION: Version 3.0.0$
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
# $RIKEN_copyright: Riken Center for Computational Sceience,
# System Software Development Team, 2016, 2017, 2018, 2019, 2020$

top_builddir = ..
top_srcdir = $(top_builddir)
srcdir = .

MAN7_SRCS = README-html.md

include $(top_srcdir)/build/var.mk
include $(top_srcdir)/build/rule.mk

all: install

### doxygen

documents:
	-@$(RM) -r man latex html
	echo -n "" > doxygen-man713.log
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
		) | doxygen - >> doxygen-man713.log 2>&1; \
	done
	-@grep -e "warning" -e "error" doxygen-man713.log && ( echo "Doxygen failed"; exit 1; )
	./markdown.sh "# Commands"  1 "-" > commands.in
	./markdown.sh "# Functions" 3 "-" > functions.in
	cat README.md.in commands.in functions.in Slides-md.md Author-md.md > README.md
	cat README-man.doxy  README.md.in commands.in functions.in Author-md.md > README-man.md
	cat README-html.doxy README.md.in Slides-html.md Author-html.md > README-html.md
	@echo ==== README.md =====;
	echo -n "" > doxygen-readme.log
	( \
		cat "config-common.doxy"; \
		echo ""; \
		cat "config-man7.doxy"; \
		echo ""; \
		echo "STRIP_FROM_PATH = $(top_srcdir)"; \
		printf "INPUT = README-man.md"; \
		echo ""; \
	) | doxygen - >> doxygen-readme.log 2>&1
	-@grep -e "warning" -e "error" doxygen-readme.log && ( echo "Doxygen failed"; exit 1; )
	$(MAKE) -C latex-inuse
	$(CP) README.md $(top_srcdir)
	@echo ==== html =====;
	echo -n "" > doxygen-html.log
	( \
	cat "config-common.doxy"; \
	echo ""; \
	cat "config-html.doxy"; \
	echo ""; \
	echo "STRIP_FROM_PATH = $$doxy_dirs"; \
	printf "INPUT = "; \
	find .. -name .doxygen_latex -print | while read file; do \
		dir=$$(dirname $$file); \
		while read src; do \
			[ -f $$dir/$$src ] && echo $$dir/$$src || \
			echo $$top_srcdir/$$dir/$$src; \
		done <$$file; \
	done | tr '\012' ' '; \
	) | doxygen - >> doxygen-html.log 2>&1;
	-@grep -e "warning" -e "error" doxygen-html.log && ( echo "Doxygen failed"; exit 1; )
	@echo ==== mandb =====; \
	if which mandb; then mandb ./man -c -u; fi
.PHONY: doxygen

install:
	@echo "Installing man pages ->" $(mandir)
	@$(MKDIR_P) $(mandir);
	@(cd ./man  && tar cf - . ) | (cd $(mandir) && $(RM) -r * && tar xf -)
	@echo "Installing pdf ->" $(pdfdir)
	@$(MKDIR_P) $(pdfdir);
	@$(CP) ./latex-inuse/libpip-manpages.pdf $(pdfdir)
	@$(MKDIR_P) $(slidedir);
	@echo "Installing html ->" $(htmldir)
	@$(MKDIR_P) $(htmldir);
	@(cd ./html && tar cf - . ) | (cd $(htmldir) && $(RM) -r * && tar xf -)
	@$(CP) ./slides/* $(slidedir)
.PHONY: doc-install

doc-reset:
	git checkout man latex latex-inuse html slides
.PHONE: doc-reset

clean:
	$(RM) commannds.in functions.in
	$(RM) doxygen*.log
	$(RM) *~ \#*\#
	$(MAKE) -C latex-inuse clean

veryclean: clean
	$(RM) Slides-html.md
	$(MAKE) -C latex-inuse veryclean
