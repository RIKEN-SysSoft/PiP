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

#define _GNU_SOURCE

//#define PIP_NO_MALLOPT
//#define PIP_USE_STATIC_CTX  /* this is slower, adds 30ns */
//#define PRINT_MAPS
//#define PRINT_FDS

/* the EVAL define symbol is to measure the time for calling dlmopen() */
//#define EVAL

//#define DEBUG

#include <sys/prctl.h>
#include <time.h>
#include <malloc.h> 		/* M_MMAP_THRESHOLD and M_TRIM_THRESHOLD  */

#include <pip_dlfcn.h>
#include <pip_internal.h>
#include <pip_blt.h>
#include <pip_gdbif.h>

#define CHECK_TLS

#ifdef CHECK_TLS
__thread int pipid_tls;
#endif


int pip_count_vec_( char **vecsrc ) {
  int n = 0;
  if( vecsrc != NULL ) {
    for( ; vecsrc[n]!= NULL; n++ );
  }
  return( n );
}

static char **pip_copy_vec3( char *addition0,
			     char *addition1,
			     char *addition2,
			     char **vecsrc ) {
  char 		**vecdst, *p;
  size_t	vecln, veccc, sz;
  int 		i, j;

  vecln = 0;
  veccc = 0;
  if( addition0 != NULL ) {
    vecln ++;
    veccc += strlen( addition0 ) + 1;
  }
  if( addition1 != NULL ) {
    vecln ++;
    veccc += strlen( addition1 ) + 1;
  }
  if( addition2 != NULL ) {
    vecln ++;
    veccc += strlen( addition2 ) + 1;
  }
  for( i=0; vecsrc[i]!=NULL; i++ ) {
    vecln ++;
    veccc += strlen( vecsrc[i] ) + 1;
  }
  vecln ++;		/* plus final NULL */

  sz = ( sizeof(char*) * vecln ) + veccc;
  if( ( vecdst = (char**) PIP_MALLOC( sz ) ) == NULL ) return NULL;
  p = ((char*)vecdst) + ( sizeof(char*) * vecln );
  i = j = 0;
  if( addition0 != NULL ) {
    vecdst[j++] = p;
    p = stpcpy( p, addition0 ) + 1;
  }
  if( addition1 != NULL ) {
    vecdst[j++] = p;
    p = stpcpy( p, addition1 ) + 1;
  }
  if( addition2 != NULL ) {
    vecdst[j++] = p;
    p = stpcpy( p, addition2 ) + 1;
  }
  for( i=0; vecsrc[i]!=NULL; i++ ) {
    vecdst[j++] = p;
    p = stpcpy( p, vecsrc[i] ) + 1;
  }
  vecdst[j] = NULL;

  return( vecdst );
}

static char **pip_copy_vec( char **vecsrc ) {
  return pip_copy_vec3( NULL, NULL, NULL, vecsrc );
}

static char **pip_copy_env( char **envsrc, int pipid ) {
  char rootenv[128], taskenv[128];
  char *preload_env = getenv( "LD_PRELOAD" );
  ASSERT( sprintf( rootenv, "%s=%p", PIP_ROOT_ENV, pip_root_ ) <= 0 );
  ASSERT( sprintf( taskenv, "%s=%d", PIP_TASK_ENV, pipid     ) <= 0 );
  return pip_copy_vec3( rootenv, taskenv, preload_env, envsrc );
}

size_t pip_stack_size( void ) {
  char 		*env, *endptr;
  size_t 	sz, scale;
  int 		i;

  if( ( sz = pip_root_->stack_size_blt ) == 0 ) {
    if( ( env = getenv( PIP_ENV_STACKSZ ) ) == NULL &&
	( env = getenv( "KMP_STACKSIZE" ) ) == NULL &&
	( env = getenv( "OMP_STACKSIZE" ) ) == NULL ) {
      sz = PIP_STACK_SIZE;	/* default */
    } else {
      if( ( sz = (size_t) strtol( env, &endptr, 10 ) ) <= 0 ) {
	pip_warn_mesg( "stacksize: '%s' is illegal and "
		       "default size (%d KiB) is set",
		       env,
		       PIP_STACK_SIZE / 1024 );
	sz = PIP_STACK_SIZE;	/* default */
      } else {
	scale = 1;
	switch( *endptr ) {
	case 'T': case 't':
	  scale *= 1024;
	case 'G': case 'g':
	  scale *= 1024;
	case 'M': case 'm':
	  scale *= 1024;
	case 'K': case 'k':
	case '\0':		/* default is KiB */
	  scale *= 1024;
	  sz *= scale;
	  break;
	case 'B': case 'b':
	  for( i=PIP_STACK_SIZE_MIN; i<sz; i*=2 );
	  sz = i;
	  break;
	default:
	  pip_warn_mesg( "stacksize: '%s' is illegal and default is used",
			 env );
	  sz = PIP_STACK_SIZE;
	  break;
	}
	sz = ( sz < PIP_STACK_SIZE_MIN ) ? PIP_STACK_SIZE_MIN : sz;
      }
    }
    pip_root_->stack_size_blt = sz;
  }
  return sz;
}

