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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

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
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>
#include <fcntl.h>
#include <link.h>
#include <dlfcn.h>

#include <pip_blt.h>
#include <pip_clone.h>
#include <pip_context.h>
#include <pip_signal.h>
#include <pip_util.h>
#include <pip_debug.h>
#include <pip_gdbif.h>

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
#define PIP_MAGIC_WLEN		(8)

#define PIP_LD_SOLIBS		{ NULL }

#define PIP_STACK_SIZE		(8*1024*1024) /* 8 MiB */
#define PIP_STACK_SIZE_MIN	(1*1024*1024) /* 1 MiB */
#define PIP_STACK_ALIGN		(256)

#define PIP_EXITED		(1)
#define PIP_EXIT_WAITED		(2)
#define PIP_ABORT		(9)

struct pip_root;
struct pip_task_internal;
struct pip_task_annex;
struct pip_gdbif_task;
struct pip_gdbif_root;

typedef struct pip_cacheblk {
  char		cacheblk[PIP_CACHEBLK_SZ];
} pip_cacheblk_t;

typedef volatile uint8_t	pip_stack_protect_t;
typedef pip_stack_protect_t	*pip_stack_protect_p;

typedef struct pip_task_internal {
  union {
    struct {
      /* Most frequently accessed (time critial) part */
      pip_task_t		queue; /* !!! DO NOT MOVE THIS AROUND !!! */
      pip_task_t		schedq;	/* scheduling queue */
      struct pip_task_internal	*task_sched; /* scheduling task */
      pip_tls_t			tls;
      pip_ctx_p			ctx_suspend; /* context to resume */
      pip_stack_protect_p	flag_stackpp; /* pointer to unprotect stack */
      /* end of one cache block (64 bytes) */
      /* less frequently accessed part follows */
      pip_atomic_t		ntakecare; /* tasks must be taken care */
      struct {
	int16_t			pipid;	    /* PiP ID */
	uint16_t		schedq_len; /* length of schedq */
	volatile uint16_t	oodq_len; /* indictaes ood task is added */
	volatile uint8_t	type; /* PIP_TYPE_ROOT, PIP_TYPE_TASK, ... */
	pip_stack_protect_t	flag_stackp; /* stack protection flag */
	volatile char		state; /* BLT state */
	volatile uint8_t	flag_exit; /* if this task is terminated */
	volatile uint8_t	flag_wakeup; /* flag to wakeup */
      };
      struct pip_task_internal	*decoupled_sched; /* decoupled sched */
      struct pip_task_annex	*annex;
#ifdef PIP_USE_FCONTEXT
      pip_ctx_p			*ctx_savep;
#endif
    };
    pip_cacheblk_t		__align__[2];
  };
} pip_task_internal_t;

typedef	int  (*main_func_t)(int,char**,char**);
typedef	int  (*start_func_t)(void*);
typedef int  (*mallopt_t)(int,int);
typedef void (*free_t)(void*);
typedef	void (*ctype_init_t)(void);
typedef void (*glibc_init_t)(int,char**,char**);
typedef	void (*fflush_t)(FILE*);
typedef int  (*named_export_fin_t)(struct pip_task_internal*);
typedef int  (*pip_clone_mostly_pthread_t)
( pthread_t *newthread, int, int, size_t, void *(*)(void *), void*, pid_t* );
typedef void (*pip_set_tid_t)( pthread_t, int, int* );

typedef struct {
  /* functions */
  main_func_t		main;	      /* main function address */
  start_func_t		start;	      /* strat function instead of main */
  ctype_init_t		ctype_init;   /* to call __ctype_init() */
  fflush_t		libc_fflush;  /* to call fflush() at the end */
  mallopt_t		mallopt;      /* to call mallopt() */
  named_export_fin_t	named_export_fin; /* for free()ing hash entries */
  /* glibc workaround */
  pip_set_tid_t		pip_set_tid; /* to correct pd->tid */
  /* glibc variables */
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
  int			*fd_list; /* list of close-on-exec FDs */
  pip_task_queue_t	*queue;
} pip_spawn_args_t;

typedef sem_t			pip_sem_t;

