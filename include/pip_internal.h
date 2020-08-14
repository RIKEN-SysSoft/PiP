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
  * Written by Atsushi HORI <ahori@riken.jp>
*/

#ifndef _pip_internal_h_
#define _pip_internal_h_

#ifdef DOXYGEN_INPROGRESS
#ifndef INLINE
#define INLINE
#endif
#ifndef NORETURN
#define NORETURN
#endif
#else
#ifndef INLINE
#define INLINE			inline static
#endif
#ifndef NORETURN
#define NORETURN		__attribute__((noreturn))
#endif
#endif

#ifndef DOXYGEN_INPROGRESS

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>
#include <fcntl.h>
#include <link.h>
#include <dlfcn.h>
#include <ucontext.h>

#include <config.h>

#ifndef CACHE_LINE_SIZE
#define CACHE_LINE_SIZE		(64)
#endif

#include <pip_blt.h>
#include <pip_clone.h>
#include <pip_context.h>
#include <pip_signal.h>
#include <pip_util.h>
#include <pip_gdbif.h>
#include <pip_debug.h>

/* the EVAL define symbol is to measure the time for calling dlmopen() */
//#define EVAL

/** BLT/ULP supported version (RELEASED as PiP v3) **/
#define PIP_BASE_VERSION	(0x4000U)
/** BLT/ULP UNsupported version (RELEASED as PiP v2)
#define PIP_BASE_VERSION	(0x3000U)
**/
/** ULP supported version -- abandoned to release
#define PIP_BASE_VERSION	(0x2000U)
**/
/** The very first stable release version -- No, this was not stable !!
#define PIP_BASE_VERSION	(0x1000U)
**/

#define PIP_API_VERSION		PIP_BASE_VERSION

#define PIP_ROOT_ENV		"PIP_ROOT_ADDR"
#define PIP_TASK_ENV		"PIP_TASK_PIPID"

#define PIP_MAGIC_WORD		"PrcInPrc"
#define PIP_MAGIC_WLEN		(8)

#define PIP_STACK_SIZE		(8*1024*1024) /* 8 MiB */
#define PIP_STACK_SIZE_MIN	(1*1024*1024) /* 1 MiB */
#define PIP_STACK_ALIGN		(256)

#define PIP_EXITED		(1)
#define PIP_EXIT_WAITED		(2)

#define PIP_CACHE_ALIGN(X)					\
  ( ( (X) + CACHE_LINE_SIZE - 1 ) & ~( CACHE_LINE_SIZE -1 ) )

struct pip_root;
struct pip_task_internal;
struct pip_gdbif_task;

typedef	int  (*main_func_t)(int,char**,char**);
typedef	int  (*start_func_t)(void*);
typedef void (*libc_init_first_t)(int,char**,char**);
typedef int  (*res_init_t)(void);
typedef	void (*ctype_init_t)(void);
typedef int  (*mallopt_t)(int,int);
typedef	void (*fflush_t)(FILE*);
typedef void (*exit_t)(int);
typedef void (*pthread_exit_t)(void*);
typedef int  (*named_export_fin_t)(struct pip_task_internal*);
typedef int  (*pip_clone_mostly_pthread_t)
( pthread_t *newthread, int, int, size_t, void *(*)(void *), void*, pid_t* );
typedef int  (*pip_init_t)(struct pip_root*,struct pip_task_internal*);
typedef int  (*pip_clone_syscall_t)(int(*)(void*),void*,int,void*,pid_t*,void*,pid_t*);
typedef struct pip_task_internal* (*pip_current_utask_t)( void );
struct pip_gdbif_root;

