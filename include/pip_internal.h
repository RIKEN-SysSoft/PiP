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

#define PIP_LD_SOLIBS		{ NULL }

#ifdef AHAHAHAH
#define LIBCDIR			"/usr/lib64/"
#endif
#define LIBCDIR			"/home/ahori/work/PIP/GLIBC/install/lib/"

#define PIP_STD_NSSLIBS		{					\
      LIBCDIR "libnss_dns.so", LIBCDIR "libnss_files.so",		\
      LIBCDIR "libnss_nis.so", LIBCDIR "libnss_nisplus.so",		\
      LIBCDIR "libnss_db.so",  LIBCDIR "libnss_compat.so",		\
      LIBCDIR "libnss_hesiod.so",					\
      NULL }

typedef	int(*main_func_t)(int,char**);

typedef	void(*ctype_init_t)(void);
typedef	void(*fflush_t)(FILE*);

typedef struct {
  int			pipid;
  int			pid;
  int	 		retval;
  pthread_t		thread;
  void			*loaded;
  main_func_t		mainf;
  ctype_init_t		ctype_initf;
  fflush_t		libc_fflush;
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
  pip_spawnhook_t	hook_before;
  pip_spawnhook_t	hook_after;
  void			*hook_arg;
  char 			*prog;
  char 			**argv;
  char 			**envv;
} pip_spawn_args_t;

extern pip_task_t 	*pip_task;

#endif