int pip_isa_coefd_( int fd ) {
  int flags = fcntl( fd, F_GETFD );
  return( ( flags > 0 ) && ( flags & FD_CLOEXEC ) );
}

#define PROCFD_PATH		"/proc/self/fd"
static int pip_list_coe_fds( int *fd_listp[] ) {
  DIR *dir;
  struct dirent *direntp;
  int i, fd, err = 0;

  ENTER;
  if( ( dir = opendir( PROCFD_PATH ) ) != NULL ) {
    int fd_dir = dirfd( dir );
    int nfds = 0;

    while( ( direntp = readdir( dir ) ) != NULL ) {
      if( direntp->d_name[0] != '.' &&
	  ( fd = atoi( direntp->d_name ) ) >= 0 &&
	  fd != fd_dir &&
	  pip_isa_coefd_( fd ) ) {
	nfds ++;
      }
    }
    if( nfds > 0 ) {
      ASSERT( ( *fd_listp = (int*) malloc( sizeof(int) * ( nfds + 1 ) ) ) == NULL );
      rewinddir( dir );
      i = 0;
      while( ( direntp = readdir( dir ) ) != NULL ) {
	errno = 0;
	if( direntp->d_name[0] != '.'             		&&
	    ( fd = strtol( direntp->d_name, NULL, 10 ) ) >= 0	&&
	    errno == 0						&&
	    fd != fd_dir                         		&&
	    pip_isa_coefd_( fd ) ) {
	  (*fd_listp)[i++] = fd;
	}
      }
      (*fd_listp)[i] = -1;
    }
    (void) closedir( dir );
    (void) close( fd_dir );
  }
  RETURN( err );
}

static int pip_load_dso( void **handlep, const char *path ) {
  Lmid_t	lmid;
  int 		flags = RTLD_NOW | RTLD_LOCAL;
  /* RTLD_GLOBAL is NOT accepted and dlmopen() returns EINVAL */
  void 		*loaded;
  int		err;

  DBGF( "handle=%p", *handlep );
  if( *handlep == NULL ) {
    lmid = LM_ID_NEWLM;
  } else if( pip_dlinfo( *handlep, RTLD_DI_LMID, (void*) &lmid ) != 0 ) {
    DBGF( "dlinfo(%p): %s", handlep, dlerror() );
    RETURN( ENXIO );
  }
  loaded = pip_dlmopen( lmid, path, flags );
  if( loaded == NULL ) {
    if( ( err = pip_check_pie( path, 1 ) ) != 0 ) RETURN( err );
    pip_warn_mesg( "dlmopen(%s): %s", path, dlerror() );
    RETURN( ENOEXEC );
  } else {
    DBGF( "dlmopen(%s): SUCCEEDED", path );
    *handlep = loaded;
  }
  RETURN( 0 );
}

static int pip_find_symbols( pip_spawn_program_t *progp,
			     void *handle,
			     pip_symbols_t *symp ) {
  int err = 0;

  /* functions */
  symp->main             = pip_dlsym( handle, "main"                  );
  if( progp->funcname != NULL ) {
    symp->start          = pip_dlsym( handle, progp->funcname         );
  }
  symp->ctype_init       = pip_dlsym( handle, "__ctype_init"          );
  symp->libc_fflush      = pip_dlsym( handle, "fflush"                );
  symp->mallopt          = pip_dlsym( handle, "mallopt"               );
  /* pip_named_export_fin symbol may not be found when the task
     program is not linked with the PiP lib. (due to not calling
     any PiP functions) */
  symp->named_export_fin = pip_dlsym( handle, "pip_named_export_fin_" );
  /* glibc workaround */
  symp->pip_set_tid      = pip_dlsym( handle, "pip_set_pthread_tid"   );
  //symp->pip_set_tid = NULL;
  /* variables */
  symp->environ          = pip_dlsym( handle, "environ"               );
  symp->libc_argvp       = pip_dlsym( handle, "__libc_argv"           );
  symp->libc_argcp       = pip_dlsym( handle, "__libc_argc"           );
  symp->prog             = pip_dlsym( handle, "__progname"            );
  symp->prog_full        = pip_dlsym( handle, "__progname_full"       );

  /* check mandatory symbols */
  DBGF( "env:%p  func(%s):%p   main:%p",
	symp->environ, progp->funcname, symp->start, symp->main );
  if( symp->environ == NULL) {
    err = ENOEXEC;
  } else {
    if( progp->funcname == NULL ) {
      if( symp->main == NULL ) {
	pip_warn_mesg( "Unable to find main "
		       "(possibly, not linked with '-rdynamic' option)" );
	err = ENOEXEC;
      }
    } else if( symp->start == NULL ) {
      pip_warn_mesg( "Unable to find start function (%s)",
		     progp->funcname );
      err = ENOEXEC;
    }
  }
  RETURN( err );
}

