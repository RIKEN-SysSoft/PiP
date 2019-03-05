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
 * Written by Atsushi HORI <ahori@riken.jp>, 2016, 2017, 2018, 2019
 */

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/wait.h>
#include <sched.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>

//#define PIP_NO_MALLOPT
//#define PIP_USE_STATIC_CTX  /* this is slower, adds 30ns */
//#define PRINT_MAPS
//#define PRINT_FDS

/* the EVAL define symbol is to measure the time for calling dlmopen() */
//#define EVAL

//#define DEBUG

#include <pip_internal.h>
#include <pip_gdbif_func.h>

extern void pip_named_export_fin_(  pip_task_internal_t* );
extern void pip_named_export_fin_all_( void );

extern void pip_reset_task_struct_( pip_task_internal_t* );

int pip_is_threaded_( void ) {
  return ( pip_root_->opts & PIP_MODE_PTHREAD ) != 0 ? CLONE_THREAD : 0;
}

static int pip_is_shared_fd_( void ) {
  if( pip_root_->cloneinfo == NULL )
    return (pip_root_->opts & PIP_MODE_PTHREAD) != 0 ? CLONE_FILES : 0;
  return pip_root_->cloneinfo->flag_clone & CLONE_FILES;
}

int pip_raise_signal_( pip_task_internal_t *taski, int sig ) {
  int err = 0;

  ENTER;
  //if( PIP_IS_PASSIVE( taski ) ) RETURN( EPERM );
  if( pip_is_threaded_() ) {
    if( pthread_kill( taski->annex->thread, sig ) == ESRCH ) {
      DBGF( "[%d] task->thread:%p", taski->pipid,
	    (void*) taski->annex->thread );
      if( kill( taski->annex->pid, sig ) != 0 ) err = errno;
    }
  } else if( taski->annex->pid > 0 ) {
    if( ( kill( taski->annex->pid, sig ) ) != 0 ) err = errno;
  }
  RETURN( err );
}

int pip_kill( int pipid, int signal ) {
  int err;

  if( signal < 0 ) RETURN( EINVAL );
  if( ( err = pip_check_pipid_( &pipid ) ) == 0 ) {
    err = pip_raise_signal_( pip_get_task_( pipid ), signal );
  }
  RETURN( err );
}

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
  if( ( vecdst = (char**) malloc( sz ) ) == NULL ) return NULL;
  p = ((char*)vecdst) + ( sizeof(char*) * vecln );
  i = j = 0;
  if( addition0 ) {
    vecdst[j++] = p;
    p = stpcpy( p, addition0 ) + 1;
  }
  ASSERT( ( (intptr_t)p >= (intptr_t)(vecdst+sz) ) );
  if( addition1 ) {
    vecdst[j++] = p;
    p = stpcpy( p, addition1 ) + 1;
  }
  ASSERT( ( (intptr_t)p >= (intptr_t)(vecdst+sz) ) );
  for( i=0; vecsrc[i]!=NULL; i++ ) {
    vecdst[j++] = p;
    p = stpcpy( p, vecsrc[i] ) + 1;
    ASSERT( ( (intptr_t)p >= (intptr_t)(vecdst+sz) ) );
  }
  vecdst[j] = NULL;

  if( 0 ) {
    int ii;
    for( ii=0; vecsrc[ii]!=NULL; ii++ ) {
      fprintf( stderr, "<<SRC>> vec[%d] %s\n", ii, vecsrc[ii] );
    }
    for( ii=0; vecdst[ii]!=NULL; ii++ ) {
      fprintf( stderr, "<<DST>> vec[%d] %s\n", ii, vecdst[ii] );
    }
  }
  return( vecdst );
}

static char **pip_copy_vec( char **vecsrc ) {
  return pip_copy_vec3( NULL, NULL, NULL, vecsrc );
}

