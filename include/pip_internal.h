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
#include <stddef.h>
#include <sched.h>
#include <unistd.h>
#include <string.h>

#ifdef DEBUG
#ifndef MCHECK
//#define MCHECK
#endif
#endif

#include <pip.h>
#include <pip_blt.h>
#include <pip_clone.h>
#include <pip_machdep.h>
#include <pip_debug.h>
#include <pip_util.h>

//#define PIP_USE_STATIC_CTX

/* the EVAL define symbol is to measure the time for calling dlmopen() */
//#define EVAL

/** BLT supported version **/
#define PIP_BASE_VERSION	(0x4000U)
/** ULP supported version
#define PIP_BASE_VERSION	(0x3000U)
**/
/** ULP supported version
#define PIP_BASE_VERSION	(0x2000U)
**/
/** The very first stable release version
#define PIP_BASE_VERSION	(0x1000U)
**/

#define PIP_API_VERSION		PIP_BASE_VERSION

#define PIP_ROOT_ENV		"PIP_ROOT_ADDR"
#define PIP_TASK_ENV		"PIP_TASK_PIPID"

#define PIP_MAGIC_WORD		"PrcInPrc"
#define PIP_MAGIC_LEN		(8)

#define PIP_LD_SOLIBS		{ NULL }

#define PIP_STACK_SIZE		(8*1024*1024) /* 8 MiB */
#define PIP_STACK_SIZE_MIN	(1*1024*1024) /* 1 MiB */
#define PIP_STACK_ALIGN		(256)

#define PIP_EXITED		(1)
#define PIP_EXIT_FINALIZE	(2)
#define PIP_ABORT		(9)

struct pip_task_internal;

typedef struct pip_cacheblk {
  char		cacheblk[PIP_CACHEBLK_SZ];
} pip_cacheblk_t;

typedef volatile uint8_t	pip_stack_protect_t;

struct pip_task_annex;

typedef struct pip_task_internal {
  union {
    struct {
      /* Most frequently accessed (time critial) part */
      pip_task_t		queue; /* !!! DO NOT MOVE THIS AROUND !!! */
      pip_task_t		schedq;	/* scheduling queue */
      struct pip_task_internal	*task_sched; /* scheduling task */
#ifdef PIP_TYPE_TLS
      pip_tls_t			tls;
#endif
      pip_ctx_t			*ctx_suspend; /* context to resume */
      pip_stack_protect_t	*flag_stackpp; /* to unprotect stack (ctxsw) */
      /* end of one cache block (64 bytes) */
      /* less frequently accessed part follows */
      pip_atomic_t		ntakecare; /* tasks must be taken care */
      struct {
	int16_t			pipid;	    /* PiP ID */
	uint16_t		schedq_len; /* length of schedq */
	volatile uint16_t	oodq_len; /* indictaes ood task is added */
	pip_stack_protect_t	flag_stackp; /* stack protection (ctxsw) */
	uint8_t			type; /* PIP_TYPE_ROOT, PIP_TYPE_TASK, ... */
	char			state;	       /* BLT state */
	volatile uint8_t	flag_exit; /* if this task is terminated */
	volatile uint8_t	flag_wakeup; /* flag to wakeup */
	volatile uint8_t	flag_semwait; /* if sem_wait() is called */
      };
      struct pip_task_annex	*annex;

#ifdef PIP_USE_STATIC_CTX
      pip_ctx_t			*ctx_static; /* staically allocated context */
#endif
    };
    pip_cacheblk_t		__align__[2];
  };
} pip_task_internal_t;

typedef	int(*main_func_t)(int,char**,char**);
typedef	int(*start_func_t)(void*);
typedef int(*mallopt_t)(int,int);
typedef void(*free_t)(void*);
typedef void(*pthread_init_t)(int,char**,char**);
typedef	void(*ctype_init_t)(void);
typedef void(*glibc_init_t)(int,char**,char**);
typedef	void(*fflush_t)(FILE*);
typedef int(*named_export_fin_t)(struct pip_task_internal*);
typedef int (*pip_clone_mostly_pthread_t)
( pthread_t *newthread, int, int, size_t, void *(*)(void *), void*, pid_t* );

typedef struct {
  /* functions */
  main_func_t		main;	      /* main function address */
  start_func_t		start;	      /* strat function instead of main */
  ctype_init_t		ctype_init;   /* to call __ctype_init() */
  fflush_t		libc_fflush;  /* to call fflush() at the end */
  mallopt_t		mallopt;      /* to call mallopt() */
  named_export_fin_t	named_export_fin; /* for free()ing hash entries */
  /* Unused functions */
  free_t		free;	      /* to override free() - EXPERIMENTAL*/
  glibc_init_t		glibc_init;   /* only in patched Glibc */
  pthread_init_t	pthread_init; /* __pthread_init_minimum() */
  /* variables */
  char			***libc_argvp; /* to set __libc_argv */
  int			*libc_argcp;   /* to set __libc_argc */
  char			**prog;
  char			**prog_full;
  char			***environ;    /* pointer to the environ variable */
} pip_symbols_t;

typedef struct {
  int			pipid;
  int			coreno;
  char			*prog;
  char			*prog_full;
  char			**argv;
  char			*funcname;
  void			*start_arg;
  char			**envv;
  /* list of close-on-exec FDs */
  int			*fd_list;
} pip_spawn_args_t;

struct pip_gdbif_task;

typedef sem_t			pip_blocking_t;

