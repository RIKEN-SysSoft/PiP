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

#include <pthread.h>
#include <ucontext.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <pip_machdep.h>
#include <pip_clone.h>
#include <pip_debug.h>

#ifndef PIP_ULP_PRINT_SIZE
#include <pip.h>
#endif

/** ULP supported version **/
#define PIP_BASE_VERSION	(0x2000U)
/** The very first stable release version
#define PIP_BASE_VERSION	(0x1000U)
**/

#define PIP_VERSION		PIP_BASE_VERSION

#define PIP_ROOT_ENV		"PIP_ROOT"

#define PIP_MAGIC_WORD		"PrcInPrc"
#define PIP_MAGIC_LEN		(8)

#define MAIN_FUNC		"main"

#define PIP_PIPID_NONE		(-999)

#define PIP_LD_SOLIBS		{ NULL }

#define PIP_ULP_STACK_SIZE	(8*1024*1024) /* 8 MiB */
#define PIP_ULP_MIN_STACK_SIZE	(1*1024*1024) /* 1 MiB */
#define PIP_ULP_STACK_ALIGN	(256)

#define PIP_MASK32		(0xFFFFFFFF)

typedef	int(*main_func_t)(int,char**);
typedef int(*mallopt_t)(int,int);
typedef void(*free_t)(void*);

typedef	void(*ctype_init_t)(void);
typedef void(*glibc_init_t)(int,char**,char**);
typedef	void(*fflush_t)(FILE*);

typedef	void(*pipulp_terminated_t)(void*);

#define PIP_ULP_NEXT(L)		(((pip_dlist_t*)(L))->next)
#define PIP_ULP_PREV(L)		(((pip_dlist_t*)(L))->prev)
#define PIP_ULP_PREV_NEXT(L)	(((pip_dlist_t*)(L))->prev->next)
#define PIP_ULP_NEXT_PREV(L)	(((pip_dlist_t*)(L))->next->prev)

#define PIP_ULP_LIST_INIT(L)			\
  do { PIP_ULP_NEXT(L) = (pip_dlist_t*)(L);		\
    PIP_ULP_PREV(L)    = (pip_dlist_t*)(L); } while(0)

#define PIP_ULP_ENQ(L,R)						\
  do { PIP_ULP_NEXT(L)   = PIP_ULP_NEXT(R);				\
    PIP_ULP_PREV(L)      = (pip_dlist_t*)(R);				\
    PIP_ULP_NEXT(R)      = (pip_dlist_t*)(L);				\
    PIP_ULP_NEXT_PREV(R) = (pip_dlist_t*)(L); } while(0)

#define PIP_ULP_DEQ(L)					\
  do { PIP_ULP_NEXT_PREV(L) = PIP_ULP_PREV(L);			\
    PIP_ULP_PREV_NEXT(L)    = PIP_ULP_NEXT(L); } while(0)

#define PIP_ULP_NULLQ(R)	( PIP_ULP_NEXT(R) == PIP_ULP_PREV(R) )

#define PIP_ULP_ENQ_LOCK(L,R,lock)		\
  do { pip_spin_lock(lock);			\
    PIP_ULP_ENQ((L),(R));			\
    pip_spin_unlock(lock); } while(0)

#define PIP_ULP_DEQ_LOCK(L,lock)		\
  do { pip_spin_lock(lock);			\
    PIP_ULP_DEQ((L));				\
    pip_spin_unlock(lock); } while(0)

typedef struct pip_dlist {
  struct pip_dlist	*next;
  struct pip_dlist	*prev;
} pip_dlist_t;

typedef struct {
  /* functions */
  main_func_t		main;	     /* main function address */
  ctype_init_t		ctype_init;  /* to call __ctype_init() */
  glibc_init_t		glibc_init;  /* only in patched Glibc */
  fflush_t		libc_fflush; /* to call fflush() at the end */
  mallopt_t		mallopt;     /* to call mapllopt() */
  free_t		free;	     /* to override free() :EXPERIMENTAL*/
  /* variables */
  char 			***libc_argvp; /* to set __libc_argv */
  int			*libc_argcp;   /* to set __libc_argc */
  char			***environ;  /* pointer to the environ variable */
} pip_symbols_t;

struct pip_task;

typedef struct pip_ulp_ {
  struct pip_task	*ulp_root;
  ucontext_t		ctx_ulp;
  size_t		stack_sz;
  void			*stack_region;
  void			*loaded;
  pip_symbols_t		symbols;
  char			*prog;
  char			**argv;
  char			**envv;
  pipulp_terminated_t	termcb;
} PIP_ULP_t;

#ifndef PIP_ULP_PRINT_SIZE
typedef struct {
  int 			pipid;
  int	 		coreno;
  pip_spawnhook_t	hook_before;
  pip_spawnhook_t	hook_after;
  void			*hook_arg;
  char 			*prog;
  char 			**argv;
  char 			**envv;
} pip_spawn_args_t;

typedef struct pip_task {
  int			pipid;	 /* PiP ID */
  pid_t			pid;	 /* Pid in process mode */
  pthread_t		thread;	 /* thread */
  //int			coreno;	 /* CPU core binding */
  int	 		retval;	 /* return value of exit() */
  void			*loaded; /* loaded DSOs */
  pip_symbols_t		symbols;
  pip_spawn_args_t	*args;	 /* arguments for PiP task */
  ucontext_t 		*ctx_exit; /* to implement pip_exit() */

  pip_dlist_t		ulp_root;
  PIP_ULP_t		*ulp_curr;
  void			*ulp_stack_list;

  volatile void		*export;  /* PiP export region */
} pip_task_t;

typedef struct {
  char			magic[PIP_MAGIC_LEN];
  unsigned int		version;
  size_t		root_size;
  size_t		size;
  pthread_t		thread;
  pip_spinlock_t	lock_ldlinux;
  volatile void		*export;
  pip_clone_t	 	*cloneinfo;
  char			*pip_root_env;
  free_t		free;	/* free() function address */
  size_t		page_size;
  int			opts;
  int			pid;
  int			pipid_curr;
  int			ntasks_curr;
  int			ntasks_accum;
  int			ntasks;
  pip_task_t		tasks[];
} pip_root_t;
#endif

#endif

#endif
