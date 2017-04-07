/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2016
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

//#define HAVE_GLIBC_INIT

#define PIP_BASE_VERSION	(0x0100U)
#ifndef HAVE_GLIBC_INIT
#define PIP_VERSION		PIP_BASE_VERSION
#else
#define PIP_VERSION		(PIP_BASE_VERSION|0x8)
#endif

#define PIP_ROOT_ENV		"PIP_ROOT"

#define PIP_MAGIC_WORD		"PrcInPrc"
#define PIP_MAGIC_LEN		(8)

#define MAIN_FUNC		"main"

#define PIP_PIPID_NONE		(-999)

#define PIP_ERRMSG_TAG		"#PIP Error (pid:%d)# "

#define PIP_LD_SOLIBS		{ NULL }

typedef	int(*main_func_t)(int,char**);

#ifndef HAVE_GLIBC_INIT
typedef	void(*ctype_init_t)(void);
#else
typedef void(*glibc_init_t)(int,char**,char**);
#endif
typedef	void(*fflush_t)(FILE*);

typedef int(*mallopt_t)(int,int);
typedef void(*free_t)(void*);

typedef struct pip_spawn_args {
  int 			pipid;
  int	 		coreno;
  pip_spawnhook_t	hook_before;
  pip_spawnhook_t	hook_after;
  void			*hook_arg;
  char 			*prog;
  char 			**argv;
  char 			**envv;
} pip_spawn_args_t;

typedef struct {
  int			pipid;	/* PiP ID */
  pid_t			pid;	/* PID in process mode */
  int			coreno;	/* CPU core binding */
  int	 		retval;	/* return value of exit() */
  pthread_t		thread;	/* thread */
  void			*loaded; /* loaded DSOs */
  pip_spawn_args_t	*args;	 /* arguments for thread */

  /* symbol addresses to call or setup before jumping into the main */
  main_func_t		mainf;	/* main function address */
#ifndef HAVE_GLIBC_INIT
  ctype_init_t		ctype_init; /* to call __ctype_init() */
#else
  glibc_init_t		glibc_init; /* for patched Glibc */
#endif
  fflush_t		libc_fflush; /* to call fflush() at the end */
  free_t		free;	     /* to override free() :EXPERIMENTAL*/
  mallopt_t		mallopt;     /* to call mapllopt() */
  char 			***libc_argvp; /* to set __libc_argv */
  int			*libc_argcp;   /* to set __libc_argc */

  /* to implement pip_exit(), the context to be used by setcontext() */
  ucontext_t 		*ctx;

  char			***envvp; /* environment vars */
  volatile void		*export;  /* PiP export region */
} pip_task_t;

typedef struct {
  char			magic[PIP_MAGIC_LEN];
  unsigned int		version;
  size_t		size;
  pthread_t		thread;
  pip_spinlock_t	spawn_lock;
  volatile void		*export;
  pip_clone_t	 	*cloneinfo;
  char			*pip_root_env;
  free_t		free;
  int			opts;
  int			pid;
  int			pipid_curr;
  int			ntasks_curr;
  int			ntasks_accum;
  int			ntasks;
  pip_task_t		tasks[];
} pip_root_t;

extern pip_task_t 	*pip_task;

#endif

#endif