static int pip_load_prog( pip_spawn_program_t *progp,
			  pip_spawn_args_t *args,
			  pip_task_internal_t *taski ) {
  void	*loaded = NULL;
  int 	err;

  DBGF( "prog=%s", progp->prog );

  PIP_ACCUM( time_load_dso,
	     ( err = pip_load_dso( &loaded, progp->prog ) ) == 0 );
  if( err == 0 ) {
    pip_symbols_t *symp = &taski->annex->symbols;
    err = pip_find_symbols( progp, loaded, symp );
    if( err != 0 ) {
      (void) pip_dlclose( loaded );
    } else {
      taski->annex->loaded = loaded;
      pip_gdbif_load_( taski );
    }
  }
  RETURN( err );
}

int pip_do_corebind( pid_t tid, int coreno, cpu_set_t *oldsetp ) {
  int err = 0;

  ENTER;
  if( coreno != PIP_CPUCORE_ASIS ) {
    if( tid == 0 ) tid = pip_gettid();
    if( coreno < 0 || coreno > CPU_SETSIZE ) {
      err = EINVAL;
    } else {
      cpu_set_t cpuset;

      CPU_ZERO( &cpuset );
      CPU_SET( coreno, &cpuset );
      /* here, do not call pthread_setaffinity(). This MAY fail because    */
      /* pd->tid is NOT set yet.  I do not know why.  But it is OK because */
      /* pthread_setaffinity() calls sched_setaffinity() with tid.         */
      if( oldsetp != NULL ) {
	if( sched_getaffinity( tid, sizeof(cpuset), oldsetp ) != 0 ) {
	  err = errno;
	}
      }
      if( err == 0 ) {
	if( sched_setaffinity( tid, sizeof(cpuset), &cpuset ) != 0 ) {
	  err = errno;
	}
      }
    }
  }
  RETURN( err );
}

static int pip_undo_corebind( pid_t tid, int coreno, cpu_set_t *oldsetp ) {
  int err = 0;

  ENTER;
  if( coreno != PIP_CPUCORE_ASIS ) {
    if( tid == 0 ) tid = pip_gettid();
    /* here, do not call pthread_setaffinity().  See above comment. */
    if( sched_setaffinity( tid, sizeof(cpu_set_t), oldsetp ) != 0 ) {
      err = errno;
    }
  }
  RETURN( err );
}

int pip_get_dso( int pipid, void **loaded ) {
  pip_task_internal_t *task;
  int err;

  if( ( err = pip_check_pipid_( &pipid ) ) != 0 ) RETURN( err );
  task = pip_get_task_( pipid );
  if( loaded != NULL ) *loaded = task->annex->loaded;
  RETURN( 0 );
}

static int pip_glibc_init( pip_symbols_t *symbols,
			   pip_spawn_args_t *args,
			   void *loaded ) {
  int argc = pip_count_vec_( args->argv );

  if( symbols->prog != NULL ) {
    *symbols->prog = args->prog;
  }
  if( symbols->prog_full != NULL ) {
    *symbols->prog_full = args->prog_full;
  }
  if( symbols->libc_argcp != NULL ) {
    DBGF( "&__libc_argc=%p", symbols->libc_argcp );
    *symbols->libc_argcp = argc;
  }
  if( symbols->libc_argvp != NULL ) {
    DBGF( "&__libc_argv=%p", symbols->libc_argvp );
    *symbols->libc_argvp = args->argv;
  }
  if( symbols->environ != NULL ) {
    *symbols->environ = args->envv;	/* setting environment vars */
  }

  /* heap is not safe to use */
#ifndef PIP_NO_MALLOPT
  if( symbols->mallopt != NULL ) {
#ifdef M_MMAP_THRESHOLD
    if( symbols->mallopt( M_MMAP_THRESHOLD, 1 ) == 1 ) {
      DBGF( "mallopt(M_MMAP_THRESHOLD): succeeded" );
    } else {
      DBGF( "mallopt(M_MMAP_THRESHOLD): failed !!!!!!" );
    }
#endif
  }
#endif
  if( symbols->glibc_init != NULL ) {
    DBGF( ">> glibc_init@%p()", symbols->glibc_init );
    symbols->glibc_init( argc, args->argv, args->envv );
    DBGF( "<< glibc_init@%p()", symbols->glibc_init );
  } else if( symbols->ctype_init != NULL ) {
    DBGF( ">> __ctype_init@%p()", symbols->ctype_init );
    symbols->ctype_init();
    DBGF( "<< __ctype_init@%p()", symbols->ctype_init );
  }
  PIP_CHECK_CTYPE;
  return( argc );
}

