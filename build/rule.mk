# default target is "all"

all: subdir-all header-all lib-all prog-all post-all-hook
.PHONY: all

install: all \
	pre-install-hook \
	subdir-install header-install lib-install prog-install \
	post-install-hook
.PHONY: install

clean: subdir-clean header-clean lib-clean prog-clean \
	misc-clean post-clean-hook
.PHONY: clean

testclean: \
	subdir-testclean testclean-here post-testclean-hook
.PHONY: testclean

veryclean: clean testclean \
	subdir-veryclean header-veryclean lib-veryclean prog-veryclean \
	post-veryclean-hook
.PHONY: veryclean

distclean: veryclean \
	subdir-distclean header-veryclean lib-distclean prog-distclean \
	distclean-here post-distclean-hook
.PHONY: distclean

documents: subdir-documents doc-here post-documents-hook
.PHONY: documents

misc-clean:
	$(RM) *.E *.S
	$(RM) \#*\# .\#* *~
	$(RM) core.*
	-$(RM) .nfs*
.PHONY: misc-clean

distclean-here:
	@if [ -f $(srcdir)/Makefile.in ]; then \
		$(RM) Makefile; \
	fi
	$(RM) .doxygen_*
.PHONY: distclean-here

testclean-here:
	$(RM) *.log.xml
	$(RM) test.log test.log.* test.out.* .test-sum-*.sh
	$(RM) loop-*.log loop-*.log~ .loop-*.log \#loop-*.log\# .\#loop-*.log
	$(RM) core.*
	$(RM) seek-file.text
.PHONY: testclean-here

.PHONY: check-installed

### subdir rules