typedef struct pip_symbols {
  main_func_t		main;	      /* main function address */
  start_func_t		start;	      /* strat function instead of main */
  /* PiP init functions */
  pip_init_t		pip_init;     /* implicit initialization */
  named_export_fin_t	named_export_fin; /* for free()ing hash entries */
  /* glibc variables */
  char			***libc_argvp; /* to set __libc_argv     */
  int			*libc_argcp;   /* to set __libc_argc     */
  char			**prog;	       /* to set __progname      */
  char			**prog_full;   /* to set __progname_full */
  char			***environ;    /* pointer to the environ variable */
  /* GLIBC init funcs */
  libc_init_first_t	libc_init;    /* initialize GLIBC */
  res_init_t		res_init;     /* (unused) GLIBC res_init() */
  ctype_init_t		ctype_init;   /* to call GLIBC __ctype_init() */
  /* GLIBC functions */
  mallopt_t		mallopt;	/* to call GLIBC mallopt() */
  long long		*malloc_hook;	/* Kaiming patch */
  long long		*realloc_hook;	/* (unused) */
  long long 		*memalign_hook;	/* (unused) */
  long long		*free_hook;	/* (unused) */
  fflush_t		libc_fflush; /* to call GLIBC fflush() at the end */
  exit_t		exit;	     /* call exit() from fork()ed process */
  pthread_exit_t	pthread_exit;	   /* (see above exit) */
  void			*__reserved__[15]; /* reserved for future use */
} pip_symbols_t;

typedef struct pip_char_vec {
  char			**vec;
  char			*strs;
} pip_char_vec_t;

typedef struct pip_spawn_args {
  int			pipid;
  int			coreno;
  int			argc;
  char			*prog;
  char			*prog_full;
  char			*funcname;
  void			*start_arg;
  pip_char_vec_t	argvec;
  pip_char_vec_t	envvec;
  int			*fd_list; /* list of close-on-exec FDs */
  pip_task_queue_t	*queue;
} pip_spawn_args_t;

typedef sem_t			pip_sem_t;

typedef struct pip_task_misc {
  volatile pthread_t		thread;	/* thread */
  void				*loaded; /* loaded DSO handle */
  /* for debug */
  struct pip_task_internal	*task_current;
  /* spawn info */
  pip_spawn_args_t		args;	 /* arguments for a PiP task */
  pip_symbols_t			symbols; /* symbols */
  pip_spawnhook_t		hook_before; /* before spawning hook */
  pip_spawnhook_t		hook_after;  /* after spawning hook */
  void				*hook_arg;   /* hook arg */
  /* GDB interface */
  //AH//void				*load_address;
  struct pip_gdbif_task		*gdbif_task; /* GDB if */
  /* stack for pip-gdb automatic attach */
  void				*sigalt_stack;
} pip_task_misc_t;

typedef struct pip_task_annex {
  /* less frequently accessed part follows */
  pip_sem_t			sleep;
  pip_ctx_p			ctx_trampoline; /* trampoline context */
  void				*stack_trampoline;

  volatile int32_t		status;	   /* exit value */
  uint32_t			opts;
  uint32_t			opts_sync;
  volatile int8_t		flag_sigchld; /* termination in thread mode */
  uint8_t			flag_exit; /* if this task is terminated */
  uint8_t			flag_wakeup; /* flag to wakeup */

  struct pip_task_internal	*coupled_sched; /* decoupled sched */

  void				*aux; /* pointer to user data (if provided) */
  void				*import_root;
  void *volatile		exp;
  void				*named_exptab;

  volatile pid_t		tid; /* TID in process mode at beginning */
  struct pip_root		*task_root;

  pip_task_misc_t		*misc;
  /* 16 words */
} pip_task_annex_t;

typedef void(*pip_deffered_proc_t)(void*);

typedef struct pip_task_internal_body {
  /* Most frequently accessed (time critial) part */
  pip_task_t			queue; /* !!! DO NOT MOVE THIS AROUND !!! */
  pip_task_t			schedq;	/* scheduling queue head */
  struct pip_task_internal	*task_sched; /* scheduling task */
  pip_tls_t			tls;	     /* Thread Local Storage */
  pip_ctx_p			ctx_suspend; /* context to resume */
  pip_ctx_p			*ctx_savep; /* for fcontext */
  /* 8 words */
  pip_deffered_proc_t		deffered_proc;
  void				*deffered_arg;
  pip_atomic_t			refcount; /* reference count */
  pip_task_t			oodq;	   /* out-of-damain queue */
  pip_spinlock_t		lock_oodq; /* lock for OOD queue */
  struct {
    int16_t			pipid;	  /* PiP ID */
    int16_t			schedq_len; /* length of schedq */
    volatile int16_t		oodq_len; /* indictaes ood task is added */
    uint8_t			type; /* PIP_TYPE_ROOT, PIP_TYPE_TASK, ... */
    char			state; /* PIP_TYPE_ROOT, PIP_TYPE_TASK, ... */
  };
  /* 15 words */
} pip_task_internal_body_t;

