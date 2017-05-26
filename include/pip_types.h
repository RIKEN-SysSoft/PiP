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

typedef ucontext_t	pip_ulp_ctx_t;

struct pip_task;

typedef struct {
  pip_ulp_ctx_t		*ctx;
  struct pip_task	*ulp;
} pip_ulp_t;

typedef pip_ulp_t* (*pip_ulp_create_t)   ( void* );
typedef       void (*pip_ulp_yield_t)    ( void* );
typedef       void (*pip_ulp_destroy_t)  ( void* );

typedef struct pip_ulp_sched {
  pip_ulp_create_t	create;
  pip_ulp_yield_t	yield;
  pip_ulp_destroy_t	destroy;
} pip_ulp_sched_t;

typedef int(*pip_spawnhook_t)(void*);
typedef	void(*pip_ulp_exithook_t)(void*);

#endif /* _pip_types_h_ */