subdir-all subdir-debug subdir-install \
subdir-clean subdir-veryclean subdir-distclean \
subdir-testclean subdir-check-installed \
subdir-documents:
	@target=`expr $@ : 'subdir-\(.*\)'`; \
	for dir in -- $(SUBDIRS); do \
		case $${dir} in --) continue;; esac; \
		echo '[' making $${dir} ']'; \
		case $(top_srcdir) in \
		/*)	top_srcdir=$(top_srcdir); \
			srcdir=$(srcdir)/$${dir};; \
		*)	rel=`echo $${dir}|sed 's|[^/][^/]*|..|g'`; \
			top_srcdir=$${rel}/$(top_srcdir); \
			srcdir=$${rel}/$(srcdir)/$${dir};; \
		esac; \
		if test -f $(srcdir)/$${dir}/Makefile.in; then \
			( cd $${dir} && \
			  case "$(srcdir)" in \
			  .)	$(MAKE) $${target};;\
			  *)	$(MAKE) top_srcdir=$${top_srcdir} \
					srcdir=$${srcdir} \
					$${target};; \
			  esac; \
			) || exit 1; \
		else \
			test -d $${dir} || $(MKDIR_P) $${dir} || exit 1; \
			( cd $${dir} && \
			  case "$(srcdir)" in \
			  .)	$(MAKE) $${target};;\
			  *)	$(MAKE) -f $${srcdir}/Makefile \
					top_srcdir=$${top_srcdir} \
					srcdir=$${srcdir} \
					$${target};; \
			  esac; \
			) || exit 1; \
		fi; \
	done
.PHONY: subdir-all subdir-debug subdir-install subdir-documents \
	subdir-clean subdir-veryclean subdir-distclean subdir-testclean

### header rules

header-all: $(HEADERS)
.PHONY: header-all

header-install:
	@for i in -- $(HEADERS); do \
		case $$i in --) continue;; esac; \
		$(MKDIR_P) $(includedir)/pip; \
		echo \
		$(INSTALL_DATA) $(srcdir)/$$i $(includedir)/pip/$$i; \
		$(INSTALL_DATA) $(srcdir)/$$i $(includedir)/pip/$$i; \
	done
	@for i in -- $(EXEC_HEADERS); do \
		case $$i in --) continue;; esac; \
		$(MKDIR_P) $(exec_includedir)/pip; \
		echo \
		$(INSTALL_DATA) $$i $(includedir)/pip/$$i; \
		$(INSTALL_DATA) $$i $(includedir)/pip/$$i; \
	done
.PHONY: header-install

header-clean:
.PHONY: header-clean

header-veryclean:
.PHONY: header-veryclean

header-distclean:
	@case "$(EXEC_HEADERS)" in \
	'')	;; \
	*)	echo; \
		$(RM) $(EXEC_HEADERS); \
		$(RM) $(EXEC_HEADERS);; \
	esac
.PHONY: header-distclean

header-testclean:

.PHONY: header-testclean

### lib rules

lib-all: $(LIBRARIES)
.PHONY: lib-all

lib-install:
	@for i in -- $(LIBRARIES); do \
		case $$i in --) continue;; esac; \
		$(MKDIR_P) $(libdir); \
		echo \
		$(INSTALL_DATA) $$i $(libdir)/$$i; \
		$(INSTALL_DATA) $$i $(libdir)/$$i; \
	done
.PHONY: lib-install

lib-clean: obj-clean
.PHONY: lib-clean

lib-veryclean:
	@case "$(LIBRARIES)" in \
	'')	;; \
	*)	echo \
		$(RM) $(LIBRARIES); \
		$(RM) $(LIBRARIES);; \
	esac
.PHONY: lib-veryclean

lib-distclean:
.PHONY: lib-distclean

lib-testclean:
	$(RM) *.log *.log.xml
	$(RM) test.log test.log.* test.out.*
	$(RM) .test-sum-*.sh
	$(RM) loop-*.log .loop-*.log
	$(RM) seek-file.text
.PHONY: lib-testclean

### prog rules

prog-all: $(PROGRAMS)
.PHONY: prog-all

prog-install:
	@for i in -- $(PROGRAMS_TO_INSTALL); do \
		case $$i in --) continue;; esac; \
		$(MKDIR_P) $(bindir); \
		echo \
		$(INSTALL_PROGRAM) $$i $(bindir)/$$i; \
		$(INSTALL_PROGRAM) $$i $(bindir)/$$i; \
	done
.PHONY: prog-install

prog-clean: obj-clean
.PHONY: prog-clean

prog-veryclean:
	@case "$(PROGRAMS)" in \
	'')	;; \
	*)	echo \
		$(RM) $(PROGRAMS); \
		$(RM) $(PROGRAMS);; \
	esac
.PHONY: prog-veryclean

prog-distclean:
.PHONY: prog-distclean

prog-testclean:
	$(RM) *.log *.log.xml
	$(RM) test.log test.log.* test.out.*
	$(RM) .test-sum-*.sh
	$(RM) loop-*.log .loop-*.log
	$(RM) seek-file.text
.PHONY: prog-testclean

### doc (doxygen) rules

doc-distclean:
	-$(RM) .doxygen_*
.PHONY: doc-distclean

doc-here: doc-distclean
	-@case "$(MAN1_SRCS)" in \
	'')	;; \
	*)	for i in $(MAN1_SRCS); do echo $$i; done >>.doxygen_man1; \
		for i in $(MAN1_SRCS); do echo $$i; done >>.doxygen_latex;; \
	esac
	-@case "$(MAN3_SRCS)" in \
	'')	;; \
	*)	for i in $(MAN3_SRCS); do echo $$i; done >>.doxygen_man3; \
		for i in $(MAN3_SRCS); do echo $$i; done >>.doxygen_latex;; \
	esac
	-@case "$(MAN7_SRCS)" in \
	'')	;; \
	*)	for i in $(MAN7_SRCS); do echo $$i; done >>.doxygen_man7; \
		for i in $(MAN7_SRCS); do echo $$i; done >>.doxygen_latex;; \
	esac
.PHONY: doc-here

### common rules

obj-clean:
	@case "$(OBJS)" in \
	'')	;; \
	*)	echo \
		$(RM) $(OBJS); \
		$(RM) $(OBJS);; \
	esac

$(OBJS):

### hooks

post-all-hook:
pre-install-hook:
post-install-hook:
post-clean-hook:
post-veryclean-hook:
post-distclean-hook:
post-testclean-hook:
post-documents-hook:
.PHONY: post-all-hook pre-install-hook post-install-hook
.PHONY: post-clean-hook post-veryclean-hook post-distclean-hook
.PHONY: post-testclean-hook
.PHONY: post-documents-hook
.PHONY: debug cdebug
