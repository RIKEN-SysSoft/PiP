#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([pip], [3.0.0], [pip@ml.riken.jp], [], [https://github.com/RIKEN-SysSoft/PiP])
AC_CONFIG_SRCDIR([RELEASE_NOTE])
AC_CONFIG_AUX_DIR([build])
AC_CONFIG_MACRO_DIR([build/m4])
AC_CONFIG_HEADERS([include/config.h])

# Specialized system macros
AC_CANONICAL_HOST

# srcdir_top

cwd=`dirname $0`;
srcdir_top=`realpath $cwd`;
AC_SUBST(srcdir_top)

# Checks for command line options

AC_MSG_CHECKING([for glibc libdir])
AC_ARG_WITH([glibc-libdir],
  [AS_HELP_STRING([--with-glibc-libdir=DIR],
                  [glibc lib directory @<:@default=/usr/lib64@:>@])])
case "$with_glibc_libdir" in
""|yes|no) with_glibc_libdir="/usr/lib64";;
esac
dynamic_linker=`ls -d "${with_glibc_libdir}"/ld-@<:@0-9@:>@*.so | sed -n '$p'`
glibc_lib=`ls -d "${with_glibc_libdir}"/libc-@<:@0-9@:>@*.so | sed -n '$p'`
if test -d "${with_glibc_libdir}" -a -x "${dynamic_linker}" -a -f "${glibc_lib}"; then
  AC_MSG_RESULT([${with_glibc_libdir}])
  glibc_libdir="${with_glibc_libdir}"
  glibc_incdir="`realpath ${with_glibc_libdir}/../include`"
else
  AC_MSG_ERROR([can't find ld.so.])
fi
if ! test -d "${glibc_incdir}" ; then
  glibc_incdir="/usr/include"
fi
AC_MSG_RESULT([${glibc_incdir}])

AC_SUBST(prefix)
AC_SUBST(exec_prefix)
AC_SUBST(includedir)
AC_SUBST(libdir)
AC_SUBST(glibc_incdir)
AC_SUBST(glibc_libdir)
AC_SUBST(glibc_lib)
AC_SUBST(dynamic_linker)

AC_ARG_ENABLE([fcontext], [AC_HELP_STRING([--enable-fcontext],
  ["yes": use fcontext, "fp(reg(s))": fcontext with FP regs, "no": use (slow) ucontext])],
[
case "$enable_fcontext" in
  no)
     boost_fctx_objs="";
     boost_fctx_arch="";
     boost_fctx_cppflags="";;
  ""|yes|fp|fpreg|fpregs)
    AC_CHECK_PROG([AS], [as], [yes])
    AS_IF([test x"$AS" != x"yes"], [AC_MSG_ERROR([Can't find assmbler (as)])])
    boost_fctx_objs="fctx_jump.o fctx_make.o";
    case $host in
      x86_64-*-*-*)  boost_fctx_arch="x86_64_sysv";;
      aarch64-*-*-*) boost_fctx_arch="arm64_aapcs";;
      *) AC_MSG_ERROR([Unsupported host cpu]);;
    esac;
    case "$enable_fcontext" in
      yes) boost_fctx_cppflags="-DPIP_USE_FCONTEXT";;
      fp|fpreg|fpregs)
           boost_fctx_cppflags="-DPIP_USE_FCONTEXT -DBOOST_USE_TSX";;
    esac
    ;;
esac
],
[
  boost_fctx_objs="fctx_jump.o fctx_make.o"
  case $host in
  	x86_64-*-*-*)  boost_fctx_arch="x86_64_sysv";;
	aarch64-*-*-*) boost_fctx_arch="arm64_aapcs";;
	*) AC_MSG_ERROR([Unsupported host]);;
  esac
  boost_fctx_flag="-DPIP_USE_FCONTEXT"
])

AC_SUBST(boost_fctx_objs)
AC_SUBST(boost_fctx_arch)
AC_SUBST(boost_fctx_cppflags)

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
	lib/fcontext.mk
	bin/pipcc
	bin/pip-mode
	bin/pip-check
	bin/pip-man
	util/piplnlibs
	doc/latex-inuse/libpip-manpages.tex
])
AC_OUTPUT
