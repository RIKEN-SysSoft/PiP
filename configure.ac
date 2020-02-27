#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([pip], [2.0.0], [pip@ml.riken.jp], [], [https://github.com/RIKEN-SysSoft/PiP])
AC_CONFIG_SRCDIR([RELEASE_NOTE])
AC_CONFIG_AUX_DIR([build])
AC_CONFIG_MACRO_DIR([build/m4])
AC_CONFIG_HEADERS([include/config.h])

# Specialized system macros
AC_CANONICAL_HOST

# Checks for command line options

AC_MSG_CHECKING([for glibc libdir])
AC_ARG_WITH([glibc-libdir],
  [AS_HELP_STRING([--with-glibc-libdir=DIR],
                  [glibc lib directory @<:@default=/lib64@:>@])])
case "$with_glibc_libdir" in
""|yes|no) with_glibc_libdir=/lib64;;
esac
dynamic_linker=`ls -d "${with_glibc_libdir}"/ld-@<:@0-9@:>@*.so | sed -n '$p'`
if test -d "${with_glibc_libdir}" -a -x "${dynamic_linker}"; then
  AC_MSG_RESULT([${with_glibc_libdir}])
  glibc_libdir=${with_glibc_libdir}
else
  AC_MSG_ERROR([can't find ld.so.])
fi
AC_SUBST(prefix)
AC_SUBST(exec_prefix)
AC_SUBST(includedir)
AC_SUBST(libdir)
AC_SUBST(glibc_libdir)
AC_SUBST(dynamic_linker)

AC_ARG_ENABLE([fcontext], [AC_HELP_STRING([--disable-fcontext],
  [Switch if using GLIBC's ucontext of Boost's fcontext (fpreg)])],[yes])
AS_IF([test "x$enable_fcontext" != "xno"], [
  AC_CHECK_PROG([AS], [as], [yes])
  AS_IF([test x"$AS" != x"yes"], [AC_MSG_ERROR([Can't find assmbler (as)])])
  AC_DEFINE(enable_fcontext)
])

boost_fctx_objs=""
if test "x$enable_fcontext" != "xno"; then
  boost_fctx_objs="fctx_jump.o fctx_make.o"
  case $host in
	x86_64-*-*-*)
		boost_fctx_arch="x86_64_sysv";;
	aarch64-*-*-*)
		boost_fctx_arch="arm64_aapcs";;
  esac
  if test "x$enable_fcontext" == "xfpreg"; then
     boost_fctx_fpreg_flag="-DBOOST_USE_TSX"
  fi
fi
AC_SUBST(boost_fctx_objs)
AC_SUBST(boost_fctx_arch)
AC_SUBST(boost_fctx_fpreg_flag)

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL

if test "$GCC" = yes; then
  CFLAGS="$CFLAGS -Wall"
fi


# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([
	build/config.mk
	release/version.conf
	lib/Makefile
	bin/pipcc
	bin/pip-mode
	bin/pip-check
	bin/pip-man
	util/piplnlibs
])
AC_OUTPUT