static char **pip_copy_env( char **envsrc, int pipid ) {
  char rootenv[128];
  char taskenv[128];
  char *preload_env = getenv( "LD_PRELOAD" );

  if( sprintf( rootenv, "%s=%p", PIP_ROOT_ENV, pip_root_ ) <= 0 ||
      sprintf( taskenv, "%s=%d", PIP_TASK_ENV, pipid    ) <= 0 ) {
    return NULL;
  }
  return pip_copy_vec3( rootenv, taskenv, preload_env, envsrc );
}

size_t pip_stack_size( void ) {
  char 		*env, *endptr;
  size_t 	sz, scale;
  int 		i;

  if( ( sz = pip_root_->stack_size ) == 0 ) {
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
    pip_root_->stack_size = sz;
  }
  return sz;
}

int pip_is_coefd_( int fd ) {
  int flags = fcntl( fd, F_GETFD );
  return( flags > 0 && FD_CLOEXEC );
}

static void pip_close_on_exec( void ) {
  DIR *dir;
  struct dirent *direntp;
  int fd;

#ifdef PRINT_FDS
  pip_print_fds();
#endif

#define PROCFD_PATH		"/proc/self/fd"
  if( ( dir = opendir( PROCFD_PATH ) ) != NULL ) {
    int fd_dir = dirfd( dir );
    while( ( direntp = readdir( dir ) ) != NULL ) {
      if( ( fd = atoi( direntp->d_name ) ) >= 0 &&
	  fd != fd_dir && pip_is_coefd_( fd ) ) {
#ifdef DEBUG
	pip_print_fd( fd );
#endif
	(void) close( fd );
	DBGF( "<PID=%d> fd[%d] is closed (CLOEXEC)", getpid(), fd );
      }
    }
    (void) closedir( dir );
    (void) close( fd_dir );
  }
#ifdef PRINT_FDS
  pip_print_fds();
#endif
}

static int pip_load_dso( void **handlep, char *path ) {
  Lmid_t	lmid;
  int 		flags = RTLD_NOW | RTLD_LOCAL;
  /* RTLD_GLOBAL is NOT accepted and dlmopen() returns EINVAL */
  void 		*loaded;
  int		err;

  DBGF( "handle=%p", *handlep );
  if( *handlep == NULL ) {
    lmid = LM_ID_NEWLM;
  } else if( dlinfo( *handlep, RTLD_DI_LMID, (void*) &lmid ) != 0 ) {
    DBGF( "dlinfo(%p): %s", handlep, dlerror() );
    RETURN( ENXIO );
  }
  DBGF( "calling dlmopen(%s)", path );
  PIP_ACCUM( time_dlmopen, ( loaded = dlmopen( lmid, path, flags ) ) == NULL );
  DBG;
  if( loaded == NULL ) {
    if( ( err = pip_check_pie( path ) ) != 0 ) RETURN( err );
    pip_warn_mesg( "dlmopen(%s): %s", path, dlerror() );
    RETURN( ENOEXEC );
  } else {
    DBGF( "dlmopen(%s): SUCCEEDED", path );
    *handlep = loaded;
  }
  RETURN( 0 );
}

void pip_dlclose_( void *handle ) {
#ifdef AH
  pip_spin_lock( &pip_root_->lock_ldlinux );
  do {
    dlclose( handle );
  } while( 0 );
  pip_spin_unlock( &pip_root_->lock_ldlinux );
#endif
}

