/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2016
*/

#ifndef _pip_machdep_h_
#define _pip_machdep_h_

#if defined(__x86_64__)
#include <pip_machdep_x86_64.h>
#elif defined(__aarch64__)
#include <pip_machdep_aarch64.h>
#else
#error "Unsupported Machine Type"
#endif

#endif