static void
return_from_start_func( pip_task_internal_t *taski, int extval ) {
  ENTER;
  if( taski->annex->hook_after != NULL ) {
    void *hook_arg = taski->annex->hook_arg;
    int err;
    /* the after hook is supposed to free the hook_arg being malloc()ed  */
    /* by root and this is the reason to call this from the root context */
    if( ( err = taski->annex->hook_after( hook_arg ) ) != 0 ) {
      pip_warn_mesg( "PIPID:%d after-hook returns %d", taski->pipid, err );
      if( extval == 0 ) extval = err;
    }
  }
  pip_do_exit( taski, extval );
  NEVER_REACH_HERE;
}

void pip_exit( int extval ) {
  DBGF( "extval:%d", extval );
  if( !pip_is_initialized() ) {
    exit( extval );
  } else if( pip_isa_root() ) {
    exit( extval );
  } else {
    return_from_start_func( pip_task_, extval );
  }
  NEVER_REACH_HERE;
}

static void pip_reset_signal_handler( int sig ) {
  if( !pip_is_threaded_() ) {
    struct sigaction	sigact;
    memset( &sigact, 0, sizeof( sigact ) );
    sigact.sa_sigaction = (void(*)(int,siginfo_t*,void*)) SIG_DFL;
    ASSERT( sigaction( sig, &sigact, NULL )   != 0 );
  } else {
    sigset_t sigmask;
    (void) sigemptyset( &sigmask );
    (void) sigaddset( &sigmask, sig );
    ASSERT( pthread_sigmask( SIG_BLOCK, &sigmask, NULL ) != 0 );
  }
}

static void
pip_jump_into( pip_spawn_args_t *args, pip_task_internal_t *self ) {
  char **argv     = args->argv;
  char **envv     = args->envv;
  void *hook_arg  = self->annex->hook_arg;
  void *start_arg = args->start_arg;
  int 	argc, i;
  int	err, extval;

  argc = pip_glibc_init( &self->annex->symbols,
			 args,
			 self->annex->loaded );

  if( self->annex->hook_before != NULL &&
      ( err = self->annex->hook_before( hook_arg ) ) != 0 ) {
    pip_warn_mesg( "[%s] before-hook returns %d", argv[0], err );
    extval = err;
  } else {
    DBGF( "fd_list:%p", args->fd_list );
    if( args->fd_list != NULL ) {
      for( i=0; args->fd_list[i]>=0; i++ ) { /* Close-on-exec FDs */
	DBGF( "COE: %d", args->fd_list[i] );
	(void) close( args->fd_list[i] );
      }
    }
    pip_gdbif_hook_before_( self );
    if( self->annex->symbols.start == NULL ) {
      /* calling hook function, if any */
      DBGF( "[%d] >> main@%p(%d,%s,%s,...)",
	    args->pipid, self->annex->symbols.main, argc, argv[0], argv[1] );
      extval = self->annex->symbols.main( argc, argv, envv );
      DBGF( "[%d] << main@%p(%d,%s,%s,...) = %d",
	    args->pipid, self->annex->symbols.main, argc, argv[0], argv[1],
	    extval );
    } else {
      DBGF( "[%d] >> %s:%p(%p)",
	    args->pipid, args->funcname,
	    self->annex->symbols.start, start_arg );
      extval = self->annex->symbols.start( start_arg );
      DBGF( "[%d] << %s:%p(%p) = %d",
	    args->pipid, args->funcname,
	    self->annex->symbols.start, start_arg, extval );
    }
  }
  if( (pid_t) self->task_sched->annex->tid != pip_gettid() ) {
    /* when a pip task call fork() and the forked */
    /* process returns from main, this may happen  */
    DBGF( "%d : %d", self->task_sched->annex->tid, pip_gettid() );
    exit( extval );
    NEVER_REACH_HERE;
  }
  return_from_start_func( self, extval );
  NEVER_REACH_HERE;
}