static int pip_find_symbols( pip_spawn_program_t *progp,
			     void *handle,
			     pip_symbols_t *symp ) {
  int err = 0;

  /* functions */
  symp->main             = dlsym( handle, "main"                         );
  if( progp->funcname != NULL ) {
    symp->start          = dlsym( handle, progp->funcname                );
  }
#ifdef PIP_PTHREAD_INIT
  symp->pthread_init     = dlsym( handle, "__pthread_initialize_minimal" );
#endif
  symp->ctype_init       = dlsym( handle, "__ctype_init"                 );
  symp->libc_fflush      = dlsym( handle, "fflush"                       );
  symp->mallopt          = dlsym( handle, "mallopt"                      );
  symp->named_export_fin = dlsym( handle, "pip_named_export_fin_"        );
  /* pip_named_export_fin symbol may not be found when the task
     program is not linked with the PiP lib. (due to not calling
     any PiP functions) */
  /* unused */
  symp->free             = dlsym( handle, "free"                         );
  symp->glibc_init       = dlsym( handle, "glibc_init"                   );
  /* variables */
  symp->environ          = dlsym( handle, "environ"                      );
  symp->libc_argvp       = dlsym( handle, "__libc_argv"                  );
  symp->libc_argcp       = dlsym( handle, "__libc_argc"                  );
  symp->progname         = dlsym( handle, "__progname"                   );
  symp->progname_full    = dlsym( handle, "__progname_full"              );

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
			  pip_task_internal_t *taski ) {
  void	*loaded = NULL;
  int 	err;

  DBGF( "prog=%s", progp->prog );

#ifdef PRINT_MAPS
  pip_print_maps();
#endif
  PIP_ACCUM( time_load_dso, ( err = pip_load_dso( &loaded, progp->prog ))==0);
#ifdef PRINT_MAPS
  pip_print_maps();
#endif
  DBG;
  if( err == 0 ) {
    err = pip_find_symbols( progp, loaded, &taski->annex->symbols );
    if( err != 0 ) {
      DBG;
      (void) pip_dlclose_( loaded );
    } else {
      DBG;
      taski->annex->loaded = loaded;
      pip_gdbif_load_( taski );
    }
  }
  RETURN( err );
}

static int pip_do_corebind( int coreno, cpu_set_t *oldsetp ) {
  int err = 0;

  ENTER;
  if( coreno != PIP_CPUCORE_ASIS ) {
    cpu_set_t cpuset;

    CPU_ZERO( &cpuset );
    CPU_SET( coreno, &cpuset );

    if( pip_is_threaded_() ) {
      err = pthread_getaffinity_np( pthread_self(),
				    sizeof(cpu_set_t),
				    oldsetp );
      if( err == 0 ) {
	err = pthread_setaffinity_np( pthread_self(),
				      sizeof(cpu_set_t),
				      &cpuset );
      }
    } else {
      if( sched_getaffinity( 0, sizeof(cpuset), oldsetp ) != 0 ||
	  sched_setaffinity( 0, sizeof(cpuset), &cpuset ) != 0 ) {
	err = errno;
      }
    }
  }
  RETURN( err );
}

static int pip_undo_corebind( int coreno, cpu_set_t *oldsetp ) {
  int err = 0;

  ENTER;
  if( coreno != PIP_CPUCORE_ASIS ) {
    if( pip_is_threaded_() ) {
      err = pthread_setaffinity_np( pthread_self(),
				    sizeof(cpu_set_t),
				    oldsetp );
    } else {
      if( sched_setaffinity( 0, sizeof(cpu_set_t), oldsetp ) != 0 ) {
	err = errno;
      }
    }
  }
  RETURN( err );
}

