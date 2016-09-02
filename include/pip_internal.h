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

#include <pthread.h>
#include <ucontext.h>
#include <stdlib.h>

#include <pip_machdep.h>
#include <pip_clone.h>
#include <pip_debug.h>

#define PIP_ROOT_ENV		"PIP_ROOT"

#define PIP_MAGIC_WORD		"PrcInPrc"
#define PIP_MAGIC_LEN		(8)

#define MAIN_FUNC		"main"

#define PIP_PIPID_NONE		(-999)

#define PIP_LD_SOLIBS		{ NULL }

typedef   int(*main_func_t)(int,char**);

typedef struct {
  int			pipid;
  int			pid;
  int	 		retval;
  pthread_t		thread;
  void			*loaded;
  main_func_t		mainf;
  ucontext_t 		ctx;
  char			**argv;
  char			**envv;
  char			***envvp;
  volatile void		*export;
} pip_task_t;

typedef struct {
  char			magic[PIP_MAGIC_LEN];
  pthread_t		thread;
  pip_spinlock_t	spawn_lock;
  volatile void		*export;
  pip_clone_t	 	*cloneinfo;
  int			opts;
  int			pid;
  int			ntasks_curr;
  int			ntasks_accum;
  int			ntasks;
  pip_task_t		tasks[];
} pip_root_t;

typedef struct pip_spawn_args {
  int 			pipid;
  int	 		coreno;
  char 			*prog;
  char 			**argv;
  char 			**envv;
} pip_spawn_args_t;

extern pip_task_t 	*pip_task;

#endif