static void pip_sigquit_handler( int sig, 
				 void(*handler)(), 
				 struct sigaction *oldp ) {
  DBG;
  pthread_exit( NULL );
}

static void* pip_do_spawn( void *thargs )  {
  void pip_cb_start( void *tsk ) {
    pip_task_internal_t *taski = (pip_task_internal_t*) tsk;
    /* let root proc know the task is running (or enqueued) */
    taski->annex->tid    = pip_gettid();
    taski->annex->thread = pthread_self();
  }
  /* The context of this function is of the root task                */
  /* so the global var; pip_task (and pip_root) are of the root task */
  /* and do not call malloc() and free() in this contxt !!!!         */
  pip_spawn_args_t *args = (pip_spawn_args_t*) thargs;
  int	 		pipid  = args->pipid;
  int 			coreno = args->coreno;
  pip_task_internal_t 	*self  = &pip_root_->tasks[pipid];
  pip_task_queue_t	*queue = args->queue;
  int			err    = 0;

  ENTER;

  if( pip_is_threaded_() ) {
    void pip_set_signal_handler_( int sig, 
				  void(*)(), 
				  struct sigaction* );
    if( self->annex->symbols.pip_set_tid != NULL ) {
      int tid, old;
      tid = pip_gettid();
      self->annex->symbols.pip_set_tid( pthread_self(), tid , &old );
      DBGF( "TID:%d", tid );
    }
    pip_set_signal_handler_( SIGQUIT, pip_sigquit_handler, NULL );
  }

#ifdef PIP_SAVE_TLS
  pip_save_tls( &self->tls );
#endif
  pip_memory_barrier();
#ifdef CHECK_TLS
  DBGF( "TLS:0x%lx  pipid_tls@%p", (intptr_t)self->tls, &pipid_tls );
  pipid_tls = pipid;
#endif

  {
    char sym[3];
    sym[0] = ( pipid % 10 ) + '0';
    sym[1] = ':';
    sym[2] = '\0';
    pip_set_name_( sym, args->prog, args->funcname );
  }
  pip_atomic_fetch_and_add( &pip_root_->ntasks_count, 1 );
  DBGF( "nblks:%d ntasks:%d",
	(int) pip_root_->ntasks_blocking,
	(int) pip_root_->ntasks_count );

  if( !pip_is_threaded_() ) {
    pip_reset_signal_handler( SIGCHLD );
    pip_reset_signal_handler( SIGTERM );
    pip_reset_signal_handler( SIGHUP  );
    (void) setpgid( 0, (pid_t) pip_root_->task_root->annex->tid );
  }

  if( ( err = pip_do_corebind( 0, coreno, NULL ) ) != 0 ) {
    pip_warn_mesg( "failed to bound CPU core:%d (%d)", coreno, err );
    err = 0;
  }
  pip_ctx_t	ctx;
  volatile int	flag_jump = 0;	/* must be volatile */

  self->annex->ctx_exit = &ctx;
  pip_save_context( &ctx );
  if( !flag_jump ) {
    flag_jump = 1;
    if( self->annex->opts & PIP_TASK_PASSIVE ) { /* passive task */
      void pip_suspend_and_enqueue_generic_( pip_task_internal_t*,
					     pip_task_queue_t*,
					     int,
					     pip_enqueue_callback_t,
					     void* );
      /* suspend myself */
      PIP_SUSPEND( self );
      pip_suspend_and_enqueue_generic_( self,
					queue,
					1, /* lock flag */
					pip_cb_start,
					self );
      /* resumed */

    } else {			/* active task */
      PIP_RUN( self );
      if( queue != NULL ) {
	int pip_dequeue_and_resume_multiple_( pip_task_internal_t*,
					      pip_task_queue_t*,
					      pip_task_internal_t*,
					      int* );

	int n = PIP_TASK_ALL;
	err = pip_dequeue_and_resume_multiple_( self, queue, self, &n );
      }
      /* since there is no callback, the cb func is called explicitly */
      pip_cb_start( (void*) self );
    }
    if( err == 0 ) {
      pip_jump_into( args, self );
    } else {
      pip_do_exit( self, err );
    }
    NEVER_REACH_HERE;
  }
  /* long jump on ctx_exit to terminate itself */

  DBGF( "PIPID:%d  tid:%d (%d)",
	self->pipid, self->annex->tid, pip_gettid() );
  ASSERTD( (pid_t) self->annex->tid != pip_gettid() );
#ifdef CHECK_TLS
  DBGF( "TLS:0x%lx  pipid_tls:%d (%p)  pipid:%d",
	(intptr_t) self->tls, pipid_tls, &pipid_tls, self->pipid );
  ASSERT( pipid_tls != self->pipid );
#endif
  /* call fflush() in the target context to flush out std* messages */
  if( self->annex->symbols.libc_fflush != NULL ) {
    self->annex->symbols.libc_fflush( NULL );
  }
  if( self->annex->symbols.named_export_fin != NULL ) {
    self->annex->symbols.named_export_fin( self );
  }
  DBGF( "PIPID:%d -- FORCE EXIT", self->pipid );
  if( pip_is_threaded_() ) {	/* thread mode */
    self->annex->flag_sigchld = 1;
    pip_memory_barrier();
    (void) pip_raise_signal_( pip_root_->task_root, SIGCHLD );
    /* root might be terminated */
    return NULL;
    //pthread_exit( NULL );
  } else {			/* process mode */
    exit( WEXITSTATUS(self->annex->status) );
  }
  NEVER_REACH_HERE;
  return NULL;			/* dummy */
}