static int pip_corebind( int coreno ) {
  cpu_set_t cpuset;

  ENTER;
  if( coreno != PIP_CPUCORE_ASIS &&
      coreno >= 0                &&
      coreno <  sizeof(cpuset) * 8 ) {
    DBG;
    CPU_ZERO( &cpuset );
    CPU_SET( coreno, &cpuset );
    if( sched_setaffinity( 0, sizeof(cpuset), &cpuset ) != 0 ) RETURN( errno );
  } else {
    DBGF( "coreno=%d", coreno );
  }
  RETURN( 0 );
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
			   char *prog,
			   char **argv,
			   char **envv,
			   void *loaded ) {
  int argc = pip_count_vec_( argv );

  if( symbols->progname != NULL ) {
    char *p;
    if( ( p = strrchr( prog, '/' ) ) == NULL) {
      *symbols->progname = prog;
    } else {
      *symbols->progname = p + 1;
    }
  }
  if( symbols->progname_full != NULL ) {
    if( ( *symbols->progname_full = realpath( prog, NULL ) ) == NULL ) {
      *symbols->progname_full = prog;
    }
  }
  if( symbols->libc_argcp != NULL ) {
    DBGF( "&__libc_argc=%p", symbols->libc_argcp );
    *symbols->libc_argcp = argc;
  }
  if( symbols->libc_argvp != NULL ) {
    DBGF( "&__libc_argv=%p", symbols->libc_argvp );
    *symbols->libc_argvp = argv;
  }
  if( symbols->environ != NULL ) {
    *symbols->environ = envv;	/* setting environment vars */
  }

#ifdef PIP_PTHREAD_INIT
  if( symbols->pthread_init != NULL ) {
    symbols->pthread_init( argc, argv, envv );
  }
#endif

#ifndef PIP_NO_MALLOPT
  if( symbols->mallopt != NULL ) {
    DBGF( ">> mallopt()" );
#ifdef M_MMAP_THRESHOLD
    if( symbols->mallopt( M_MMAP_THRESHOLD, 1 ) == 1 ) {
      DBGF( "<< mallopt(M_MMAP_THRESHOLD): succeeded" );
    } else {
      DBGF( "<< mallopt(M_MMAP_THRESHOLD): failed !!!!!!" );
    }
#endif
#ifdef M_TRIM_THRESHOLD
    if( symbols->mallopt( M_TRIM_THRESHOLD, -1 ) == 1 ) {
      DBGF( "<< mallopt(M_TRIM_THRESHOLD): succeeded" );
    } else {
      DBGF( "<< mallopt(M_TRIM_THRESHOLD): failed !!!!!!" );
    }
#endif
  }
#endif
  if( symbols->glibc_init != NULL ) {
    DBGF( ">> glibc_init@%p()", symbols->glibc_init );
    symbols->glibc_init( argc, argv, envv );
    DBGF( "<< glibc_init@%p()", symbols->glibc_init );
  } else if( symbols->ctype_init != NULL ) {
    DBGF( ">> __ctype_init@%p()", symbols->ctype_init );
    symbols->ctype_init();
    DBGF( "<< __ctype_init@%p()", symbols->ctype_init );
  }
  DBG;
  PIP_CHECK_CTYPE;
  return( argc );
}

