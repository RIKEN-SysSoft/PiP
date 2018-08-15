/*
  * $RIKEN_copyright: 2018 Riken Center for Computational Sceience,
  * 	  System Software Devlopment Team. All rights researved$
  * $PIP_VERSION: Version 1.0$
  * $PIP_license: <Simplified BSD License>
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted provided that the following conditions are
  * met:
  *
  * 1. Redistributions of source code must retain the above copyright
  *    notice, this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright
  *    notice, this list of conditions and the following disclaimer in the
  *    documentation and/or other materials provided with the distribution.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  * The views and conclusions contained in the software and documentation
  * are those of the authors and should not be interpreted as representing
  * official policies, either expressed or implied, of the PiP project.$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2016-2018
*/

#ifndef _pip_internal_h_
#define _pip_internal_h_

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

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

#define PIP_PIPID_NONE		(-9999)

#define PIP_LD_SOLIBS		{ NULL }

#define PIP_STACK_SIZE		(8*1024*1024) /* 8 MiB */
#define PIP_STACK_SIZE_MIN	(1*1024*1024) /* 1 MiB */
#define PIP_STACK_ALIGN		(256)

#define PIP_MASK32		(0xFFFFFFFF)

#define PIP_EXITED		(1)
#define PIP_EXIT_FINALIZE	(2)

struct pip_task;

typedef	int(*main_func_t)(int,char**,char**);
typedef	int(*start_func_t)(void*);
typedef int(*mallopt_t)(int,int);
typedef void(*free_t)(void*);
typedef void(*pthread_init_t)(int,char**,char**);
typedef	void(*ctype_init_t)(void);
typedef void(*glibc_init_t)(int,char**,char**);
typedef	void(*fflush_t)(FILE*);
typedef int(*named_export_fin_t)(struct pip_task*);

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
  named_export_fin_t	named_export_fin; /* for free()ing hash entries */
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

#define PIP_TYPE_NONE		(0x000)
#define PIP_TYPE_ROOT		(0x100)
#define PIP_TYPE_TASK		(0x080)
#define PIP_TYPE_ULP		(0x040)

struct pip_gdbif_task;

typedef struct pip_blocking {
  union {			/* mutex or semaphore */
    pthread_mutex_t	mutex;
    sem_t		semaphore;
  };
} pip_blocking_t;

typedef struct pip_task {
  /* Frequently accessing part (packed into a cache block */
  union {
    struct {
      pip_ulp_t		queue;	     /* !!! DO NOT MOVE THIS AROUND !!! */
      pip_ulp_t		schedq;	     /* ULP scheduling queue */
      pip_spinlock_t	lock_schedq; /* lock of scheduling queue (no need ?) */
      struct pip_task	*task_sched; /* scheduling task */
      struct pip_task	*task_resume; /* scheduling task when resumed */
      int32_t		pipid;	     /* PiP ID */
      int32_t		type;	/* PIP_TYPE_TASK, PIP_TYPE_ULP, or ... */
    };
    char		__gap0__[PIP_CACHEBLK_SZ];
  };
  /* PiP ULP (type&PIP_TYPE_ULP) */
  void 			*ulp_stack; /* OBS */
  union {
    struct {
      pip_ctx_t		*ctx_suspend; /* context to resume */
      pip_blocking_t	sleep;
      pip_blocking_t	wait;
    };
    char		__gap1__[PIP_CACHEBLK_SZ];
  };
  /* PiP task (type&PIP_TYPE_TASK) */
  union {
    struct {
      pid_t		pid;	/* PID in process mode */
      pthread_t		thread;	/* thread */
      pip_spawnhook_t	hook_before; /* before spawning hook */
      pip_spawnhook_t	hook_after;  /* after spawning hook */
      void		*hook_arg;   /* hook arg */
      pip_spinlock_t	lock_malloc; /* lock for pip_malloc and pip_free */
    };
    char		__gap2__[PIP_CACHEBLK_SZ];
  };
  /* common part */
  void			*loaded; /* loaded DSO handle */
  pip_symbols_t		symbols; /* symbols */
  pip_spawn_args_t	args;	 /* arguments for a PiP task */
  void *volatile	export;
  void			*named_exptab;
  pip_ctx_t		*context;
  volatile int		flag_exit; /* if this task is terminated or not */
  int			extval;	   /* exit value */
  /* GDB interface */
  struct pip_gdbif_task	*gdbif_task; /* GDB interface */

} pip_task_t;

typedef struct {
  /* sanity check info */
  char			magic[PIP_MAGIC_LEN];
  unsigned int		version;
  size_t		root_size;
  size_t		size;
  /* actual root info */
  pip_spinlock_t	lock_ldlinux; /* lock for dl*() functions */
  size_t		page_size;
  unsigned int		opts;
  int			ntasks;
  int			ntasks_curr;
  int			ntasks_accum;
  int			pipid_curr;
  pip_clone_t		*cloneinfo;   /* only valid with process:preload */
  pip_task_t		*task_root; /* points to tasks[ntasks] */
  pip_spinlock_t	lock_tasks; /* lock for finding a new task id */
  /* ULP related info */
  pip_spinlock_t	lock_stack_flist; /* ULP: lock for stack free list */
  void			*stack_flist;	  /* ULP: stack free list */
  size_t		stack_size;
  /* for chaining SIGCHLD */
  struct sigaction	sigact_chain;
  /* PiP tasks array */
  pip_task_t		tasks[];

} pip_root_t;

extern pip_root_t	*pip_root;
extern pip_task_t	*pip_task;

#define pip_likely(x)	__builtin_expect((x),1)
#define pip_unlikely(x)	__builtin_expect((x),0)

#define PIP_TASK(ULP)	((pip_task_t*)(ULP))
#define PIP_ULP(TASK)	((pip_ulp_t*)(TASK))
#define PIP_ULP_CTX(U)	(PIP_TASK(U)->ctx_suspend)

#define PIP_MIDLEN	(64)

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#endif /* _pip_internal_h_ */