static int pip_find_a_free_task( int *pipidp ) {
  int pipid = *pipidp;
  int i, err = 0;

  if( pip_root_->ntasks_accum >= PIP_NTASKS_MAX ) RETURN( EOVERFLOW );
  if( pipid < PIP_PIPID_ANY || pipid >= pip_root_->ntasks ) {
    DBGF( "pipid=%d", pipid );
    RETURN( EINVAL );
  }

  pip_spin_lock( &pip_root_->lock_tasks );
  /*** begin lock region ***/
  do {
    DBGF( "pipid:%d  ntasks:%d  pipid_curr:%d",
	  pipid, pip_root_->ntasks, pip_root_->pipid_curr );
    if( pipid != PIP_PIPID_ANY ) {
      if( pip_root_->tasks[pipid].pipid != PIP_PIPID_NULL ) {
	err = EAGAIN;
	goto unlock;
      }
    } else {
      for( i=pip_root_->pipid_curr; i<pip_root_->ntasks; i++ ) {
	if( pip_root_->tasks[i].pipid == PIP_PIPID_NULL ) {
	  pipid = i;
	  goto found;
	}
      }
      for( i=0; i<=pip_root_->pipid_curr; i++ ) {
	if( pip_root_->tasks[i].pipid == PIP_PIPID_NULL ) {
	  pipid = i;
	  goto found;
	}
      }
      err = EAGAIN;
      goto unlock;
    }
  found:
    pip_root_->tasks[pipid].pipid = pipid;	/* mark it as occupied */
    pip_root_->pipid_curr = pipid + 1;
    *pipidp = pipid;

  } while( 0 );
 unlock:
  /*** end lock region ***/
  pip_spin_unlock( &pip_root_->lock_tasks );

  RETURN( err );
}