typedef struct pip_task_annex {
  /* less and less frequently accessed part follows */
  pip_blocking_t		sleep;
  void				*sleep_stack;

  pip_task_t			oodq;	   /* out-of-damain queue */
  pip_spinlock_t		lock_oodq; /* lock for OOD queue */

  void *volatile		export;
  void				*named_exptab;

  void				*aux; /* pointer to user data (if provided) */

  uint32_t			opts;
  int32_t			extval;	   /* exit value */

  pid_t				pid; /* PID in process mode at beginning */
  pthread_t			thread;	/* thread */
  void				*loaded; /* loaded DSO handle */
  /* spawn info */
  pip_spawn_args_t		args;	 /* arguments for a PiP task */
  pip_symbols_t			symbols; /* symbols */
  pip_spawnhook_t		hook_before; /* before spawning hook */
  pip_spawnhook_t		hook_after;  /* after spawning hook */
  void				*hook_arg;   /* hook arg */

  /* GDB interface */
  struct 			pip_gdbif_task	*gdbif_task; /* GDB if */
} pip_task_annex_t;


#define PIP_TASKI(TASK)		((pip_task_internal_t*)(TASK))
#define PIP_TASKQ(TASK)		((pip_task_t*)(TASK))

#define PIP_TYPE_NONE		(0x00)
#define PIP_TYPE_ROOT		(0x01)
#define PIP_TYPE_TASK		(0x02)

#define PIP_TASK_RUNNING	('R')
#define PIP_TASK_SUSPENDED	('S')

#define PIP_RUN(T)		( PIP_TASKI(T)->state = PIP_TASK_RUNNING   )
#define PIP_SUSPEND(T)		( PIP_TASKI(T)->state = PIP_TASK_SUSPENDED )

#define PIP_IS_RUNNING(T)	( PIP_TASKI(T)->state == PIP_TASK_RUNNING   )
#define PIP_IS_SUSPENDED(T)	( PIP_TASKI(T)->state == PIP_TASK_SUSPENDED )

#define PIP_ISA_ROOT(T)		( PIP_TASKI(T)->type & PIP_TYPE_ROOT )
#define PIP_ISA_TASK(T)		\
  ( PIP_TASKI(T)->type & ( PIP_TYPE_ROOT | PIP_TYPE_TASK ) )

#define PIP_BUSYWAIT_COUNT	(10*1000)

#define PIP_MASK32		(0xFFFFFFFF)

typedef struct {
  /* sanity check info */
  char			magic[PIP_MAGIC_LEN];
  unsigned int		version;
  size_t		size_whole;
  size_t		size_root;
  size_t		size_task;
  size_t		size_annex;
  /* actual root info */
  pip_spinlock_t	lock_ldlinux; /* lock for dl*() functions */
  pip_atomic_t		ntasks_blocking;
  int			ntasks_count;
  int			ntasks_curr;
  int			ntasks_accum;
  int			ntasks;
  int			pipid_curr;
  uint32_t		opts;
  size_t		page_size;
  pip_clone_t		*cloneinfo;   /* only valid with process:preload */
  pip_task_internal_t	*task_root; /* points to tasks[ntasks] */
  pip_spinlock_t	lock_tasks; /* lock for finding a new task id */
  /* BLT related info */
  size_t		stack_size_blt; /* size for the stack for BLTs */
  size_t		stack_size_sleep; /* size for the stack for sleeping */
  /* for chaining SIGCHLD */
  struct sigaction	sigact_chain;
  /* PiP tasks array */
  pip_task_annex_t	*annex;
  pip_task_internal_t	tasks[];

} pip_root_t;

extern pip_clone_mostly_pthread_t 	pip_clone_mostly_pthread_ptr;

#define PIP_MIDLEN		(64)

#define pip_likely(x)		__builtin_expect((x),1)
#define pip_unlikely(x)		__builtin_expect((x),0)

#define IF_LIKELY(C)		if( pip_likely( C ) )
#define IF_UNLIKELY(C)		if( pip_unlikely( C ) )

extern pip_root_t		*pip_root_;
extern pip_task_internal_t	*pip_task_;

inline static int pip_is_alive_( pip_task_internal_t *taski ) {
  return taski->pipid != PIP_TYPE_NONE && !taski->flag_exit;
}

inline static int pip_get_pipid_( void ) {
  if( pip_task_ == NULL ) return PIP_PIPID_NULL;
  return pip_task_->pipid;
}

inline static int pip_check_pipid_( int *pipidp ) {
  int pipid = *pipidp;

  if( pip_root_ == NULL          ) RETURN( EPERM  );
  if( pipid >= pip_root_->ntasks ) RETURN( EINVAL );
  if( pipid != PIP_PIPID_MYSELF &&
      pipid <  PIP_PIPID_ROOT    ) RETURN( EINVAL );

  switch( pipid ) {
  case PIP_PIPID_ROOT:
    break;
  case PIP_PIPID_ANY:
    RETURN( EINVAL );
  case PIP_PIPID_MYSELF:
    if( pip_isa_root() ) {
      *pipidp = PIP_PIPID_ROOT;
    } else if( pip_isa_task() ) {
      *pipidp = pip_task_->pipid;
    } else {
      RETURN( ENXIO );
    }
    break;
  }
  return 0;
}

inline static pip_task_internal_t *pip_get_task_( int pipid ) {
  pip_task_internal_t 	*taski = NULL;

  switch( pipid ) {
  case PIP_PIPID_ROOT:
    taski = pip_root_->task_root;
    break;
  default:
    taski = &pip_root_->tasks[pipid];
    break;
  }
  return taski;
}

inline static pip_clone_t *pip_get_cloneinfo_( void ) {
  return pip_root_->cloneinfo;
}

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#endif /* _pip_internal_h_ */
