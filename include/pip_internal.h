/*
  * $RIKEN_copyright: Riken Center for Computational Sceience,
  * System Software Development Team, 2016, 2017, 2018, 2019$
  * $PIP_VERSION: Version 1.0.0$
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
typedef void(*pthread_init_t)(int,char**,char**);
typedef	void(*ctype_init_t)(void);
typedef void(*glibc_init_t)(int,char**,char**);
typedef void(*add_stack_user_t)(void);
typedef	void(*fflush_t)(FILE*);

typedef struct {
  /* functions */
  main_func_t		main;	     /* main function address */
  pthread_init_t	pthread_init; /* __pthread_init_minimum() */
  ctype_init_t		ctype_init;  /* to call __ctype_init() */
  glibc_init_t		glibc_init;  /* only in patched Glibc */
  add_stack_user_t	add_stack; /* for GDB workaroundn */
  fflush_t		libc_fflush; /* to call fflush() at the end */
  mallopt_t		mallopt;     /* to call mallopt() */
  free_t		free;	     /* to override free() - EXPERIMENTAL*/
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
  char			**envv;
} pip_spawn_args_t;

#define PIP_TYPE_NONE	(0)
#define PIP_TYPE_ROOT	(1)
#define PIP_TYPE_TASK	(2)
#define PIP_TYPE_ULP	(3)

struct pip_gdbif_task;

typedef struct pip_task {
  int			pipid;	 /* PiP ID */
  int			type;	 /* PIP_TYPE_TASK or PIP_TYPE_ULP */

  void			*export;
  ucontext_t		*ctx_exit;
  void			*loaded;
  pip_symbols_t		symbols;
  pip_spawn_args_t	args;	/* arguments for a PiP task */
  int			retval;

  struct pip_gdbif_task	*gdbif_task;

  int			boundary[0];
  union {
    struct {			/* for PiP tasks */
      pid_t		pid;	/* PID in process mode */
      pthread_t		thread;	/* thread */
      pip_spawnhook_t	hook_before;
      pip_spawnhook_t	hook_after;
      void		*hook_arg;
      pip_spinlock_t	lock_malloc; /* lock for pip_malloc and pip_free */
    };
    struct {			/* for PiP ULPs */
      struct pip_task	*task_parent;
      void		*stack;
      struct pip_ulp	*ulp;
    };
  };
} pip_task_t;

#define PIP_FILLER_SZ	(PIP_CACHE_SZ-sizeof(pip_spinlock_t))

typedef struct {
  char			magic[PIP_MAGIC_LEN];
  unsigned int		version;
  size_t		root_size;
  size_t		size;
  pip_spinlock_t	lock_ldlinux; /* lock for dl*() functions */
  union {
    struct {
      size_t		page_size;
      unsigned int	opts;
      unsigned int	actual_mode;
      int		ntasks;
      int		ntasks_curr;
      int		ntasks_accum;
      int		pipid_curr;
      pip_clone_t	*cloneinfo;   /* only valid with process:preload */
    };
    char		__filler0__[PIP_FILLER_SZ];
  };
  pip_spinlock_t	lock_stack_flist; /* ULP: lock for stack free list */
  union {
    struct {
      void		*stack_flist;	  /* ULP: stack free list */
      size_t		stack_size;
      pip_task_t	*task_root; /* points to tasks[ntasks] */
    };
    char		__filler1__[PIP_FILLER_SZ];
  };
  pip_spinlock_t	lock_tasks; /* lock for finding a new task id */
  pip_task_t		tasks[];
} pip_root_t;


#ifdef __cplusplus
extern "C" {
#endif
  /* the following functions deeply depends on PiP execution mode */
  int    pip_get_thread( int pipid, pthread_t *threadp );
  int    pip_is_pthread( int *flagp );
  int    pip_is_shared_fd( int *flagp );
  int    pip_is_shared_sighand( int *flagp );
#ifdef __cplusplus
}
#endif

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#endif /* PIP_INTERNAL_FUNCS */

#endif /* _pip_internal_h_ */
