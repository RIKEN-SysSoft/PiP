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
#ifndef PIP_USE_MUTEX
#include <semaphore.h>
#endif

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

#define PIP_STACK_SIZE	(8*1024*1024) /* 8 MiB */
#define PIP_MIN_STACK_SIZE	(1*1024*1024) /* 1 MiB */
#define PIP_STACK_ALIGN	(256)

#define PIP_MASK32		(0xFFFFFFFF)

#define PIP_EXITED		(1)
#define PIP_EXIT_FINALIZE	(2)

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
  /* Frequently accessing part (packed into a cache block */
  union {
    struct {
      pip_ulp_t		queue;	     /* !!! DO NOT MOVE THIS AROUND !!! */
      pip_ulp_t		schedq;	     /* ULP scheduling queue */
      pip_spinlock_t	lock_schedq; /* lock of scheduling queue (no need ?) */
      struct pip_task	*task_sched; /* scheduling task */
      int32_t		pipid;	     /* PiP ID */
      int32_t		type;	/* PIP_TYPE_TASK, PIP_TYPE_ULP, or ... */
    };
    char		__gap0__[PIP_CACHEBLK_SZ];
  };
  /* PiP ULP (type==PIP_TYPE_ULP) */
  union {
    struct {
      void		*ulp_stack;   /* stack area of this ULP */
      pip_ctx_t		*ctx_suspend; /* context to resume */
#ifdef PIP_USE_MUTEX
      pthread_mutex_t	mutex_wait; /* mutex to block at pip_wait() */
#else
      sem_t		sem_wait; /* semaphore rp block at pip_wait() */
#endif
    };
    char		__gap1__[PIP_CACHEBLK_SZ];
  };
  /* PiP task (type==PIP_TYPE_TASK) */
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
  pip_ctx_t		*ctx_exit; /* longjump context for pip_exit() */
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

#define PIP_TASK(ULP)	((pip_task_t*)(ULP))
#define PIP_ULP(TASK)	((pip_ulp_t*)(TASK))
#define PIP_ULP_CTX(U)	(PIP_TASK(U)->ctx_suspend)

#define PIP_ULP_INIT(L)					\
  do { PIP_ULP_NEXT(L) = (L); PIP_ULP_PREV(L) = (L); } while(0)

#define PIP_ULP_NEXT(L)		((L)->next)
#define PIP_ULP_PREV(L)		((L)->prev)
#define PIP_ULP_PREV_NEXT(L)	((L)->prev->next)
#define PIP_ULP_NEXT_PREV(L)	((L)->next->prev)

#define PIP_ULP_ENQ_FIRST(L,E)				\
  do { PIP_ULP_NEXT(E)   = PIP_ULP_NEXT(L);		\
    PIP_ULP_PREV(E)      = (L);				\
    PIP_ULP_NEXT_PREV(L) = (E);				\
    PIP_ULP_NEXT(L)      = (E); } while(0)

#define PIP_ULP_ENQ_LAST(L,E)				\
  do { PIP_ULP_NEXT(E)   = (L);				\
    PIP_ULP_PREV(E)      = PIP_ULP_PREV(L);		\
    PIP_ULP_PREV_NEXT(L) = (E);				\
    PIP_ULP_PREV(L)      = (E); } while(0)

#define PIP_ULP_DEQ(L)					\
  do { PIP_ULP_NEXT_PREV(L) = PIP_ULP_PREV(L);		\
    PIP_ULP_PREV_NEXT(L) = PIP_ULP_NEXT(L); 		\
    PIP_ULP_INIT(L); } while(0)

#define PIP_ULP_MOVE_QUEUE(P,Q)				\
  do { if( !PIP_ULP_ISEMPTY(Q) ) {			\
    PIP_ULP_NEXT_PREV(Q) = (P);				\
    PIP_ULP_PREV_NEXT(Q) = (P);				\
    PIP_ULP_NEXT(P) = PIP_ULP_NEXT(Q);			\
    PIP_ULP_PREV(P) = PIP_ULP_PREV(Q);			\
    PIP_ULP_INIT(Q); } } while(0)

#define PIP_ULP_ISEMPTY(L)				\
  ( PIP_ULP_NEXT(L) == (L) && PIP_ULP_PREV(L) == (L) )

#define PIP_ULP_FOREACH(L,E)				\
  for( (E)=(L)->next; (L)!=(E); (E)=PIP_ULP_NEXT(E) )

#define PIP_ULP_FOREACH_SAFE(L,E,TV)				\
  for( (E)=(L)->next, (TV)=PIP_ULP_NEXT(E);			\
       (L)!=(E);						\
       (E)=(TV), (TV)=PIP_ULP_NEXT(TV) )

#define PIP_ULP_FOREACH_SAFE_XXX(L,E,TV)			\
  for( (E)=(L), (TV)=PIP_ULP_NEXT(E); (L)!=(E); (E)=(TV) )

#define PIP_LIST_INIT(L)	PIP_ULP_INIT(L)
#define PIP_LIST_ISEMPTY(L)	PIP_ULP_ISEMPTY(L)
#define PIP_LIST_ADD(L,E)	PIP_ULP_ENQ_LAST(L,E)
#define PIP_LIST_DEL(E)		PIP_ULP_DEQ(E)
#define PIP_LIST_FOREACH(L,E)	PIP_ULP_FOREACH(L,E)

#define pip_likely(x)		__builtin_expect((x),1)
#define pip_unlikely(x)		__builtin_expect((x),0)

#define PIP_MIDLEN	(64)

#define ERRJ		{ DBG;                goto error; }
#define ERRJ_ERRNO	{ DBG; err=errno;     goto error; }
#define ERRJ_ERR(ENO)	{ DBG; err=(ENO);     goto error; }
#define ERRJ_CHK(FUNC)	{ if( (FUNC) ) goto error; }

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#endif /* _pip_internal_h_ */
