#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([pip], [SNAPSHOT])
AC_CONFIG_SRCDIR([include/pip.h])
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
AC_SUBST(glibc_libdir)
AC_SUBST(dynamic_linker)


# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

if test "$GCC" = yes; then
  CFLAGS="$CFLAGS -Wall"
fi

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([build/config.mk])
AC_OUTPUT
