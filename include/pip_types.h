/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2016, 2017
*/

#ifndef _pip_types_h_
#define _pip_types_h_

#include <ucontext.h>

typedef ucontext_t*	pip_ulp_ctx_t;

typedef int(*pip_spawnhook_t)(void*);
typedef	void(*pip_ulp_exithook_t)(void*);

#endif /* _pip_types_h_ */
