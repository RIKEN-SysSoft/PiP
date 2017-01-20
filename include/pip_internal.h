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
#include <stdio.h>

#include <pip_machdep.h>
#include <pip_clone.h>
#include <pip_debug.h>

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
typedef void(*init_misc_t)(int,char**,char**);
typedef void(*getopt_clean_t)(char**);
typedef void(*libc_ctors_t)(void);
#else
typedef void(*glibc_init_t)(int,char**,char**);
#endif
typedef	void(*fflush_t)(FILE*);

typedef void(*free_t)(void*);

typedef struct {
  int			pipid;
  int			pid;
  int			coreno;
  int	 		retval;
  pthread_t		thread;
  void			*loaded;
  main_func_t		mainf;
#ifndef HAVE_GLIBC_INIT
  ctype_init_t		ctype_init;
  init_misc_t		init_misc;
  getopt_clean_t	getopt_clean;
  libc_ctors_t		libc_ctors;
#else
  glibc_init_t		glibc_init;
#endif
  fflush_t		libc_fflush;
  free_t		free;
  ucontext_t 		*ctx;
  char			**argv;
  char			**envv;
  char			***envvp;
  volatile void		*export;
} pip_task_t;

typedef struct {
  char			magic[PIP_MAGIC_LEN];
  int			version; /* for future us (backward compatibility) */
  size_t		size;
  pthread_t		thread;
  pip_spinlock_t	spawn_lock;
  volatile void		*export;
  pip_clone_t	 	*cloneinfo;
  free_t		free;
  int			opts;
  int			pid;
  int			pipid_curr;
  int			ntasks_curr;
  int			ntasks_accum;
  int			ntasks;
  pip_task_t		tasks[];
} pip_root_t;

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

extern pip_task_t 	*pip_task;

#endif