#ifdef PIP_CONCAT_STRUCT
typedef struct pip_task_internal {
  pip_task_internal_body_t	body;
  pip_task_annex_t		annex;
} pip_task_internal_t;
#define TA(T)		(&((T)->body))
#define AA(T)		(&((T)->body.annex))
#define MA(T)		((T)->body.annex.misc)
#else
typedef struct pip_task_internal {
  pip_task_internal_body_t	body;
  pip_task_annex_t		*annex;
} pip_task_internal_t;
#define TA(T)		(&((T)->body))
#define AA(T)		((T)->annex)
#define MA(T)		((T)->annex->misc)
#endif

#define PIP_TASKI(TASK)		((pip_task_internal_t*)(TASK))
#define PIP_TASKQ(TASK)		((pip_task_t*)(TASK))

#define PIP_TYPE_NULL		(0x00)
#define PIP_TYPE_ROOT		(0x01)
#define PIP_TYPE_TASK		(0x02)

#define PIP_TASK_RUNNING		('R')
#define PIP_TASK_SUSPENDED		('S')
#define PIP_TASK_RUNNING_NOSCHED	('r')
#define PIP_TASK_SUSPENDED_NOSCHED	('s')

#define PIP_RUN(T)		( TA(PIP_TASKI(T))->state = PIP_TASK_RUNNING   )
#define PIP_SUSPEND(T)		( TA(PIP_TASKI(T))->state = PIP_TASK_SUSPENDED )

#define PIP_IS_RUNNING(T)	( TA( PIP_TASKI(T))->state == PIP_TASK_RUNNING   )
#define PIP_IS_SUSPENDED(T)	( TA(PIP_TASKI(T))->state == PIP_TASK_SUSPENDED )

#define PIP_ISA_ROOT(T)		( TA(PIP_TASKI(T))->type & PIP_TYPE_ROOT )
#define PIP_ISA_TASK(T)		\
  ( TA(PIP_TASKI(T))->type & ( PIP_TYPE_ROOT | PIP_TYPE_TASK ) )

#define PIP_BUSYWAIT_COUNT	(1000)

#define PIP_MASK32		(0xFFFFFFFF)

/* The following env vars must be copied */
/* if a PiP may have different env set.  */
typedef struct pip_env {
  char	*stop_on_start;
  char	*gdb_path;
  char	*gdb_command;
  char	*gdb_signals;
  char	*show_maps;
  char	*show_pips;
  char	*__reserved__[10];
} pip_env_t;

typedef struct pip_root {
  /* sanity check info */
  char				magic[PIP_MAGIC_WLEN];
  unsigned int			version;
  size_t			size_whole;
  size_t			size_root;
  size_t			size_task;
  size_t			size_annex;
  size_t			size_misc;
  /* actual root info */
  void *volatile		export_root;
  pip_atomic_t			ntasks_blocking;
  pip_atomic_t			ntasks_count;
  int				ntasks_accum; /* not used */
  int				ntasks;
  int				pipid_curr;
  uint32_t			opts;
  int				yield_iters;
  size_t			page_size;
  pip_clone_t			*cloneinfo; /* only valid with process:preload */
  pip_task_internal_t		*task_root; /* points to tasks[ntasks] */
  pip_spinlock_t		lock_tasks; /* lock for finding a new task id */
  pip_sem_t			lock_glibc; /* lock for GLIBC functions */
  pip_sem_t			sync_spawn; /* Spawn synch */
  /* BLT related info */
  size_t			stack_size_blt; /* stack size for BLTs */
  size_t			stack_size_trampoline; /* stack size for trampoline */
  /* signal related members */
  sigset_t			old_sigmask;
  /* for chaining signal handlers */
  struct sigaction		old_sigterm;
  struct sigaction		old_sigchld;
  /* environments */
  pip_env_t			envs;
  /* GDB Interface */
  struct pip_gdbif_root		*gdbif_root;
  /* for backtrace */
  pip_spinlock_t		lock_bt; /* lock for backtrace */
  void				*__reserved__[32]; /* reserved for future use */
  /* PiP tasks array */
  pip_task_internal_t		tasks[];

} pip_root_t;