static int pip_jump_into( pip_spawn_args_t *args, pip_task_internal_t *self ) {
  char  	*prog      = args->prog;
  char 		**argv     = args->argv;
  char 		**envv     = args->envv;
  void		*start_arg = args->start_arg;
  int 		argc;
  int		extval;

  argc = pip_glibc_init( &self->annex->symbols,
			 prog,
			 argv,
			 envv,
			 self->annex->loaded );
  if( self->annex->symbols.start == NULL ) {
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
  return extval;
}

static void* pip_do_spawn( void *thargs )  {
  /* The context of this function is of the root task                */
  /* so the global var; pip_task (and pip_root) are of the root task */
  extern void pip_do_exit( pip_task_internal_t*, int );
  pip_spawn_args_t *args = (pip_spawn_args_t*) thargs;
  int	 		pipid  = args->pipid;
  char 			**argv = args->argv;
  int 			coreno = args->coreno;
  pip_task_internal_t 	*self  = &pip_root_->tasks[pipid];
  int			extval = 0;
  int			err    = 0;

  ENTER;

  //PIP_ACTIVATE( self );

#ifdef PIP_USE_STATIC_CTX
  pip_ctx_t	context;
  memset( &context, 0, sizeof(context) );
  self->ctx_static = &context;
#endif
  if( ( err = pip_corebind( coreno ) ) != 0 ) {
    pip_warn_mesg( "failed to bund CPU core (%d)", err );
  }
  self->annex->thread = pthread_self();
#ifdef PIP_SAVE_TLS
  pip_save_tls( &self->tls );
#endif

  if( pip_is_threaded_() ) {
    sigset_t sigmask;
    (void) sigemptyset( &sigmask );
    (void) sigaddset( &sigmask, SIGCHLD );
    (void) pthread_sigmask( SIG_BLOCK, &sigmask, NULL );
  }
#ifdef DEBUG
  if( pip_is_threaded_() ) {
    pthread_attr_t attr;
    size_t sz;
    int _err;
    if( ( _err = pthread_getattr_np( self->annex->thread, &attr ) ) != 0 ) {
      DBGF( "pthread_getattr_np()=%d", _err );
    } else if( ( _err = pthread_attr_getstacksize( &attr, &sz ) ) != 0 ) {
      DBGF( "pthread_attr_getstacksize()=%d", _err );
    } else {
      DBGF( "stacksize = %ld [KiB]", sz/1024 );
    }
  }
#endif
  if( !pip_is_shared_fd_() ) pip_close_on_exec();
  /* calling hook, if any */
  void	*hook_arg = self->annex->hook_arg;

  if( self->annex->hook_before != NULL &&
      ( err = self->annex->hook_before( hook_arg ) ) != 0 ) {
    pip_warn_mesg( "[%s] before-hook returns %d", argv[0], err );
    extval = err;
    goto error;
  }
  if( !pip_is_threaded_() ) {
    (void) setpgid( 0, pip_root_->task_root->annex->pid );
  }
  extval = pip_jump_into( args, self );
  /* the after hook is supposed to free the hook_arg being malloc()ed */
  /* and this is the reason to call this from the root process        */
  if( self->annex->hook_after != NULL &&
      ( err = self->annex->hook_after( hook_arg ) ) != 0 ) {
    pip_warn_mesg( "[%s] after-hook returns %d", argv[0], err );
    extval = err;
  }
 error:
  /* the main() or entry function returns here and terminate myself */
  pip_do_exit( self, extval );
  NEVER_REACH_HERE;
  return NULL;
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

static void pip_blocking_fin( pip_blocking_t *blocking ) {
  //(void) sem_destroy( &blocking->semaphore );
  (void) sem_destroy( blocking );
}

int pip_task_spawn( pip_spawn_program_t *progp,
		    int coreno,
		    uint32_t opts,
		    int *pipidp,
		    pip_spawn_hook_t *hookp ) {
  extern uint32_t pip_check_sync_flag_( uint32_t );
  cpu_set_t 		cpuset;
  pip_spawn_args_t	*args = NULL;
  pip_task_internal_t	*task = NULL;
  size_t		stack_size = pip_stack_size();
  int 			pipid;
  pid_t			pid = 0;
  int 			err = 0;

  ENTER;
  if( pip_root_       == NULL ) RETURN( EPERM  );
  if( pipidp          == NULL ) RETURN( EINVAL );
  if( !pip_isa_root()         ) RETURN( EPERM  );
  if( progp           == NULL ) RETURN( EINVAL );
  if( progp->funcname != NULL &&
      progp->argv     != NULL ) RETURN( EINVAL );
  if( progp->funcname == NULL &&
      progp->argv     == NULL ) RETURN( EINVAL );
  if( ( opts & PIP_SYNC_MASK ) == 0 ) {
    opts |= pip_root_->opts & PIP_SYNC_MASK;
  } else {
    if( ( opts = pip_check_sync_flag_( opts ) ) == 0 ) RETURN( EINVAL );
  }
  if( progp->funcname == NULL &&
      progp->prog     == NULL ) progp->prog = progp->argv[0];

  pipid = *pipidp;
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
  DBG;
  if( progp->envv == NULL ) progp->envv = environ;
  args = &task->annex->args;
  args->start_arg  = progp->arg;
  args->pipid      = pipid;
  args->coreno     = coreno;
  if( ( args->prog = strdup( progp->prog )              ) == NULL ||
      ( args->envv = pip_copy_env( progp->envv, pipid ) ) == NULL ) {
    ERRJ_ERR(ENOMEM);
  }
  if( progp->funcname == NULL ) {
    if( ( args->argv = pip_copy_vec( progp->argv ) ) == NULL ) {
      ERRJ_ERR(ENOMEM);
    }
  } else {
    if( ( args->funcname = strdup( progp->funcname ) ) == NULL ) {
      ERRJ_ERR(ENOMEM);
    }
  }

  pip_spin_lock( &pip_root_->lock_ldlinux );
  /*** begin lock region ***/
  do {
    if( ( err = pip_do_corebind( coreno, &cpuset ) ) == 0 ) {
      /* corebinding should take place before loading solibs,       */
      /* hoping anon maps would be mapped onto the closer numa node */
      PIP_ACCUM( time_load_prog,
		 ( err = pip_load_prog( progp, task ) ) == 0 );
      /* and of course, the corebinding must be undone */
      (void) pip_undo_corebind( coreno, &cpuset );
    }
  } while( 0 );
  /*** end lock region ***/
  pip_spin_unlock( &pip_root_->lock_ldlinux );
  ERRJ_CHK(err);

  pip_gdbif_task_new_( task );

  if( ( pip_root_->opts & PIP_MODE_PROCESS_PIPCLONE ) ==
      PIP_MODE_PROCESS_PIPCLONE ) {
    int flags =
      CLONE_VM |
      /* CLONE_FS | CLONE_FILES | */
      /* CLONE_SIGHAND | CLONE_THREAD | */
      CLONE_SETTLS |
      CLONE_PARENT_SETTID |
      CLONE_CHILD_CLEARTID |
      CLONE_SYSVSEM |
      CLONE_PTRACE |
      SIGCHLD;

    err = pip_clone_mostly_pthread_ptr( &task->annex->thread,
					flags,
					coreno,
					stack_size,
					(void*(*)(void*)) pip_do_spawn,
					args,
					&pid );
    DBGF( "pip_clone_mostly_pthread_ptr()=%d", err );
  } else {
    pthread_attr_t 	attr;
    pid_t tid = pip_gettid();

    if( ( err = pthread_attr_init( &attr ) ) == 0 ) {
      if( err == 0 ) {
	err = pthread_attr_setstacksize( &attr, stack_size );
	DBGF( "pthread_attr_setstacksize( %ld )= %d", stack_size, err );
      }
    }
    if( err == 0 ) {
      DBGF( "tid=%d  cloneinfo@%p", tid, pip_root_->cloneinfo );
      if( pip_root_->cloneinfo != NULL ) {
	/* lock is needed, because the preloaded clone()
	   might also be called from outside of PiP lib. */
	pip_spin_lock_wv( &pip_root_->cloneinfo->lock, tid );
      }
      do {
	err = pthread_create( &task->annex->thread,
			      &attr,
			      (void*(*)(void*)) pip_do_spawn,
			      (void*) args );
	DBGF( "pthread_create()=%d", errno );
      } while( 0 );
      /* unlock is done in the wrapper function */
      if( pip_root_->cloneinfo != NULL ) {
	pid = pip_root_->cloneinfo->pid_clone;
	pip_root_->cloneinfo->pid_clone = 0;
      }
    }
  }
  if( err == 0 ) {
    task->annex->pid = pid;
    pip_root_->ntasks_count ++;
    pip_root_->ntasks_accum ++;
    pip_root_->ntasks_curr  ++;
    pip_gdbif_task_commit_( task );
    *pipidp = pipid;

  } else {
  error:			/* undo */
    DBG;
    if( args != NULL ) {
      if( args->prog     != NULL ) free( args->prog );
      if( args->argv     != NULL ) free( args->argv );
      if( args->envv     != NULL ) free( args->envv );
      if( args->funcname != NULL ) free( args->funcname );
    }
    if( task != NULL ) {
      if( task->annex->loaded != NULL ) pip_dlclose_( task->annex->loaded );
      pip_reset_task_struct_( task );
    }
  }
  DBGF( "pip_task_spawn_(pipid=%d)", *pipidp );
  RETURN( err );
}

/*
 * The following functions must be called at root process
 */

void pip_finalize_task_( pip_task_internal_t *taski ) {
  DBGF( "pipid=%d  extval=0x%x", taski->pipid, taski->annex->extval );

  pip_gdbif_finalize_task_( taski );
  /* dlclose() and free() must be called only from the root process since */
  /* corresponding dlmopen() and malloc() is called by the root process   */
  if( taski->annex->loaded      != NULL ) pip_dlclose_( taski->annex->loaded );
  if( taski->annex->sleep_stack != NULL ) free( taski->annex->sleep_stack );
  if( taski->annex->args.prog   != NULL ) free( taski->annex->args.prog );
  if( taski->annex->args.argv   != NULL ) free( taski->annex->args.argv );
  if( taski->annex->args.envv   != NULL ) free( taski->annex->args.envv );
  if( *taski->annex->symbols.progname_full != NULL ) {
    free( *taski->annex->symbols.progname_full );
  }
  pip_blocking_fin( &taski->annex->sleep );
  pip_reset_task_struct_( taski );
}

int pip_fin( void ) {
  int ntasks, i;

  ENTER;
  if( pip_root_ == NULL ) RETURN( EPERM );
  if( pip_isa_root() ) {		/* root */
    ntasks = pip_root_->ntasks;
    for( i=0; i<ntasks; i++ ) {
      pip_task_internal_t *taski = &pip_root_->tasks[i];
      if( taski->pipid != PIP_PIPID_NULL ) {
	if( !taski->flag_exit ) {
	  DBGF( "%d/%d [pipid=%d (type=0x%x)] -- BUSY",
		i, ntasks, taski->pipid, taski->type );
	  RETURN( EBUSY );
	} else {
	  /* pip_wait*() was not called */
	  pip_finalize_task_( taski );
	}
      }
    }
    pip_named_export_fin_all_();
    /* report accumulated timer values, if set */
    PIP_REPORT( time_load_dso  );
    PIP_REPORT( time_load_prog );
    PIP_REPORT( time_dlmopen   );
    /* after this point DBG(F) macros cannot be used */
    memset( pip_root_, 0, pip_root_->size_whole );
    free( pip_root_ );
    pip_root_ = NULL;
    pip_task_ = NULL;
  } else {			/* tasks */
    pip_named_export_fin_( pip_task_ );
  }
  RETURN( 0 );
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

  DBGF( "pip_spawn()" );
  pip_spawn_from_main( &program, prog, argv, envv );
  pip_spawn_hook( &hook, before, after, hookarg );
  return pip_task_spawn( &program, coreno, 0, pipidp, &hook );
}

int pip_get_mode( int *mode ) {
  if( pip_root_ == NULL ) RETURN( EPERM  );
  if( mode      == NULL ) RETURN( EINVAL );
  *mode = ( pip_root_->opts & PIP_MODE_MASK );
  RETURN( 0 );
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
    *idp = (intptr_t) taski->annex->pid;
  }
  RETURN( 0 );
}

void pip_abort( void ) {
  extern int pip_raise_signal_( pip_task_internal_t*, int );
  ENTER;

  fflush( NULL );
  if( pip_root_ == NULL ) {
    kill( 0, SIGKILL );
  } else {
    (void) killpg( pip_root_->task_root->annex->pid, SIGKILL );
  }
}

void pip_abort_all_tasks( void ) {
  int	i, pipid;

  if( pip_isa_root() ) {
    for( i=0; i<pip_root_->ntasks; i++ ) {
      pipid = i;
      if( pip_check_pipid_( &pipid ) == 0 ) {
	(void) pip_raise_signal_( &pip_root_->tasks[i], SIGKILL );
      }
    }
  }
}
