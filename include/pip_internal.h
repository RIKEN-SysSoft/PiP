/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2016-2017
*/

#ifndef _pip_internal_h_
#define _pip_internal_h_

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <asm/prctl.h>
#include <sys/prctl.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#include <pip_clone.h>
#include <pip_machdep.h>
#include <pip_debug.h>

//#define PIP_SWITCH_TLS

/** ULP supported version (NEW) */
#define PIP_BASE_VERSION	(0x3000U)
/** ULP supported version
#define PIP_BASE_VERSION	(0x2000U)
**/
/** The very first stable release version
#define PIP_BASE_VERSION	(0x1000U)
**/

#define PIP_VERSION		PIP_BASE_VERSION

#define PIP_ROOT_ENV		"PIP_ROOT_ADDR"
#define PIP_TASK_ENV		"PIP_TASK_PIPID"

#define PIP_MAGIC_WORD		"PrcInPrc"
#define PIP_MAGIC_LEN		(8)

#define PIP_PIPID_NONE		(-999)

#define PIP_LD_SOLIBS		{ NULL }

#define PIP_STACK_SIZE	(8*1024*1024) /* 8 MiB */
#define PIP_MIN_STACK_SIZE	(1*1024*1024) /* 1 MiB */
#define PIP_STACK_ALIGN	(256)

#define PIP_MASK32		(0xFFFFFFFF)

typedef	int(*main_func_t)(int,char**,char**);
typedef	int(*start_func_t)(void*);
typedef int(*mallopt_t)(int,int);
typedef void(*free_t)(void*);
typedef void(*pthread_init_t)(int,char**,char**);
typedef	void(*ctype_init_t)(void);
typedef void(*glibc_init_t)(int,char**,char**);
typedef void(*add_stack_user_t)(void);
typedef	void(*fflush_t)(FILE*);

typedef struct {
  /* functions */
  main_func_t		main;	      /* main function address */
  start_func_t		start;	      /* strat function instead of main */
  pthread_init_t	pthread_init; /* __pthread_init_minimum() */
  ctype_init_t		ctype_init;   /* to call __ctype_init() */
  glibc_init_t		glibc_init;   /* only in patched Glibc */
  fflush_t		libc_fflush;  /* to call fflush() at the end */
  mallopt_t		mallopt;      /* to call mallopt() */
  free_t		free;	      /* to override free() - EXPERIMENTAL*/
  add_stack_user_t	add_stack;    /* for GDB workarounnd */
  /* variables */
  char			***libc_argvp; /* to set __libc_argv */
  int			*libc_argcp;   /* to set __libc_argc */
  char			**progname;
  char			**progname_full;
  char			***environ;    /* pointer to the environ variable */
} pip_symbols_t;

typedef struct {
  int			pipid;
  int			coreno;
  char			*prog;
  char			**argv;
  char			*funcname;
  void			*start_arg;
  char			**envv;
} pip_spawn_args_t;

#define PIP_TYPE_NONE	(0x0)
#define PIP_TYPE_ROOT	(0x1)
#define PIP_TYPE_TASK	(0x2)
#define PIP_TYPE_ULP	(0x3)

struct pip_gdbif_task;

typedef struct pip_task {
  pip_ulp_t		queue;	     /* ULP list elm and sisters */
  pip_ulp_t		schedq;	     /* ULP scheduling queue */
  pip_spinlock_t	lock_schedq; /* lock for the scheduling queue */
  struct pip_task	*task_sched; /* scheduling task */

  int			pipid;	/* PiP ID */
  int			type;	/* PIP_TYPE_TASK or PIP_TYPE_ULP */

  void			*loaded; /* loaded DSO handle */
  pip_symbols_t		symbols;
  pip_spawn_args_t	args;	 /* arguments for a PiP task */
  void *volatile	export;
  pip_ctx_t		*ctx_exit;
  int			flag_exit;
  int			extval;

  struct pip_gdbif_task	*gdbif_task;

  /* PiP task (type==PIP_TYPE_TASK) */
  pip_spinlock_t	lock_malloc; /* lock for pip_malloc and pip_free */
  pid_t			pid;	/* PID in process mode */
  pthread_t		thread;	/* thread */
  pip_spawnhook_t	hook_before;
  pip_spawnhook_t	hook_after;
  void			*hook_arg;

  /* PiP ULP (type==PIP_TYPE_ULP) */
  void			*ulp_stack;
  pip_ctx_t		*ctx_suspend;
  pthread_mutex_t	mutex_wait;

} pip_task_t;

typedef struct {
  char			magic[PIP_MAGIC_LEN];
  unsigned int		version;
  size_t		root_size;
  size_t		size;
  pip_spinlock_t	lock_ldlinux; /* lock for dl*() functions */
  size_t		page_size;
  unsigned int		opts;
  unsigned int		actual_mode;
  int			ntasks;
  int			ntasks_curr;
  int			ntasks_accum;
  int			pipid_curr;
  pip_clone_t		*cloneinfo;   /* only valid with process:preload */
  pip_task_t		*task_root; /* points to tasks[ntasks] */
  pip_spinlock_t	lock_tasks; /* lock for finding a new task id */
  /* ULP */
  pip_spinlock_t	lock_stack_flist; /* ULP: lock for stack free list */
  void			*stack_flist;	  /* ULP: stack free list */
  size_t		stack_size;

  pip_task_t		tasks[];
} pip_root_t;

extern pip_root_t	*pip_root;
extern pip_task_t	*pip_task;

#define PIP_TASK(ULP)	((pip_task_t*)(ULP))
#define PIP_ULP(TASK)	((pip_ulp_t*)(TASK))
#define PIP_ULP_CTX(U)	(PIP_TASK(U)->ctx_suspend)

#define pip_likely(x)		__builtin_expect((x),1)
#define pip_unlikely(x)		__builtin_expect((x),0)

#define PIP_MIDLEN	(64)

#define ERRJ		{ DBG;                goto error; }
#define ERRJ_ERRNO	{ DBG; err=errno;     goto error; }
#define ERRJ_ERR(ENO)	{ DBG; err=(ENO);     goto error; }
#define ERRJ_CHK(FUNC)	{ if( (FUNC) ) goto error; }

#ifdef EVAL
#define ES(V,F)		\
  do { double __st=pip_gettime(); if(F) (V) += pip_gettime() - __st; } while(0)
double time_dlmopen   = 0.0;
double time_load_dso  = 0.0;
double time_load_prog = 0.0;
#define REPORT(V)	 printf( "%s: %g\n", #V, V );
#else
#define ES(V,F)		if(F)
#define REPORT(V)
#endif

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#endif /* _pip_internal_h_ */
