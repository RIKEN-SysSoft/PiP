SHELL = /bin/sh
prefix = /home/ahori/PiP/x86_64/pip-1-1/install
exec_prefix = ${prefix}
PACKAGE_VERSION = 2.0.0
PACKAGE_TARNAME = proc-in-proc

host = x86_64-pc-linux-gnu
host_cpu = x86_64
host_os = linux-gnu
host_vendor = pc

srcdir_top = /home/ahori/PiP/x86_64/pip-1-1/github/PiP

default_sbindir = ${exec_prefix}/sbin
default_bindir = ${exec_prefix}/bin
default_libdir = ${exec_prefix}/lib
default_libexecdir = ${exec_prefix}/libexec
default_includedir = ${prefix}/include
default_exec_includedir = ${prefix}/include
default_datadir = ${datarootdir}
default_datarootdir = ${prefix}/share
default_mandir = ${datarootdir}/man
default_docdir = ${datarootdir}/doc/${PACKAGE_TARNAME}
default_htmldir = ${docdir}
default_pdfdir = ${docdir}
default_sysconfdir= ${prefix}/etc
default_localedir = ${datarootdir}/locale

glibc_incdir = /home/ahori/PiP/x86_64/install/include
glibc_libdir = /home/ahori/PiP/x86_64//install/lib
glibc_lib = /home/ahori/PiP/x86_64//install/lib/libc-2.17.so
dynamic_linker = /home/ahori/PiP/x86_64//install/lib/ld-2.17.so

DEFAULT_CC = gcc
DEFAULT_CFLAGS = -g -O2 -Wall
DEFAULT_LDFLAGS = 
DEFAULT_LIBS = 

DEFAULT_INSTALL = /usr/bin/install -c
DEFAULT_INSTALL_PROGRAM = ${INSTALL}
DEFAULT_INSTALL_SCRIPT = ${INSTALL}
DEFAULT_INSTALL_DATA = ${INSTALL} -m 644
