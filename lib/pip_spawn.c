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
//#define PRINT_MAPS
//#define PRINT_FDS

/* the EVAL define symbol is to measure the time for calling dlmopen() */
//#define EVAL

#include <pip_internal.h>
#include <pip_dlfcn.h>
#include <pip_gdbif_func.h>

#include <sys/prctl.h>
#include <time.h>
#include <malloc.h> 		/* M_MMAP_THRESHOLD and M_TRIM_THRESHOLD  */

pip_spinlock_t *pip_lock_clone PIP_PRIVATE;

int pip_count_vec( char **vecsrc ) {
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

#define ENVLEN	(256)
static char **pip_copy_env( char **envsrc, int pipid ) {
  char rootenv[ENVLEN], taskenv[ENVLEN];
  char *preload_env = getenv( "LD_PRELOAD" );
  ASSERT( snprintf( rootenv, ENVLEN, "%s=%p", PIP_ROOT_ENV, pip_root ) <= 0 );
  ASSERT( snprintf( taskenv, ENVLEN, "%s=%d", PIP_TASK_ENV, pipid    ) <= 0 );
  return pip_copy_vec3( rootenv, taskenv, preload_env, envsrc );
}

size_t pip_stack_size( void ) {
  char 		*env, *endptr;
  size_t 	sz, scale;
  int 		i;

  if( ( sz = pip_root->stack_size_blt ) == 0 ) {
    if( ( env = getenv( PIP_ENV_STACKSZ ) ) == NULL &&
	( env = getenv( "KMP_STACKSIZE" ) ) == NULL &&
	( env = getenv( "OMP_STACKSIZE" ) ) == NULL ) {
      sz = PIP_STACK_SIZE;	/* default */
    } else {
      if( ( sz = (size_t) strtol( env, &endptr, 10 ) ) <= 0 ) {
	pip_err_mesg( "stacksize: '%s' is illegal and "
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
	  pip_err_mesg( "stacksize: '%s' is illegal and default is used",
			env );
	  sz = PIP_STACK_SIZE;
	  break;
	}
	sz = ( sz < PIP_STACK_SIZE_MIN ) ? PIP_STACK_SIZE_MIN : sz;
      }
    }
    pip_root->stack_size_blt = sz;
  }
  return sz;
}

int pip_isa_coefd( int fd ) {
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
	  pip_isa_coefd( fd ) ) {
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
	    pip_isa_coefd( fd ) ) {
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

static int pip_find_symbols( pip_spawn_program_t *progp,
			     void *handle,
			     pip_task_internal_t *taski ) {
  pip_symbols_t *symp = &taski->annex->symbols;
  int err = 0;

  //pip_glibc_lock();
  if( progp->funcname == NULL ) {
    symp->main           = dlsym( handle, "main"                  );
  } else {
    symp->start          = dlsym( handle, progp->funcname         );
  }
  /* GLIBC */
  /* the GLIBC _init() seems not callable. It seems that */
  /* dlmopen()ed name space does not setup VDSO properly */
  //symp->libc_init        = dlsym( handle, "_init"                 );
  //symp->res_init         = dlsym( handle, "__res_init"            );
  symp->ctype_init       = dlsym( handle, "__ctype_init"          );
  symp->mallopt          = dlsym( handle, "mallopt"               );
  symp->libc_fflush      = dlsym( handle, "fflush"                );

  if( symp->libc_init == NULL ) {
    /* GLIBC variables */
    symp->libc_argcp     = dlsym( handle, "__libc_argc"           );
    symp->libc_argvp     = dlsym( handle, "__libc_argv"           );
    symp->environ        = dlsym( handle, "environ"               );
    /* GLIBC misc. variables */
    symp->prog           = dlsym( handle, "__progname"            );
    symp->prog_full      = dlsym( handle, "__progname_full"       );
  }
  /* pip_named_export_fin symbol may not be found when the task
     program is not linked with the PiP lib. (due to calling
     no PiP functions) */
  symp->named_export_fin = dlsym( handle, "pip_named_export_fin_" );
  //pip_glibc_unlock();

  DBGF( "env:%p  func(%s):%p   main:%p",
	symp->environ, progp->funcname, symp->start, symp->main );

  /* check mandatory symbols */
  if( symp->libc_init == NULL ) {
    if( symp->environ == NULL) err = ENOEXEC;
  }
  /* check start function */
  if( progp->funcname == NULL ) {
    if( symp->main == NULL ) {
      pip_err_mesg( "Unable to find main "
		    "(possibly, not linked with '-rdynamic' option)" );
      err = ENOEXEC;
    }
  } else if( symp->start == NULL ) {
    pip_err_mesg( "Unable to find start function (%s)",
		      progp->funcname );
    err = ENOEXEC;
  }
  RETURN( err );
}

static char *pip_find_newer_libpipinit( void ) {
  struct stat	stb_install, stb_build;
  char		*newer = NULL;

  DBGF( "%s", PIP_INSTALL_LIBPIPINIT );
  DBGF( "%s", PIP_BUILD_LIBPIPINIT );
  memset( &stb_install, 0, sizeof(struct stat) );
  memset( &stb_build,   0, sizeof(struct stat) );
  if( stat( PIP_INSTALL_LIBPIPINIT, &stb_install ) != 0 ) {
    /* not found in install dir */
    if( stat( PIP_BUILD_LIBPIPINIT, &stb_build   ) != 0 ) {
      /* nowhere */
      return NULL;
    }
    newer = PIP_BUILD_LIBPIPINIT;
  } else if( stat( PIP_BUILD_LIBPIPINIT, &stb_build ) != 0 ) {
    /* not found in build dir, but install dir */
    newer = PIP_INSTALL_LIBPIPINIT;
  } else {
    /* both found, take the newer one */
    if( stb_install.st_mtime > stb_build.st_mtime ) {
      newer = PIP_INSTALL_LIBPIPINIT;
    } else {
      newer = PIP_BUILD_LIBPIPINIT;
    }
  }
  return newer;
}

static int
pip_load_dso( const char *path, pip_task_internal_t *taski ) {
  Lmid_t	lmid;
  char		*libpipinit = NULL;
  pip_init_t	pip_impinit = NULL;
  void 		*loaded     = NULL, *ld;
  int 		flags = RTLD_NOW | RTLD_LOCAL;
  /* RTLD_GLOBAL is NOT accepted and dlmopen() returns EINVAL */
  int		err = 0;

#ifdef RTLD_DEEPBIND
  flags |= RTLD_DEEPBIND;
#endif

  ENTERF( "path:%s", path );
  lmid = LM_ID_NEWLM;
  pip_dlerror();			/* reset dlerror */
  if( ( loaded = pip_dlmopen( lmid, path, flags ) ) == NULL ) {
    if( ( err = pip_check_pie( path, 1 ) ) != 0 ) goto error;
    pip_err_mesg( "dlmopen(%s): %s", path, pip_dlerror() );
    err = ENOEXEC;
    goto error;
  }
  /* call pip_init_task_implicitly */
  /*** we cannot call pip_ini_task_implicitly() directly here  ***/
  /*** the name space contexts of here and there are different ***/
  pip_dlerror();			/* reset dlerror */
  pip_impinit = (pip_init_t) pip_dlsym( loaded, "pip_init_task_implicitly" );
  if( pip_impinit == NULL ) {
    DBGF( "dl_sym: %s", pip_dlerror() );
    if( ( libpipinit = pip_find_newer_libpipinit() ) != NULL ) {
      DBGF( "libpipinit: %s", libpipinit );
      pip_dlerror();			/* reset dlerror */
      if( pip_dlinfo( loaded, RTLD_DI_LMID, &lmid ) != 0 ) {
	pip_err_mesg( "Unable to obtain Lmid (%s): %s",
		      libpipinit, pip_dlerror() );
	err = ENXIO;
	goto error;
      }
      DBGF( "lmid:%d", (int) lmid );
      pip_dlerror();		/* reset dlerror */
      if( ( ld = pip_dlmopen( lmid, libpipinit, flags ) ) == NULL ) {
	pip_warn_mesg( "Unable to load %s: %s", libpipinit, pip_dlerror() );
      } else {
	pip_dlerror();		/* reset dlerror */
	pip_impinit = (pip_init_t) pip_dlsym( ld, "pip_init_task_implicitly" );
	DBGF( "dlsym: %s", pip_dlerror() );
      }
    }
  }
  DBGF( "pip_impinit:%p", pip_impinit );
  if( pip_impinit != NULL ) {
    if( ( err = pip_impinit( pip_root, taski ) ) != 0 ) goto error;
  }
  taski->annex->loaded = loaded;
  RETURN( 0 );

 error:
  if( loaded != NULL ) pip_dlclose( loaded );
  RETURN( err );
}

static int pip_load_prog( pip_spawn_program_t *progp,
			  pip_spawn_args_t *args,
			  pip_task_internal_t *taski ) {
  void	*loaded;
  int 	err;

  ENTERF( "prog=%s", progp->prog );

  PIP_ACCUM( time_load_dso,
	     ( err = pip_load_dso( progp->prog, taski ) ) == 0 );
  if( err == 0 ) {
    loaded = taski->annex->loaded;
    if( ( err = pip_find_symbols( progp, loaded, taski ) ) ) {
      (void) pip_dlclose( loaded );
    } else {
      taski->annex->loaded = loaded;
      pip_gdbif_load( taski );
    }
  }
  RETURN( err );
}

int pip_do_corebind( pid_t tid, uint32_t coreno, cpu_set_t *oldsetp ) {
  cpu_set_t cpuset;
  int flags  = coreno >> PIP_CPUCORE_FLAG_SHIFT;
  int i, err = 0;

  ENTER;
  /* PIP_CPUCORE_ASIS flag is exclusive */
  if( ( flags & PIP_CPUCORE_ASIS ) != flags ) RETURN( EINVAL );
  if( flags & PIP_CPUCORE_ASIS ) RETURN( 0 );
  coreno &= PIP_CPUCORE_CORENO_MASK;
  if( coreno >= PIP_CPUCORE_CORENO_MAX ) RETURN ( EINVAL );
  if( tid == 0 ) tid = pip_gettid();

  if( oldsetp != NULL ) {
    if( sched_getaffinity( tid, sizeof(cpuset), oldsetp ) != 0 ) {
      RETURN( errno );
    }
  }

  if( flags == 0 ) {
    CPU_ZERO( &cpuset );
    CPU_SET( coreno, &cpuset );
    /* here, do not call pthread_setaffinity(). This MAY fail */
    /* because pd->tid is NOT set yet.  I do not know why.    */
    /* But it is OK to call sched_setaffinity() with tid.     */
    if( sched_setaffinity( tid, sizeof(cpuset), &cpuset ) != 0 ) {
      RETURN( errno );
    }

  } else if( ( flags & PIP_CPUCORE_NTH ) == PIP_CPUCORE_NTH ) {
    int ncores, nth;

    if( sched_getaffinity( tid, sizeof(cpuset), &cpuset ) != 0 ) {
      RETURN( errno );
    }
    if( ( ncores = CPU_COUNT( &cpuset ) ) ==  0 ) RETURN( 0 );
    coreno %= ncores;
    nth = coreno;
    for( i=0; ; i++ ) {
      if( !CPU_ISSET( i, &cpuset ) ) continue;
      if( nth-- == 0 ) {
	CPU_ZERO( &cpuset );
	CPU_SET( i, &cpuset );
	if( sched_setaffinity( tid, sizeof(cpuset), &cpuset ) != 0 ) {
	  err = errno;
	}
	break;
      }
    }
  }
  RETURN( err );
}

static int pip_undo_corebind( pid_t tid, uint32_t coreno, cpu_set_t *oldsetp ) {
  int err = 0;

  ENTER;
  if( ( coreno >> PIP_CPUCORE_FLAG_SHIFT ) & PIP_CPUCORE_ASIS ) {
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

  if( ( err = pip_check_pipid( &pipid ) ) != 0 ) RETURN( err );
  task = pip_get_task( pipid );
  if( loaded != NULL ) *loaded = task->annex->loaded;
  RETURN( 0 );
}

static void pip_glibc_init( pip_symbols_t *symbols,
			    pip_spawn_args_t *args,
			    void *loaded ) {
  /* setting GLIBC variables */
  if( symbols->libc_init != NULL ) {
    DBGF( ">> _init@%p", symbols->libc_init );
    symbols->libc_init( args->argc, args->argv, args->envv );
    DBGF( "<< _init@%p", symbols->libc_init );
  } else {
    if( symbols->libc_argcp != NULL ) {
      DBGF( "&__libc_argc=%p", symbols->libc_argcp );
      *symbols->libc_argcp = args->argc;
    }
    if( symbols->libc_argvp != NULL ) {
      DBGF( "&__libc_argv=%p", symbols->libc_argvp );
      *symbols->libc_argvp = args->argv;
    }
    if( symbols->environ != NULL ) {
      *symbols->environ = args->envv;	/* setting environment vars */
    }
    if( symbols->prog != NULL ) {
      *symbols->prog = args->prog;
    }
    if( symbols->prog_full != NULL ) {
      *symbols->prog_full = args->prog_full;
    }
    if( symbols->ctype_init != NULL ) {
      DBGF( ">> __ctype_init@%p", symbols->ctype_init );
      symbols->ctype_init();
      DBGF( "<< __ctype_init@%p", symbols->ctype_init );
    }
  }

#ifdef AH
  /* calling GLIBC initialization functions */
  if( symbols->res_init != NULL ) {
    DBGF( ">> res_init@%p", symbols->res_init );
    (void) symbols->res_init();
    DBGF( "<< res_init@%p", symbols->res_init );
  }
#endif

#ifndef PIP_NO_MALLOPT
  /* heap (using brk or sbrk) is not safe in PiP */
  if( symbols->mallopt != NULL ) {
#ifdef M_MMAP_THRESHOLD
    if( symbols->mallopt( M_MMAP_THRESHOLD, 1 ) == 1 ) {
      DBGF( "mallopt(M_MMAP_THRESHOLD): succeeded" );
    } else {
      pip_warn_mesg( "mallopt(M_MMAP_THRESHOLD): failed !!!!!!" );
    }
#endif
  }
#endif
}

void pip_return_from_start_func( pip_task_internal_t *taski, int extval ) {
  ENTER;
  if( taski->annex->hook_after != NULL ) {
    void *hook_arg = taski->annex->hook_arg;
    int err;
    /* the after hook is supposed to free the hook_arg being malloc()ed  */
    /* by root and this is the reason to call this from the root context */
    if( ( err = taski->annex->hook_after( hook_arg ) ) != 0 ) {
      pip_err_mesg( "PIPID:%d after-hook returns %d", taski->pipid, err );
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
    pip_return_from_start_func( pip_task, extval );
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

static void pip_start_cb( void *tsk ) {
   pip_task_internal_t *taski = (pip_task_internal_t*) tsk;
  DBG;
  /* let root proc know the task is running (or enqueued) */
  taski->annex->tid    = pip_gettid();
  DBGF( "TID:%d", taski->annex->tid );
  taski->annex->thread = pthread_self();
  pip_glibc_unlock();
  /* let pip-gdb know */
  pip_gdbif_task_commit( taski );
  /* sync with root */
  pip_sem_post( &taski->annex->task_root->sync_spawn );
}

static void pip_start_user_func( pip_spawn_args_t *args,
				 pip_task_internal_t *self ) {
  pip_task_queue_t *queue = args->queue;
  char **argv     = args->argv;
  char **envv     = args->envv;
  void *start_arg = args->start_arg;
  char *env_stop;
  int	i, err, extval;

  ENTER;
  pip_glibc_init( &self->annex->symbols, args, self->annex->loaded );

  DBGF( "fd_list:%p", args->fd_list );
  if( args->fd_list != NULL ) {
    for( i=0; args->fd_list[i]>=0; i++ ) { /* Close-on-exec FDs */
      DBGF( "COE: %d", args->fd_list[i] );
      (void) close( args->fd_list[i] );
    }
  }
  pip_gdbif_hook_before( self );

  if( ( env_stop = pip_root->envs.stop_on_start ) != NULL ) {
    int pipid_stop = strtol( env_stop, NULL, 10 );
    if( pipid_stop < 0 || pipid_stop == self->pipid ) {
      pip_info_mesg( "PiP task[%d] is SIGSTOPed (%s=%s)",
		     self->pipid, PIP_ENV_STOP_ON_START, env_stop );
      pip_kill( self->pipid, SIGSTOP );
    }
  }

  if( self->annex->opts & PIP_TASK_INACTIVE ) {
    /* inactive task: suspend myself */
    pip_suspend_and_enqueue_generic( self,
				     queue,
				     1, /* lock flag */
				     pip_start_cb,
				     self );
    /* resumed */
  } else {
    /* active task and take tasks in the queue, if any */
    if( queue != NULL ) {
      int n = PIP_TASK_ALL;
      err = pip_dequeue_and_resume_multiple( self, queue, self, &n );
    }
    /* since there is no callback, the cb func is called explicitly */
    pip_start_cb( (void*) self );
  }

  if( self->annex->hook_before != NULL &&
      ( err = self->annex->hook_before( self->annex->hook_arg ) ) != 0 ) {
    pip_err_mesg( "[%s] before-hook returns %d", argv[0], err );
    extval = err;

  } else {
    if( self->annex->symbols.start == NULL ) {
      /* calling hook function, if any */
      DBGF( "[%d] >> main@%p(%d,%s,%s,...)",
	    args->pipid, self->annex->symbols.main, args->argc, argv[0], argv[1] );
      extval = self->annex->symbols.main( args->argc, argv, envv );
      DBGF( "[%d] << main@%p(%d,%s,%s,...) = %d",
	    args->pipid, self->annex->symbols.main, args->argc, argv[0], argv[1],
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
#ifdef AH
  if( (pid_t) self->task_sched->annex->tid != pip_gettid() ) {
    /* when a pip task call fork() and the forked */
    /* process returns from main, this may happen  */
    /* XXXXX in BLT this may happen !!!!! */
    DBGF( "Fork?? (%d : %d)", self->task_sched->annex->tid, pip_gettid() );
    exit( extval );
    NEVER_REACH_HERE;
  }
#endif
  pip_return_from_start_func( self, extval );
  NEVER_REACH_HERE;
}

static void pip_sigquit_handler( int sig,
				 void(*handler)(),
				 struct sigaction *oldp ) {
  DBG;
  pthread_exit( NULL );
}

static void* pip_spawn_top( void *thargs )  {
  /* The context of this function is of the root task                */
  /* so the global var; pip_task (and pip_root) are of the root task */
  /* and do not call malloc() and free() in this contxt !!!!         */
  pip_spawn_args_t	*args  = (pip_spawn_args_t*) thargs;
  int	 		pipid  = args->pipid;
  int 			coreno = args->coreno;
  pip_task_internal_t 	*self  = &pip_root->tasks[pipid];
  int			err    = 0;

  ENTER;
  if( !pip_is_threaded_() ) {
    pip_set_signal_handler( SIGQUIT, pip_sigquit_handler, NULL );
  }
  pip_set_name( self );
  pip_save_tls( &self->tls );
  pip_memory_barrier();

  if( !pip_is_threaded_() ) {
    pip_reset_signal_handler( SIGCHLD );
    pip_reset_signal_handler( SIGTERM );
    (void) setpgid( 0, (pid_t) pip_root->task_root->annex->tid );
  }
  if( ( err = pip_do_corebind( 0, coreno, NULL ) ) != 0 ) {
    pip_warn_mesg( "failed to bound CPU core:%d (%d)", coreno, err );
    err = 0;
  }
  if( err == 0 ) {
    PIP_RUN( self );
    pip_start_user_func( args, self );
  } else {
    pip_do_exit( self, err );
  }
  NEVER_REACH_HERE;
  return NULL;			/* dummy */
}

static int pip_find_a_free_task( int *pipidp ) {
  int pipid = *pipidp;
  int i, err = 0;

  if( pip_root->ntasks_accum >= PIP_NTASKS_MAX ) RETURN( EOVERFLOW );
  if( pipid < PIP_PIPID_ANY || pipid >= pip_root->ntasks ) {
    DBGF( "pipid=%d", pipid );
    RETURN( EINVAL );
  }

  pip_spin_lock( &pip_root->lock_tasks );
  /*** begin lock region ***/
  do {
    DBGF( "pipid:%d  ntasks:%d  pipid_curr:%d",
	  pipid, pip_root->ntasks, pip_root->pipid_curr );
    if( pipid != PIP_PIPID_ANY ) {
      if( pip_root->tasks[pipid].pipid != PIP_PIPID_NULL ) {
	err = EAGAIN;
	goto unlock;
      }
    } else {
      for( i=pip_root->pipid_curr; i<pip_root->ntasks; i++ ) {
	if( pip_root->tasks[i].pipid == PIP_PIPID_NULL ) {
	  pipid = i;
	  goto found;
	}
      }
      for( i=0; i<=pip_root->pipid_curr; i++ ) {
	if( pip_root->tasks[i].pipid == PIP_PIPID_NULL ) {
	  pipid = i;
	  goto found;
	}
      }
      err = EAGAIN;
      goto unlock;
    }
  found:
    pip_root->tasks[pipid].pipid = pipid;	/* mark it as occupied */
    pip_root->pipid_curr = pipid + 1;
    *pipidp = pipid;

  } while( 0 );
 unlock:
  /*** end lock region ***/
  pip_spin_unlock( &pip_root->lock_tasks );

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
  if( !pip_is_initialized()   ) RETURN( EPERM  );
  if( !pip_isa_root()         ) RETURN( EPERM  );
  if( progp           == NULL ) RETURN( EINVAL );
  if( progp->prog     == NULL ) RETURN( EINVAL );
  /* starting from main */
  if( progp->funcname == NULL &&
      ( progp->argv == NULL || progp->argv[0] == NULL ) ) {
    RETURN( EINVAL );
  }
  /* starting from an arbitrary func */
  if( ( op = pip_check_sync_flag( opts ) ) < 0 ) {
    DBGF( "opts:0x%x  op:0x%x", opts, op );
    RETURN( EINVAL );
  }
  if( pip_check_task_flag( opts ) < 0 ) {
    DBGF( "opts:0x%x  op:0x%x", opts, op );
    RETURN( EINVAL );
  }
  opts = op;

  if( progp->funcname == NULL &&
      progp->prog     == NULL ) progp->prog = progp->argv[0];

  if( pipid == PIP_PIPID_MYSELF ) RETURN( EINVAL );
  if( pipid != PIP_PIPID_ANY ) {
    if( pipid < 0 || pipid > pip_root->ntasks ) RETURN( EINVAL );
  }
  if( ( err = pip_find_a_free_task( &pipid ) ) != 0 ) ERRJ;
  task = &pip_root->tasks[pipid];
  pip_reset_task_struct( task );
  task->pipid      = pipid;	/* mark it as occupied */
  task->type       = PIP_TYPE_TASK;
  task->task_sched = task;
  task->ntakecare  = 1;
  task->annex->opts      = opts;
  task->annex->task_root = pip_root;
  if( progp->exp != NULL ) {
    task->annex->import_root = progp->exp;
  } else {
    task->annex->import_root = pip_root->export_root;
  }
  if( hookp != NULL ) {
    task->annex->hook_before = hookp->before;
    task->annex->hook_after  = hookp->after;
    task->annex->hook_arg    = hookp->hookarg;
  }
  SET_CURR_TASK( task, task );
  /* allocate stack for sleeping */
  stack_size = pip_stack_size();
  pip_page_alloc( pip_root->stack_size_sleep, &task->annex->stack_sleep );
  if( task->annex->stack_sleep == NULL ) ERRJ_ERR( ENOMEM );

  args = &task->annex->args;
  args->start_arg = progp->arg;
  args->pipid     = pipid;
  args->coreno    = coreno;
  args->queue     = queue;
  args->prog = strdup( progp->prog );
  if( args->prog == NULL ) ERRJ_ERR( ENOMEM );
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
    args->argc = pip_count_vec( args->argv );
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
  pip_gdbif_task_new( task );

  if( ( err = pip_do_corebind( 0, coreno, &cpuset ) ) == 0 ) {
    /* corebinding should take place before loading solibs,       */
    /* hoping anon maps would be mapped onto the closer numa node */
    PIP_ACCUM( time_load_prog,
	       ( err = pip_load_prog( progp, args, task ) ) == 0 );
    /* and of course, the corebinding must be undone */
    (void) pip_undo_corebind( 0, coreno, &cpuset );
  }
  ERRJ_CHK( err );

  if( ( pip_root->opts & PIP_MODE_PROCESS_PIPCLONE ) ==
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

    /* we need lock on ldlinux. supposedly glibc does someting */
    /* before calling main function */
    pip_glibc_lock();
    err = pip_clone_mostly_pthread_ptr( (pthread_t*) &task->annex->thread,
					flags,
					coreno == PIP_CPUCORE_ASIS ? - 1 :
					coreno,
					stack_size,
					(void*(*)(void*)) pip_spawn_top,
					args,
					&pid );
    DBGF( "pip_clone_mostly_pthread_ptr()=%d", err );
    if( err ) pip_glibc_unlock();

  } else {
    pthread_t		thr;
    pthread_attr_t 	attr;
    pid_t		tid = pip_gettid();

    if( ( err = pthread_attr_init( &attr ) ) == 0 ) {
      err = pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_JOINABLE );
      DBGF( "pthread_attr_setdetachstate(JOINABLE)= %d", err );
      if( err ) goto error;
      err = pthread_attr_setstacksize( &attr, stack_size );
      DBGF( "pthread_attr_setstacksize( %ld )= %d", stack_size, err );
      if( err ) goto error;

      /* we need lock on ldlinux. supposedly glibc does someting */
      /* before calling main function */
      pip_glibc_lock();
      {
	if( pip_lock_clone != NULL ) {
	  /* lock is needed, because the pip_clone()
	     might also be called from outside of PiP lib. */
	  pip_spin_lock_wv( pip_lock_clone, tid );
	}
	/* unlock is done in the wrapper function */
	err = pthread_create( &thr,
			      &attr,
			      (void*(*)(void*)) pip_spawn_top,
			      (void*) args );
	DBGF( "pthread_create()=%d", err );
      }
      if( err ) pip_glibc_unlock();
    }
  }
  if( err == 0 ) {
    /* wait until task starts running or enqueues */
    pip_sem_wait( &pip_root->sync_spawn );
    pip_root->ntasks_count ++;
    pip_root->ntasks_accum ++;
    if( bltp != NULL ) *bltp = (pip_task_t*) task;

    DBGF( "task (PIPID:%d,TID:%d) is created and running",
	  task->pipid, task->annex->tid );

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
      pip_reset_task_struct( task );
    }
  }
  DBGF( "pip_task_spawn(pipid=%d)", pipid );
  RETURN( err );
}

/*
 * The following functions must be called at root process
 */

int pip_task_spawn( pip_spawn_program_t *progp,
		    uint32_t coreno,
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
		    uint32_t coreno,
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
	       uint32_t coreno,
	       int  *pipidp,
	       pip_spawnhook_t before,
	       pip_spawnhook_t after,
	       void *hookarg ) {
  pip_spawn_program_t program;
  pip_spawn_hook_t hook;

  ENTER;
  if( prog == NULL ) return EINVAL;
  pip_spawn_from_main( &program, prog, argv, envv, NULL );
  pip_spawn_hook( &hook, before, after, hookarg );
  RETURN( pip_task_spawn( &program, coreno, 0, pipidp, &hook ) );
}