static int pip_do_task_spawn( pip_spawn_program_t *progp,
			      int pipid,
			      int coreno,
			      uint32_t opts,
			      pip_task_t **bltp,
			      pip_task_queue_t *queue,
			      pip_spawn_hook_t *hookp ) {
  cpu_set_t 		cpuset;
  pip_spawn_args_t	*args = NULL;
  pip_task_internal_t	*task = NULL;
  size_t		stack_size;
  int 			op, err = 0;

  ENTER;
  if( pip_root_       == NULL ) RETURN( EPERM  );
  if( !pip_isa_root()         ) RETURN( EPERM  );
  if( progp           == NULL ) RETURN( EINVAL );
  if( progp->prog     == NULL ) RETURN( EINVAL );
  /* starting from main */
  if( progp->funcname == NULL &&
      progp->argv     == NULL ) RETURN( EINVAL );
  /* starting from an arbitrary func */
  if( ( op = pip_check_sync_flag_( opts ) ) < 0 ) RETURN( EINVAL );
  opts = op;

  if( progp->funcname == NULL &&
      progp->prog     == NULL ) progp->prog = progp->argv[0];

  if( pipid == PIP_PIPID_MYSELF ) RETURN( EINVAL );
  if( pipid != PIP_PIPID_ANY ) {
    if( pipid < 0 || pipid > pip_root_->ntasks ) RETURN( EINVAL );
  }
  if( ( err = pip_find_a_free_task( &pipid ) ) != 0 ) ERRJ;
  task = &pip_root_->tasks[pipid];
  pip_reset_task_struct_( task );
  task->pipid       = pipid;	/* mark it as occupied */
  task->type        = PIP_TYPE_TASK;
  task->task_sched  = task;
  task->ntakecare   = 1;
  task->annex->opts = opts;
  if( hookp != NULL ) {
    task->annex->hook_before = hookp->before;
    task->annex->hook_after  = hookp->after;
    task->annex->hook_arg    = hookp->hookarg;
  }
  /* allocate stack for sleeping */
  stack_size = pip_stack_size();
  pip_page_alloc_( pip_root_->stack_size_sleep, &task->annex->sleep_stack );
  if( task->annex->sleep_stack == NULL ) ERRJ_ERR( ENOMEM );

  if( progp->envv == NULL ) progp->envv = environ;
  args = &task->annex->args;
  args->start_arg = progp->arg;
  args->pipid     = pipid;
  args->coreno    = coreno;
  args->queue     = queue;
  {
    char *p;
    if( ( p = strrchr( progp->prog, '/' ) ) == NULL) {
      args->prog = strdup( progp->prog );
    } else {
      args->prog = strdup( p + 1 );
    }
    if( args->prog == NULL ) ERRJ_ERR( ENOMEM );
  }
  if( ( args->prog_full = realpath( args->prog, NULL ) ) == NULL ) {
    args->prog_full = strdup( args->prog );
    if( args->prog_full == NULL ) ERRJ_ERR( ENOMEM );
  }
  if( ( args->envv = pip_copy_env( progp->envv, pipid ) ) == NULL ) {
    ERRJ_ERR( ENOMEM );
  }
  if( progp->funcname == NULL ) {
    if( ( args->argv = pip_copy_vec( progp->argv ) ) == NULL ) {
      ERRJ_ERR( ENOMEM );
    }
  } else {
    if( ( args->funcname = strdup( progp->funcname ) ) == NULL ) {
      ERRJ_ERR( ENOMEM );
    }
  }
  if( pip_is_shared_fd_() ) {
    args->fd_list = NULL;
  } else if( ( err = pip_list_coe_fds( &args->fd_list ) ) != 0 ) {
    ERRJ_ERR( ENOMEM );
  }
  /* must be called before calling dlmopen() */
  pip_gdbif_task_new_( task );	

  pip_gdbif_task_new_( task );

  if( ( err = pip_do_corebind( 0, coreno, &cpuset ) ) == 0 ) {
    /* corebinding should take place before loading solibs,       */
    /* hoping anon maps would be mapped onto the closer numa node */
    PIP_ACCUM( time_load_prog,
	       ( err = pip_load_prog( progp, args, task ) ) == 0 );
    /* and of course, the corebinding must be undone */
    (void) pip_undo_corebind( 0, coreno, &cpuset );
  }
  ERRJ_CHK(err);

  if( ( pip_root_->opts & PIP_MODE_PROCESS_PIPCLONE ) ==
      PIP_MODE_PROCESS_PIPCLONE ) {
    int flags =
      CLONE_VM |
      CLONE_SETTLS |
      CLONE_PARENT_SETTID |
      CLONE_CHILD_CLEARTID |
      CLONE_SYSVSEM |
      CLONE_PTRACE |
      SIGCHLD;
    pid_t pid;

    err = pip_clone_mostly_pthread_ptr( (pthread_t*) &task->annex->thread,
					flags,
					coreno == PIP_CPUCORE_ASIS ? - 1 :
					coreno,
					stack_size,
					(void*(*)(void*)) pip_do_spawn,
					args,
					&pid );
    DBGF( "pip_clone_mostly_pthread_ptr()=%d", err );

  } else {
    pthread_t		thr;
    pthread_attr_t 	attr;
    pip_clone_t		*clone_info = pip_root_->cloneinfo;
    pid_t		tid = pip_gettid();

    if( ( err = pthread_attr_init( &attr ) ) == 0 ) {
      err = pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_JOINABLE );
      DBGF( "pthread_attr_setdetachstate(JOINABLE)= %d", err );
      if( err ) goto error;
      err = pthread_attr_setstacksize( &attr, stack_size );
      DBGF( "pthread_attr_setstacksize( %ld )= %d", stack_size, err );
      if( err ) goto error;

      if( clone_info != NULL ) { /* pip_preload */
	DBGF( "tid=%d  cloneinfo@%p", tid, clone_info );
	/* lock is needed, because the preloaded clone()
	   might also be called from outside of PiP lib. */
	pip_spin_lock_wv( &clone_info->lock, tid );
      }
      {
	err = pthread_create( &thr,
			      &attr,
			      (void*(*)(void*)) pip_do_spawn,
			      (void*) args );
	DBGF( "pthread_create()=%d", errno );
      }
      /* unlock is done in the wrapper function */
    }
  }
  if( err == 0 ) {
    struct timespec ts;
    int i;

    ts.tv_sec  = 0;
    ts.tv_nsec = 100 * 1000;	/* 0.1 msec */

    for( i=0; i<100; i++ ) {
      /* wait until task starts running or       */
      /* task is enqueued if to be suspended  */
      if( task->annex->tid    >  0 &&
	  task->annex->thread != 0 ) goto done;
      nanosleep( &ts, NULL );
      pip_system_yield();
    }
    pip_err_mesg( "Spawning PiP task (PIPID:%d) does not respond "
		  "(timeout)",
		  task->pipid );
    err = ETIMEDOUT;
    goto error;

  done:
    DBGF( "task (PIPID:%d,TID:%d) is created and running", 
	  task->pipid, task->annex->tid );
    pip_root_->ntasks_accum ++;
    pip_gdbif_task_commit_( task );
    if( bltp != NULL ) *bltp = (pip_task_t*) task;

  } else {
  error:			/* undo */
    if( args != NULL ) {
      PIP_FREE( args->prog );
      PIP_FREE( args->prog_full );
      PIP_FREE( args->funcname );
      PIP_FREE( args->argv );
      PIP_FREE( args->envv );
      PIP_FREE( args->fd_list );
    }
    if( task != NULL ) {
      if( task->annex->loaded != NULL ) pip_dlclose( task->annex->loaded );
      pip_reset_task_struct_( task );
    }
  }
  DBGF( "pip_task_spawn_(pipid=%d)", pipid );
  RETURN( err );
}