#define PIP_MIDLEN		(64)

#define PIP_PRIVATE		__attribute__((visibility ("hidden")))

#define pip_likely(x)		__builtin_expect((x),1)
#define pip_unlikely(x)		__builtin_expect((x),0)
#define IF_LIKELY(C)		if( pip_likely( C ) )
#define IF_UNLIKELY(C)		if( pip_unlikely( C ) )

#ifndef __W_EXITCODE
#define __W_EXITCODE(retval,signal)	( (retval) << 8 | (signal) )
#endif
#define PIP_W_EXITCODE(X,S)	__W_EXITCODE(X,S)

INLINE int
pip_able_to_terminate_now( pip_task_internal_t *taski ) {
  DBGF( "[PIPID:%d] flag_exit:%d  RFC:%d",
	TA(taski)->pipid,
	AA(taski)->flag_exit,
	(int) TA(taski)->refcount );
  return
    TA(taski)->refcount    == 0 &&
    TA(taski)->schedq_len  == 0 &&
    TA(taski)->oodq_len    == 0 &&
    AA(taski)->flag_exit;
}

extern pip_root_t		*pip_root;
extern pip_task_internal_t	*pip_task;

extern pip_clone_mostly_pthread_t 	pip_clone_mostly_pthread_ptr;

extern int __attribute__ ((visibility ("default")))
pip_init_task_implicitly( pip_root_t *root,
			  pip_task_internal_t *task );

extern void pip_wakeup( pip_task_internal_t *taski ) PIP_PRIVATE;
extern void pip_wakeup_to_die( pip_task_internal_t *taski ) PIP_PRIVATE;
extern void pip_sched_ood_task( pip_task_internal_t *schedi,
				pip_task_internal_t *taski,
				pip_task_internal_t *wakeup ) PIP_PRIVATE;

extern  void pip_terminate_task( pip_task_internal_t *self ) PIP_PRIVATE;
extern void pip_decouple_context( pip_task_internal_t *taski,
				  pip_task_internal_t *schedi ) PIP_PRIVATE;
extern void pip_couple_context( pip_task_internal_t *schedi,
				pip_task_internal_t *taski ) PIP_PRIVATE;
extern void pip_swap_context( pip_task_internal_t *taski,
			      pip_task_internal_t *nexti ) PIP_PRIVATE;
extern void pip_stack_protect( pip_task_internal_t *taski,
			       pip_task_internal_t *nexti ) PIP_PRIVATE;
extern void pip_stack_unprotect( pip_task_internal_t *taski ) PIP_PRIVATE;
extern void pip_stack_wait( pip_task_internal_t *taski ) PIP_PRIVATE;

extern void pip_do_exit( pip_task_internal_t* ) PIP_PRIVATE;
extern void pip_reset_task_struct( pip_task_internal_t* ) PIP_PRIVATE;
extern void pip_finalize_task( pip_task_internal_t* ) PIP_PRIVATE;
extern void pip_finalize_task_RC( pip_task_internal_t* ) PIP_PRIVATE;

extern int  pip_patch_GOT( char*, char*, void* ) PIP_PRIVATE;
extern void pip_undo_patch_GOT( void ) PIP_PRIVATE;
extern int  pip_wrap_clone( void ) PIP_PRIVATE;

extern void pip_sleep( pip_task_internal_t* ) PIP_PRIVATE;

extern void pip_set_signal_handler( int sig, void(*)(),
				    struct sigaction* ) PIP_PRIVATE;
extern int  pip_raise_signal( pip_task_internal_t*, int ) PIP_PRIVATE;
extern void pip_set_sigmask( int ) PIP_PRIVATE;
extern void pip_unset_sigmask( void ) PIP_PRIVATE;
extern int  pip_signal_wait( int ) PIP_PRIVATE;

extern void pip_page_alloc( size_t, void** ) PIP_PRIVATE;
extern int  pip_check_sync_flag( uint32_t* ) PIP_PRIVATE;

extern int  pip_dlclose( void* );

extern void pip_named_export_init( pip_task_internal_t* ) PIP_PRIVATE;
extern void pip_named_export_fin_all( void ) PIP_PRIVATE;