typedef struct pip_task_annex {
  /* less and less frequently accessed part follows */
  pip_ctx_p			ctx_sleep; /* context to resume */
  pip_sem_t			sleep;
  void				*stack_sleep;

  pip_task_t			oodq;	   /* out-of-damain queue */
  pip_spinlock_t		lock_oodq; /* lock for OOD queue */

  void *volatile		exp;
  void				*named_exptab;

  void				*aux; /* pointer to user data (if provided) */

  uint32_t			opts;
  uint32_t			opts_sync;
  volatile int32_t		status;	   /* exit value */
  volatile int			flag_sigchld; /* termination in thread mode */

  volatile pid_t		tid; /* TID in process mode at beginning */
  volatile pthread_t		thread;	/* thread */
  void				*loaded; /* loaded DSO handle */
  struct pip_root		*task_root;
  /* spawn info */
  pip_spawn_args_t		args;	 /* arguments for a PiP task */
  pip_symbols_t			symbols; /* symbols */
  pip_spawnhook_t		hook_before; /* before spawning hook */
  pip_spawnhook_t		hook_after;  /* after spawning hook */
  void				*hook_arg;   /* hook arg */
  /* GDB interface */
  void				*load_address;
  struct pip_gdbif_task		*gdbif_task; /* GDB if */
  int				sleep_busy;
} pip_task_annex_t;

#define PIP_TASKI(TASK)		((pip_task_internal_t*)(TASK))
#define PIP_TASKQ(TASK)		((pip_task_t*)(TASK))

#define PIP_TYPE_NONE		(0x00)
#define PIP_TYPE_ROOT		(0x01)
#define PIP_TYPE_TASK		(0x02)

#define PIP_TASK_RUNNING		('R')
#define PIP_TASK_SUSPENDED		('S')
#define PIP_TASK_RUNNING_NOSCHED	('r')
#define PIP_TASK_SUSPENDED_NOSCHED	('s')

#define PIP_RUN(T)		( PIP_TASKI(T)->state = PIP_TASK_RUNNING   )
#define PIP_SUSPEND(T)		( PIP_TASKI(T)->state = PIP_TASK_SUSPENDED )

#define PIP_IS_RUNNING(T)	( PIP_TASKI(T)->state == PIP_TASK_RUNNING   )
#define PIP_IS_SUSPENDED(T)	( PIP_TASKI(T)->state == PIP_TASK_SUSPENDED )

#define PIP_ISA_ROOT(T)		( PIP_TASKI(T)->type & PIP_TYPE_ROOT )
#define PIP_ISA_TASK(T)		\
  ( PIP_TASKI(T)->type & ( PIP_TYPE_ROOT | PIP_TYPE_TASK ) )

#define PIP_BUSYWAIT_COUNT	(10*1000)

#define PIP_MASK32		(0xFFFFFFFF)

typedef struct pip_root {
  /* sanity check info */
  char			magic[PIP_MAGIC_WLEN];
  unsigned int		version;
  size_t		size_whole;
  size_t		size_root;
  size_t		size_task;
  size_t		size_annex;
  /* actual root info */
  pip_spinlock_t	lock_glibc;   /* lock for GLIBC functions */
  pip_atomic_t		ntasks_blocking;
  pip_atomic_t		ntasks_count;
  int			ntasks_accum; /* not used */
  int			ntasks;
  int			pipid_curr;
  uint32_t		opts;
  int			yield_iters;
  size_t		page_size;
  pip_clone_t		*cloneinfo; /* only valid with process:preload */
  pip_task_internal_t	*task_root; /* points to tasks[ntasks] */
  pip_spinlock_t	lock_tasks; /* lock for finding a new task id */
  /* Spawn synch */
  pip_sem_t		sync_root;
  pip_sem_t		sync_task;
  /* BLT related info */
  size_t		stack_size_blt; /* stack size for BLTs */
  size_t		stack_size_sleep; /* stack size for sleeping */
  /* signal related members */
  sigset_t		old_sigmask;
  /* for chaining signal handlers */
  struct sigaction	old_sighup;
  struct sigaction	old_sigterm;
  struct sigaction	old_sigchld;
  struct sigaction	old_sigsegv;
  /* GDB Interface */
  struct pip_gdbif_root	*gdbif_root;
  /* for backtrace */
  pip_spinlock_t	lock_bt; /* lock for backtrace */
  /* PiP tasks array */
  pip_task_annex_t	*annex;
  pip_task_internal_t	tasks[];

} pip_root_t;

typedef int  (*pip_init_t)(pip_root_t*,pip_task_internal_t*);

extern pip_clone_mostly_pthread_t 	pip_clone_mostly_pthread_ptr;
extern int  pip_get_thread_id( pthread_t th );
extern void pip_set_thread_id( pthread_t th, int );

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

extern pip_root_t		*pip_root;
extern pip_task_internal_t	*pip_task;

extern int __attribute__ ((visibility ("default")))
pip_init_task_implicitly( pip_root_t *root,
			  pip_task_internal_t *task );

