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

#ifdef PIP_INTERNAL_FUNCS

#include <ucontext.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <pip_machdep.h>
#include <pip_clone.h>
#include <pip_debug.h>

/** ULP supported version **/
#define PIP_BASE_VERSION	(0x2000U)
/** The very first stable release version
#define PIP_BASE_VERSION	(0x1000U)
**/

#define PIP_VERSION		PIP_BASE_VERSION

#define PIP_ROOT_ENV		"PIP_ROOT"
#define PIP_TASK_ENV		"PIP_TASK"

#define PIP_MAGIC_WORD		"PrcInPrc"
#define PIP_MAGIC_LEN		(8)

#define MAIN_FUNC		"main"

#define PIP_PIPID_NONE		(-999)

#define PIP_LD_SOLIBS		{ NULL }

#define PIP_ULP_STACK_SIZE	(8*1024*1024) /* 8 MiB */
#define PIP_ULP_MIN_STACK_SIZE	(1*1024*1024) /* 1 MiB */
#define PIP_ULP_STACK_ALIGN	(256)

#define PIP_MASK32		(0xFFFFFFFF)

typedef	int(*main_func_t)(int,char**,char**);
typedef int(*mallopt_t)(int,int);
typedef void(*free_t)(void*);

typedef	void(*ctype_init_t)(void);
typedef void(*glibc_init_t)(int,char**,char**);
typedef	void(*fflush_t)(FILE*);

typedef struct {
  /* functions */
  main_func_t		main;	     /* main function address */
  ctype_init_t		ctype_init;  /* to call __ctype_init() */
  glibc_init_t		glibc_init;  /* only in patched Glibc */
  fflush_t		libc_fflush; /* to call fflush() at the end */
  mallopt_t		mallopt;     /* to call mallopt() */
  free_t		free;	     /* to override free() - EXPERIMENTAL*/
  /* variables */
  char 			***libc_argvp; /* to set __libc_argv */
  int			*libc_argcp;   /* to set __libc_argc */
  char			***environ;    /* pointer to the environ variable */
} pip_symbols_t;

typedef struct {
  int 			pipid;
  int	 		coreno;
  char 			*prog;
  char 			**argv;
  char 			**envv;
} pip_spawn_args_t;

#define PIP_TYPE_NONE	(0)
#define PIP_TYPE_ROOT	(1)
#define PIP_TYPE_TASK	(2)
#define PIP_TYPE_ULP	(3)

typedef struct pip_task {
  int			pipid;	 /* PiP ID */
  int			type;	 /* PIP_TYPE_TASK or PIP_TYPE_ULP */

  void			*export;
  ucontext_t		*ctx_exit;
  void			*loaded;
  pip_symbols_t		symbols;
  pip_spawn_args_t	args;	/* arguments for a PiP task */
  int			retval;

  int			boundary[0];
  union {
    struct {			/* for PiP tasks */
      pid_t		pid;	/* PID in process mode */
      pthread_t		thread;	/* thread */
      pip_spawnhook_t	hook_before;
      pip_spawnhook_t	hook_after;
      void		*hook_arg;
    };
    struct {			/* for PiP ULPs */
      struct pip_task	*task_parent;
      void		*stack;
      struct pip_ulp	*ulp;
    };
  };
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
  pip_clone_t	 	*cloneinfo;   /* only valid with PIP_MODE_PIPCLONE */

  size_t		stack_size;

  pip_spinlock_t	lock_stack_flist; /* ULP: lock for stack free list */
  void 			*stack_flist;	  /* ULP: stack free list */

  pip_task_t		*task_root; /* points to tasks[ntasks] */
  pip_spinlock_t	lock_tasks; /* lock for finding a new task id */
  pip_task_t		tasks[];
} pip_root_t;

  /* the following functions deeply depends on PiP execution mode */

  int    pip_get_thread( int pipid, pthread_t *threadp );
  int    pip_is_pthread( int *flagp );
  int    pip_is_shared_fd( int *flagp );
  int    pip_is_shared_sighand( int *flagp );
  char **pip_copy_vec( char *add0, char *add1, char *add2, char **vecsrc );

#ifdef __cplusplus
}
#endif

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#endif /* PIP_INTERNAL_FUNCS */

#endif /* _pip_internal_h_ */
