# default target is "all"

all: subdir-all header-all lib-all prog-all post-all-hook
.PHONY: all

install: all \
	pre-install-hook \
	subdir-install header-install lib-install prog-install examples-install \
	post-install-hook
.PHONY: install

clean: subdir-clean header-clean lib-clean prog-clean \
	misc-clean post-clean-hook
.PHONY: install

veryclean: clean \
	subdir-veryclean header-veryclean lib-veryclean prog-veryclean \
	post-veryclean-hook
.PHONY: veryclean

distclean: veryclean \
	subdir-distclean header-veryclean lib-distclean prog-distclean \
	doxygen-distclean distclean-here post-distclean-hook
.PHONY: distclean

doxygen: subdir-doxygen doxygen-here
.PHONY: doxygen

misc-clean:
	@echo \
	$(RM) *~ .nfs*; \
	$(RM) *~ .nfs*
.PHONY: misc-clean

distclean-here:
	@if [ -f $(srcdir)/Makefile.in ]; then \
		echo \
		$(RM) Makefile .doxygen_html .doxygen_man1 .doxygen_man3; \
		$(RM) Makefile .doxygen_html .doxygen_man1 .doxygen_man3; \
	fi
.PHONY: distclean-here

### subdir rules

subdir-all subdir-install subdir-clean subdir-veryclean subdir-distclean \
subdir-doxygen:
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
.PHONY: subdir-all subdir-install \
	subdir-clean subdir-veryclean subdir-distclean

### header rules

header-all: $(HEADERS)
.PHONY: header-all

header-install:
	@for i in -- $(HEADERS); do \
		case $$i in --) continue;; esac; \
		$(MKDIR_P) $(DESTDIR)$(includedir); \
		echo \
		$(INSTALL_DATA) $(srcdir)/$$i $(DESTDIR)$(includedir)/$$i; \
		$(INSTALL_DATA) $(srcdir)/$$i $(DESTDIR)$(includedir)/$$i; \
	done
	@for i in -- $(EXEC_HEADERS); do \
		case $$i in --) continue;; esac; \
		$(MKDIR_P) $(DESTDIR)$(exec_includedir); \
		echo \
		$(INSTALL_DATA) $$i $(DESTDIR)$(includedir)/$$i; \
		$(INSTALL_DATA) $$i $(DESTDIR)$(includedir)/$$i; \
	done
.PHONY: header-install

header-clean:
.PHONY: header-clean

header-veryclean:
.PHONY: header-veryclean

header-distclean:
	@case "$(EXEC_HEADERS)" in \
	'')	;; \
	*)	echo \
		$(RM) $(EXEC_HEADERS); \
		$(RM) $(EXEC_HEADERS);; \
	esac
.PHONY: header-distclean

### lib rules

lib-all: $(LIBRARIES)
.PHONY: lib-all

lib-install:
	@for i in -- $(LIBRARIES); do \
		case $$i in --) continue;; esac; \
		$(MKDIR_P) $(DESTDIR)$(libdir); \
		echo \
		$(INSTALL_DATA) $$i $(DESTDIR)$(libdir)/$$i; \
		$(INSTALL_DATA) $$i $(DESTDIR)$(libdir)/$$i; \
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

### prog rules

prog-all: $(PROGRAMS)
.PHONY: prog-all

prog-install:
	@for i in -- $(PROGRAMS_TO_INSTALL); do \
		case $$i in --) continue;; esac; \
		$(MKDIR_P) $(DESTDIR)$(bindir); \
		echo \
		$(INSTALL_PROGRAM) $$i $(DESTDIR)$(bindir)/$$i; \
		$(INSTALL_PROGRAM) $$i $(DESTDIR)$(bindir)/$$i; \
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

### doxygen rules

doxygen-here:
	-@$(RM) .doxygen_html;
	-@case "$(MAN1_SRCS)" in \
	'')	;; \
	*)	for i in $(MAN1_SRCS); do echo $$i; done > .doxygen_man1; \
		for i in $(MAN1_SRCS); do echo $$i; done >>.doxygen_html;; \
	esac
	-@case "$(MAN3_SRCS)" in \
	'')	;; \
	*)	for i in $(MAN3_SRCS); do echo $$i; done > .doxygen_man3; \
		for i in $(MAN3_SRCS); do echo $$i; done >>.doxygen_html;; \
	esac
.PHONY: doxygen-all

doxygen-distclean:
	-$(RM) .doxygen_html .doxygen_man1 .doxygen_man3
.PHONY: doxygen-distclean

### examples rules

examples-install:
	@for i in -- $(EXAMPLES); do \
		case $$i in --) continue;; esac; \
		$(MKDIR_P) $(DESTDIR)$(examplesdir); \
		echo \
		$(INSTALL_DATA) $(srcdir)/$$i $(DESTDIR)$(examplesdir)/$$i; \
		$(INSTALL_DATA) $(srcdir)/$$i $(DESTDIR)$(examplesdir)/$$i; \
	done
.PHONY: examples-install

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
.PHONY: post-all-hook pre-install-hook post-install-hook
.PHONY: post-clean-hook post-veryclean-hook post-distclean-hook