/*
 * The following functions must be called at root process
 */

int pip_task_spawn( pip_spawn_program_t *progp,
		    int coreno,
		    uint32_t opts,
		    int *pipidp,
		    pip_spawn_hook_t *hookp ) {
  pip_task_t	*task;
  int 		pipid;
  int 		err;

  ENTER;
  if( pipidp == NULL ) {
    pipid = PIP_PIPID_ANY;
  } else {
    pipid = *pipidp;
  }
  err = pip_do_task_spawn( progp, pipid, coreno, opts, &task, NULL, hookp );
  if( !err ) {
    if( pipidp != NULL ) *pipidp = PIP_TASKI(task)->pipid;
  }
  RETURN( err );
}

int pip_blt_spawn_( pip_spawn_program_t *progp,
		    int coreno,
		    uint32_t opts,
		    int *pipidp,
		    pip_task_t **bltp,
		    pip_task_queue_t *queue,
		    pip_spawn_hook_t *hookp ) {
  pip_task_t 	*blt;
  int 		pipid, err;

  ENTER;
  if( pipidp == NULL ) {
    pipid = PIP_PIPID_ANY;
  } else {
    pipid = *pipidp;
  }
  err = pip_do_task_spawn( progp, pipid, coreno, opts, &blt, queue, hookp );
  if( !err ) {
    if( pipidp != NULL ) *pipidp = PIP_TASKI(blt)->pipid;
    if( bltp   != NULL ) *bltp   = blt;
  }
  RETURN( err );
}

int pip_spawn( char *prog,
	       char **argv,
	       char **envv,
	       int  coreno,
	       int  *pipidp,
	       pip_spawnhook_t before,
	       pip_spawnhook_t after,
	       void *hookarg ) {
  pip_spawn_program_t program;
  pip_spawn_hook_t hook;

  ENTER;
  if( prog == NULL ) return EINVAL;
  pip_spawn_from_main( &program, prog, argv, envv );
  pip_spawn_hook( &hook, before, after, hookarg );
  RETURN( pip_task_spawn( &program, coreno, 0, pipidp, &hook ) );
}

int pip_get_id( int pipid, intptr_t *idp ) {
  pip_task_internal_t *taski;
  int err;

  if( ( err = pip_check_pipid_( &pipid ) ) != 0 ) RETURN( err );
  if( idp == NULL ) RETURN( EINVAL );

  taski = pip_get_task_( pipid );
  if( pip_is_threaded_() ) {
    /* Do not use gettid(). This is a very Linux-specific function */
    /* The reason of supporintg the thread PiP execution mode is   */
    /* some OSes other than Linux does not support clone()         */
    *idp = (intptr_t) taski->task_sched->annex->thread;
  } else {
    *idp = (intptr_t) taski->annex->tid;
  }
  RETURN( 0 );
}