extern void pip_do_exit( pip_task_internal_t*, int ) PIP_PRIVATE;
extern void pip_reset_task_struct( pip_task_internal_t* ) PIP_PRIVATE;
extern int  pip_able_to_terminate_immediately( pip_task_internal_t* )
  PIP_PRIVATE;
extern void pip_finalize_task( pip_task_internal_t* ) PIP_PRIVATE;
extern void pip_finalize_task_RC( pip_task_internal_t* ) PIP_PRIVATE;

extern int  pip_patch_GOT( char*, char*, void* ) PIP_PRIVATE;
extern void pip_undo_patch_GOT( void ) PIP_PRIVATE;
extern int  pip_wrap_clone( void ) PIP_PRIVATE;

#ifdef AH
extern void pip_swap_context( pip_task_internal_t*,
			      pip_task_internal_t* ) PIP_PRIVATE;
extern void pip_decouple_context( pip_task_internal_t*,
				  pip_task_internal_t* ) PIP_PRIVATE;
extern void pip_couple_context( pip_task_internal_t*,
				pip_task_internal_t* ) PIP_PRIVATE;
extern void pip_stack_protect( pip_task_internal_t*,
			       pip_task_internal_t* ) PIP_PRIVATE;
#endif
extern void pip_sleep( pip_task_internal_t* ) PIP_PRIVATE;

extern void pip_set_signal_handler( int sig, void(*)(),
				    struct sigaction* ) PIP_PRIVATE;
extern int  pip_raise_signal( pip_task_internal_t*, int ) PIP_PRIVATE;
extern void pip_set_sigmask( int ) PIP_PRIVATE;
extern void pip_unset_sigmask( void ) PIP_PRIVATE;
extern int  pip_signal_wait( int ) PIP_PRIVATE;

extern void pip_page_alloc( size_t, void** ) PIP_PRIVATE;
extern int  pip_check_sync_flag( uint32_t ) PIP_PRIVATE;
extern int  pip_check_task_flag( uint32_t ) PIP_PRIVATE;

extern int  pip_dlclose( void* );

extern void pip_suspend_and_enqueue_generic( pip_task_internal_t*,
					     pip_task_queue_t*,
					     int,
					     pip_enqueue_callback_t,
					     void* ) PIP_PRIVATE;

extern void pip_named_export_init( pip_task_internal_t* ) PIP_PRIVATE;
extern void pip_named_export_fin_all( void ) PIP_PRIVATE;

extern void pip_deadlock_inc( void ) PIP_PRIVATE;
extern void pip_deadlock_dec( void ) PIP_PRIVATE;

extern int  pip_is_threaded_( void ) PIP_PRIVATE;
extern int  pip_is_shared_fd_( void ) PIP_PRIVATE;
extern int  pip_isa_coefd( int ) PIP_PRIVATE;
extern void pip_set_name( pip_task_internal_t* ) PIP_PRIVATE;

extern void pip_pipidstr( pip_task_internal_t *taski, char *buf ) PIP_PRIVATE;
extern void pip_page_alloc( size_t, void** ) PIP_PRIVATE;
extern int  pip_count_vec( char** ) PIP_PRIVATE;
extern int  pip_get_dso( int pipid, void **loaded ) PIP_PRIVATE;

extern int  pip_dequeue_and_resume_multiple( pip_task_internal_t*,
					     pip_task_queue_t*,
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

extern void pip_debug_on_exceptions( void ) PIP_PRIVATE;

extern void pip_message( FILE *, char*, const char*, va_list ) PIP_PRIVATE;

extern int  pip_idstr( char *buf, size_t sz ); /* donot make this private */

/* semaphore */

inline static void pip_sem_init( pip_sem_t *sem ) {
  (void) sem_init( sem, 1, 0 );
}

inline static void pip_sem_post( pip_sem_t *sem ) {
  errno = 0;
  (void) sem_post( sem );
  ASSERT( errno );
}

inline static void pip_sem_wait( pip_sem_t *sem ) {
  errno = 0;
  (void) sem_wait( sem );
  ASSERT( errno !=0 && errno != EINTR );
}

inline static void pip_sem_fin( pip_sem_t *sem ) {
  (void) sem_destroy( sem );
}

inline static int pip_get_pipid_( void ) {
  if( !pip_is_initialized() ) return PIP_PIPID_NULL;
  return pip_task->pipid;
}

inline static int pip_check_pipid( int *pipidp ) {
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
      *pipidp = pip_task->pipid;
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

inline static pip_task_internal_t *pip_get_task( int pipid ) {
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

inline static void pip_system_yield( void ) {
  if( pip_is_threaded_() ) {
    int pthread_yield( void );
    (void) pthread_yield();
  } else {
    sched_yield();
  }
}

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#endif /* _pip_internal_h_ */