extern void pip_deadlock_inc( void ) PIP_PRIVATE;
extern void pip_deadlock_dec( void ) PIP_PRIVATE;

extern int  pip_is_threaded_( void ) PIP_PRIVATE;
extern int  pip_is_shared_fd_( void ) PIP_PRIVATE;
extern int  pip_isa_coefd( int ) PIP_PRIVATE;
extern void pip_set_name( pip_task_internal_t* ) PIP_PRIVATE;

extern int  pip_taski_str( char*, size_t, pip_task_internal_t* ) PIP_PRIVATE;
extern int pip_task_str( char *p, size_t sz, pip_task_t *task );
extern size_t pip_idstr( char*, size_t ) PIP_PRIVATE;

extern void pip_page_alloc( size_t, void** ) PIP_PRIVATE;
extern int  pip_count_vec( char** ) PIP_PRIVATE;
extern int  pip_get_dso( int pipid, void **loaded ) PIP_PRIVATE;

extern int  pip_dequeue_and_resume_multiple( pip_task_queue_t*,
					     pip_task_internal_t*,
					     int* ) PIP_PRIVATE;
extern void pip_suspend_and_enqueue_generic( pip_task_internal_t*,
					     pip_task_queue_t*,
					     int,
					     pip_enqueue_callback_t,
					     void* ) PIP_PRIVATE;

extern int  pip_is_magic_ok( pip_root_t* ) PIP_PRIVATE;
extern int  pip_is_version_ok( pip_root_t* ) PIP_PRIVATE;
extern int  pip_are_sizes_ok( pip_root_t* ) PIP_PRIVATE;

extern void pip_debug_on_exceptions( pip_task_internal_t* ) PIP_PRIVATE;

INLINE int pip_are_flags_exclusive( uint32_t flags, uint32_t val ) {
  return ( flags & val ) == ( flags | val );
}

/* semaphore */

INLINE void pip_sem_init( pip_sem_t *sem ) {
  (void) sem_init( sem, 1, 0 );
}

INLINE void pip_sem_post( pip_sem_t *sem ) {
  ASSERTS( sem_post( sem ) );
}

INLINE void pip_sem_wait( pip_sem_t *sem ) {
  while( 1 ) {
    if( sem_wait( sem ) == 0 ) break;
    ASSERTS( errno != EINTR );
  }
}

INLINE void pip_sem_fin( pip_sem_t *sem ) {
  (void) sem_destroy( sem );
}

INLINE int pip_get_pipid_( void ) {
  if( !pip_is_initialized() ) return PIP_PIPID_NULL;
  return TA(pip_task)->pipid;
}

INLINE int pip_check_pipid( int *pipidp ) {
  int pipid;

  if( !pip_is_initialized() ) RETURN( EPERM  );
  if( pipidp == NULL        ) RETURN( EINVAL );
  pipid = *pipidp;

  switch( pipid ) {
  case PIP_PIPID_ROOT:
    break;
  case PIP_PIPID_ANY:
  case PIP_PIPID_NULL:
    RETURN( EINVAL );
    break;
  case PIP_PIPID_MYSELF:
    if( pip_isa_root() ) {
      *pipidp = PIP_PIPID_ROOT;
    } else if( pip_isa_task() ) {
      *pipidp = TA(pip_task)->pipid;
    } else {
      RETURN( ENXIO );		/* ???? */
    }
    break;
  default:
    if( pipid <  0                ) RETURN( EINVAL );
    if( pipid >= pip_root->ntasks ) RETURN( ERANGE );
  }
  return 0;
}

INLINE pip_task_internal_t *pip_get_task( int pipid ) {
  pip_task_internal_t 	*taski = NULL;

  switch( pipid ) {
  case PIP_PIPID_ROOT:
    taski = pip_root->task_root;
    break;
  default:
    taski = &pip_root->tasks[pipid];
    break;
  }
  return taski;
}

INLINE void pip_system_yield( void ) {
  if( pip_root != NULL && pip_is_threaded_() ) {
    (void) pthread_yield();
  } else {
    sched_yield();
  }
}

int pip_set_syncflag( uint32_t flags );

#endif /* DOXYGEN */

#endif /* _pip_internal_h_ */
