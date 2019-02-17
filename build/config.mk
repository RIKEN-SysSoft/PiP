SHELL = /bin/sh
prefix = /home/ahori/work/PIP/BLT/install
exec_prefix = ${prefix}
PACKAGE_TARNAME = pip

host = x86_64-unknown-linux-gnu
host_cpu = x86_64
host_os = linux-gnu
host_vendor = unknown

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
default_sysconfdir= ${prefix}/etc
default_localedir = ${datarootdir}/locale

glibc_libdir = /home/ahori/work/PIP/BLT/GLIBC/install/lib
dynamic_linker = /home/ahori/work/PIP/BLT/GLIBC/install/lib/ld-2.17.so

DEFAULT_CC = gcc
DEFAULT_CFLAGS = -g -O2 -Wall
DEFAULT_LDFLAGS = 
DEFAULT_LIBS = 

DEFAULT_INSTALL = /usr/bin/install -c
DEFAULT_INSTALL_PROGRAM = ${INSTALL}
DEFAULT_INSTALL_SCRIPT = ${INSTALL}
DEFAULT_INSTALL_DATA = ${INSTALL} -m 644
