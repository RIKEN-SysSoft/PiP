#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([Process-in-Processs], [3.0.0], [pip@ml.riken.jp], [proc-in-proc],
			       [https://github.com/RIKEN-SysSoft/PiP])
AC_CONFIG_SRCDIR([RELEASE_NOTE])
AC_CONFIG_AUX_DIR([build])
AC_CONFIG_HEADERS([include/pip_config.h])

# Specialized system macros
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

case "$target_os" in
     linux*) true;;
     *) AC_MSG_ERROR([Unsupported OS]);;
esac

case "$target" in
      x86_64-*)  true;;
      aarch64-*) true;;
      *) AC_MSG_ERROR([Unsupported target CPU architecture]);;
esac;

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
""|yes|no) have_pipglibc=false; with_glibc_libdir="/usr/lib64";;
*) have_pipglibc=true;;
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
AC_SUBST(have_pipglibc)
AC_SUBST(glibc_incdir)
AC_SUBST(glibc_libdir)
AC_SUBST(glibc_lib)
AC_SUBST(dynamic_linker)

getconf=`/bin/which getconf`
if test -x $getconf; then
   cache_line_size=`$getconf LEVEL1_DCACHE_LINESIZE`
   AC_DEFINE_UNQUOTED([CACHE_LINE_SIZE], [${cache_line_size}], ["Cache Line Size"])
fi

AC_ARG_ENABLE([fcontext], [AC_HELP_STRING([--enable-fcontext],
  ["yes": use fcontext (default), "fp(reg(s))": fcontext with FP regs, "no": use (slow) ucontext])],
[
case "$enable_fcontext" in
  no)
     boost_fctx_objs="";
     boost_fctx_arch="";
     boost_fctx_cppflags="";;
  *)
    boost_fctx_objs="fctx_jump.o fctx_make.o";
    case "$target" in
      x86_64-*)  boost_fctx_arch="x86_64_sysv";;
      aarch64-*) boost_fctx_arch="arm64_aapcs";;
    esac;
    case "$enable_fcontext" in
      fp|fpreg|fpregs) boost_fctx_cppflags="-DPIP_USE_FCONTEXT -DBOOST_USE_TSX";;
      *)               boost_fctx_cppflags="-DPIP_USE_FCONTEXT";;
    esac
    ;;
esac
],
[
  boost_fctx_objs="fctx_jump.o fctx_make.o"
  case "$target" in
  	x86_64-*)  boost_fctx_arch="x86_64_sysv";;
	aarch64-*) boost_fctx_arch="arm64_aapcs";;
  esac
  boost_fctx_cppflags="-DPIP_USE_FCONTEXT"
])

AC_SUBST(boost_fctx_objs)
AC_SUBST(boost_fctx_arch)
AC_SUBST(boost_fctx_cppflags)

prefix_slides=${prefix}/share/slides
AC_SUBST(prefix_slides)

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
	lib/fctx_defs.mk
	lib/fcontext.mk
	bin/pipcc
	bin/pip-mode
	bin/pip-check
	bin/pip-man
	doc/latex-inuse/libpip-manpages.tex
	doc/Slides-html.md
])
AC_OUTPUT
