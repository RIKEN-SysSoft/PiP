/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/wait.h>
#include <malloc.h>
#include <dlfcn.h>
#include <dirent.h>
#include <sched.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#define DEBUG

#include <pip.h>
#include <pip_internal.h>

/*** note that the following static variables are   ***/
/*** located at each PIP task and the root process. ***/
static pip_root_t	*pip_root = NULL;
static pip_task_t	*pip_self = NULL;

int pip_page_alloc( size_t sz, void **allocp ) {
  int pgsz = sysconf( _SC_PAGESIZE );
  if( posix_memalign( allocp, (size_t) pgsz, sz ) != 0 ) RETURN( errno );
  return 0;
}

static void pip_set_magic( pip_root_t *root ) {
  memcpy( root->magic, PIP_MAGIC_WORD, PIP_MAGIC_LEN );
}

static int pip_is_magic_ok( pip_root_t *root ) {
  return strncmp( root->magic, PIP_MAGIC_WORD, PIP_MAGIC_LEN ) == 0;
}

int pip_init( int *pipidp, int *ntasksp, void **rt_expp, int opts ) {
  pip_clone_t*	cloneinfo = NULL;
  size_t	sz;
  char		*env = NULL;
  int		ntasks;
  int 		pipid;
  int 		i;
  int 		err = 0;

  if( pip_root != NULL ) RETURN( EBUSY ); /* already initialized */

#ifndef PIP_NO_MALLOPT
  (void) mallopt( M_MMAP_THRESHOLD, 1 ); /* not to call (s)brk() */
#endif

  //pip_print_maps();
  if( ( env = getenv( PIP_ROOT_ENV ) ) == NULL ) {
    /* root process ? */

    if( ntasksp  == NULL ) {
      ntasks = PIP_NTASKS_MAX;
    } else if( *ntasksp <= 0 ) {
      RETURN( EINVAL );
    } else {
      ntasks = *ntasksp;
    }
    if( ntasks > PIP_NTASKS_MAX ) RETURN( EOVERFLOW );

    if( opts & PIP_MODEL_PTHREAD &&
	opts & PIP_MODEL_PROCESS ) RETURN( EINVAL );

    if( opts & PIP_MODEL_PROCESS ) {
      /* check if the __clone() systemcall wrapper exists or not */
      cloneinfo = (pip_clone_t*) dlsym( RTLD_DEFAULT, "pip_clone_info");
      if( cloneinfo == NULL ) {
	/* no wrapper found */
	if( ( env = getenv( "LD_PRELOAD" ) ) == NULL ) {
	  fprintf( stderr,
		   "PIP: process model is desired but "
		   "LD_PRELOAD environment variable is empty.\n" );
	} else {
	  fprintf( stderr,
		   "PIP: process model is desired but LD_PRELOAD='%s' "
		   "seems not to be effective.\n",
		   env );
	}
	RETURN( EPERM );
      }
    }

    sz = sizeof( pip_root_t ) + sizeof( pip_task_t ) * ntasks;
    if( ( err = pip_page_alloc( sz, (void**) &pip_root ) ) != 0 ) {
      RETURN( err );
    }
    (void) memset( pip_root, 0, sz );

    DBGF( "ROOTROOT (%p)", pip_root );

    pip_spin_init( &pip_root->spawn_lock );

    if( asprintf( &env, "%s=%p", PIP_ROOT_ENV, pip_root ) == 0 ) {
      err = ENOMEM;
    } else {
      if( putenv( env ) != 0 ) {
	err = errno;
      } else {
	pipid = PIP_PIPID_ROOT;
	pip_set_magic( pip_root );
	pip_root->ntasks    = ntasks;
	pip_root->thread    = pthread_self();
	pip_root->cloneinfo = cloneinfo;
	pip_root->opts      = opts;
	pip_root->pid       = getpid();
	if( rt_expp != NULL ) pip_root->export = *rt_expp;

	for( i=0; i<ntasks; i++ ) {
	  pip_root->tasks[i].pipid  = PIP_PIPID_NONE;
	  pip_root->tasks[i].thread = pip_root->thread;
	}
      }
    }
    pip_self = NULL;
    if( err ) {		/* undo */
      (void) unsetenv( PIP_ROOT_ENV );
      if( pip_root != NULL ) free( pip_root );
      pip_root = NULL;
    }
  } else {
    /* child task */
    intptr_t	ptr = (intptr_t) strtoll( env, NULL, 16 );

    pip_root = (pip_root_t*) ptr;
    if( !pip_is_magic_ok( pip_root ) ) RETURN( EPERM );

    ntasks = pip_root->ntasks;
    pipid  = PIP_PIPID_NONE;
    for( i=0; i<ntasks; i++ ) {
      if( pthread_equal( pip_root->tasks[i].thread, pthread_self() ) ) {
	pipid = i;
	pip_self = &pip_root->tasks[pipid];
	break;
      }
    }
    DBGF( "CHILDCHILD (%p) %d", pip_root, pipid );
    if( pipid < 0 ) {
      err = ENXIO;
    } else {
      if( ntasksp != NULL ) *ntasksp = ntasks;
      if( rt_expp != NULL ) *rt_expp = (void*) pip_root->export;
    }
  }
  /* root and child */
  if( !err ) {
    if( pipidp != NULL ) *pipidp = pipid;
    DBGF( "pip_self=%p  pip_root=%p", pip_self, pip_root );
  }
  RETURN( err );
}

static int pip_if_pthread_( void ) {
  if( pip_root->cloneinfo != NULL ) {
    return pip_root->cloneinfo->flag_clone & CLONE_THREAD;
  }
  return 1;
}

int pip_if_pthread( int *flagp ) {
  if( pip_root == NULL ) RETURN( EPERM  );
  if( flagp    == NULL ) RETURN( EINVAL );
  *flagp = pip_if_pthread_();
  RETURN( 0 );
}

static int pip_if_shared_fd_( void ) {
  if( pip_root->cloneinfo == NULL ) return 1;
  return pip_root->cloneinfo->flag_clone & CLONE_FILES;
}

int pip_if_shared_fd( int *flagp ) {
  /* this function is only valid on the root process */
  if( pip_root == NULL ) RETURN( EPERM  );
  if( flagp    == NULL ) RETURN( EINVAL );
  *flagp = pip_if_shared_fd_();
  RETURN( 0 );
}

int pip_if_shared_sighand( int *flagp ) {
  /* this function is only valid on the root process */
  if( pip_root == NULL ) RETURN( EPERM  );
  if( flagp    == NULL ) RETURN( EINVAL );
  if( pip_root->cloneinfo == NULL ) {
    *flagp = 1;
  } else {
    *flagp = pip_root->cloneinfo->flag_clone & CLONE_SIGHAND;
  }
  RETURN( 0 );
}

static int pip_root_p( void ) {
  return pip_self == NULL;
}

int pip_get_pipid( int *pipidp ) {
  if( pipidp   == NULL ) RETURN( EINVAL );
  if( pip_root == NULL ) return( EPERM  ); /* intentionally using return */

  if( pip_root_p() ) {
    *pipidp = PIP_PIPID_ROOT;
  } else {
    *pipidp = pip_self->pipid;
  }
  RETURN( 0 );
}

int pip_get_current_ntasks( int *ntasksp ) {
  if( ntasksp  == NULL ) RETURN( EINVAL );
  if( pip_root == NULL ) return( EPERM  ); /* intentionally using return */

  *ntasksp = pip_root->ntasks_curr;
  RETURN( 0 );
}

int pip_export( void *export ) {
  if( pip_root         == NULL ) RETURN( EPERM  );
  if( pip_root_p()             ) RETURN( EPERM  );
  /* root export address is already set by pip_init() */
  /* and you cannot reset it again                    */
  if( export           == NULL ) RETURN( EINVAL );
  if( pip_self->export != NULL ) RETURN( EBUSY  );

  pip_self->export = export;
  RETURN( 0 );
}

int pip_import( int pipid, void **exportp  ) {
  if( pip_root == NULL ) RETURN( EPERM  );
  if( exportp  == NULL ) RETURN( EINVAL );
  if( pipid == PIP_PIPID_ROOT ) {
    *exportp = (void*) pip_root->export;
  } else {
    if( pipid < 0 || pipid >= pip_root->ntasks ) RETURN( EINVAL );
    if( pip_root->tasks[pipid].pipid != pipid  ) RETURN( ENOENT );
    *exportp = (void*) pip_root->tasks[pipid].export;
  }
  pip_memory_barrier();
  RETURN( 0 );
}

char **pip_copy_vec( char *addition, char **vecsrc ) {
  char **vecdst;
  char *p;
  int vecln;
  int veccc;
  int i, j;

  if( addition != NULL ) {
    vecln = 1;
    veccc = strlen( addition ) + 1;
  } else {
    vecln = 0;
    veccc = 0;
  }

  for( i=0; vecsrc[i]!=NULL; i++ ) {
    veccc += strlen( vecsrc[i] ) + 1;
  }
  vecln += i + 1;		/* plus final NULL */

  if( ( vecdst = (char**) malloc( sizeof(char*) * vecln + veccc ) ) == NULL ) {
    return NULL;
  }
  j = 0;
  p = ((char*)vecdst) + ( sizeof(char*) * vecln );
  vecdst[j++] = p;
  if( addition ) {
    p = stpcpy( p, addition ) + 1;
    i = 0;
  } else {
    p = stpcpy( p, vecsrc[0] ) + 1;
    i = 1;
  }
  for( ; vecsrc[i]!=NULL; i++ ) {
    vecdst[j++] = p;
    p = stpcpy( p, vecsrc[i] ) + 1;
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

static char **pip_copy_env( char **envsrc ) {
  char *rootenv;
  char **envdst;
  int cc;

  cc = asprintf( &rootenv, "%s=%p", PIP_ROOT_ENV, pip_root );
  if( cc == 0 ) return NULL;
  envdst = pip_copy_vec( rootenv, envsrc );
  free( rootenv );
  return envdst;
}

static void pip_close_on_exec( void ) {
  DIR *dir;
  struct dirent *direntp;
  int fd;
  int flags;

#define PROCFD_PATH		"/proc/self/fd"
  if( ( dir = opendir( PROCFD_PATH ) ) != NULL ) {
    while( ( direntp = readdir( dir ) ) != NULL ) {
      if( ( fd = atoi( direntp->d_name ) ) >= 0 &&
	  ( flags = fcntl( fd, F_GETFD ) ) >= 0 &&
	  flags & FD_CLOEXEC ) {
	(void) close( fd );
	DBGF( "fd[%d] is closed (CLOEXEC)", fd );
      }
    }
    (void) closedir( dir );
  }
}

static void pip_finalize_task( pip_task_t *task ) {
  DBGF( "pipid=%d", task->pipid );
  task->mainf  = NULL;
  if( task->argv != NULL ) {
    free( task->argv );
    task->argv = NULL;
  }
  if( task->envv != NULL ) {
    free( task->envv );
    task->envv = NULL;
  }
  task->envvp = NULL;
  task->export = NULL;
}

static int pip_do_spawn( void *thargs )  {
  pip_spawn_args_t *args = (pip_spawn_args_t*) thargs;
  int 	pipid      = args->pipid;
  char **argv      = args->argv;
  char **envv      = args->envv;
  pip_spawnhook_t before = args->hook_before;
  pip_spawnhook_t after  = args->hook_after;
  void	*hook_arg  = args->hook_arg;
  pip_task_t *self = &pip_root->tasks[pipid];
  int err = 0;

  DBG;
  free( thargs );

  if( !pip_if_shared_fd_() ) pip_close_on_exec();

  /* calling hook, if any */
  if( before == NULL || ( err = before( hook_arg ) ) == 0 ) {
    /* argv and/or envv might be changed in the hook function */
    volatile int	flag_exit;	/* must be volatile */
    ucontext_t 		ctx;
    int   		argc;

    for( argc=0; argv[argc]!=NULL; argc++ );
    self->argv   = argv;
    *self->envvp = envv;	/* setting environment vars */

    flag_exit = 0;
    (void) getcontext( &ctx );
    if( !flag_exit ) {
      flag_exit = 1;
      self->ctx = &ctx;

      DBGF( "[%d] >> main(%d,%s,%s,...)", pipid, argc, argv[0], argv[1] );
      self->retval = self->mainf( argc, argv );
      DBGF( "[%d] << main(%d,%s,%s,...)", pipid, argc, argv[0], argv[1] );

    } else {
      DBGF( "[%d] !! main(%d,%s,%s,...)", pipid, argc, argv[0], argv[1] );
    }
    if( after != NULL ) (void) after( hook_arg );

  } else if( err != 0 ) {
    fprintf( stderr,
	     "PIP: try to spawn(%s), but the before hook returns %d\n",
	     argv[0],
	     err );
    self->retval = err;
  }
  DBG;
  pip_finalize_task( self );
  DBG;
  RETURN( 0 );
}

/*
  To have separate GOTs
  1) dlmopen()
  --- CAUTION: the max number of link maps is currently 16
  --- which is defined in GLIB/sysdeps/generic/ldsodefs.h (DL_NNS)
  2) get link map by calling dlinfo(handle)
*/

static int pip_load_so( void **handlep, char *path ) {
  Lmid_t	lmid;
  int 		flags = RTLD_NOW | RTLD_LOCAL;
  /* RTLD_GLOBAL is NOT accepted and dlmopen() returns EINVAL */
  void 		*loaded;

  if( *handlep == NULL ) {
    lmid = LM_ID_NEWLM;
  } else {
    if( dlinfo( *handlep, RTLD_DI_LMID, (void*) &lmid ) != 0 ) {
      DBGF( "dlinfo(%p): %s", handlep, dlerror() );
      RETURN( ENXIO );
    }
  }
  if( ( loaded = dlmopen( lmid, path, flags ) ) == NULL ) {
    DBGF( "dlmopen(%s): %s", path, dlerror() );
    RETURN( ENXIO );
  } else {
    DBG;
    *handlep = loaded;
  }
  RETURN( 0 );
}

static int pip_load_prog( char *prog, pip_task_t *task ) {
  void		*loaded   = NULL;
  char		*solibs[] = PIP_LD_SOLIBS;
  char 		***envvp;
  main_func_t 	main_func;
  int 		i;
  int 		err;

  for( i=0; solibs[i]!=NULL; i++ ) {
    if( ( err = pip_load_so( &loaded, solibs[i] ) ) != 0 ) goto error;
  }
  DBGF( "prog=%s", prog );
  if( ( err = pip_load_so( &loaded, prog ) ) == 0 ) {
    DBG;
    if( ( main_func = (main_func_t) dlsym( loaded, MAIN_FUNC ) ) == NULL ) {
	/* getting main function address to invoke */
      DBG;
      err = ENXIO;
    } else if( ( envvp = (char***) dlsym( loaded, "environ" ) ) == NULL ) {
      /* getting address of environmanet variable to be set */
      DBG;
      err = ENXIO;
    }
  }
 error:
  if( err == 0 ) {
    task->mainf  = main_func;
    task->envvp  = envvp;
    task->loaded = loaded;
  } else if( loaded != NULL ) {
    (void) dlclose( loaded );
  }
  RETURN( err );
}

static int pip_corebind( int coreno, cpu_set_t *oldsetp ) {
  int err = 0;

  if( coreno != PIP_CPUCORE_ASIS ) {
    cpu_set_t cpuset;

    CPU_ZERO( &cpuset );
    CPU_SET( coreno, &cpuset );

    if( pip_if_pthread_() ) {
      err = pthread_getaffinity_np( pthread_self(),
				    sizeof(cpu_set_t),
				    oldsetp );
      if( err == 0 ) {
	err = pthread_setaffinity_np( pthread_self(),
				      sizeof(cpu_set_t),
				      &cpuset );
      }
    } else {
      if( sched_getaffinity( 0, sizeof(cpuset), oldsetp ) != 0 ) {
	err = errno;
      } else if( sched_setaffinity( 0, sizeof(cpuset), &cpuset ) != 0 ) {
	err = errno;
      }
    }
  }
  RETURN( err );
}

static int pip_undo_corebind( int coreno, cpu_set_t *oldsetp ) {
  int err = 0;

  if( coreno != PIP_CPUCORE_ASIS ) {
    if( pip_if_pthread_() ) {
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

static void pip_clone_wrap_begin( void ) {
  if( pip_root->opts      &  PIP_MODEL_PROCESS &&
      pip_root->cloneinfo != NULL) {
    DBGF( "cloneinfo=%p", pip_root->cloneinfo );
    pip_root->cloneinfo->flag_wrap = 1;
  }
}

int pip_spawn( char *prog,
	       char **argv,
	       char **envv,
	       int  coreno,
	       int  *pipidp,
	       pip_spawnhook_t before,
	       pip_spawnhook_t after,
	       void *hookarg ) {
  extern char **environ;
  cpu_set_t 		cpuset;
  pip_spawn_args_t	*args = NULL;
  pip_task_t		*task;
  int 			pipid;
  int 			err;

  DBG;

  if( pip_root == NULL ) RETURN( EPERM );
  if( pipidp   == NULL ) RETURN( EINVAL );
  pipid = *pipidp;
  if( pipid < PIP_PIPID_ANY || pipid >= pip_root->ntasks ) {
    DBGF( "pipid=%d", pipid );
    RETURN( EINVAL );
  }
  if( pip_root->ntasks_accum >= PIP_NTASKS_MAX ) RETURN( EOVERFLOW );
  if( !pthread_equal( pthread_self(), pip_root->thread ) ) {
    RETURN( EPERM );
  }
  if( envv == NULL ) envv = environ;

  args = (pip_spawn_args_t*) malloc( sizeof(pip_spawn_args_t) );
  if( args == NULL ) RETURN( ENOMEM );

  args->coreno = coreno;
  args->prog   = prog;
  args->argv   = pip_copy_vec( NULL, argv );
  args->envv   = pip_copy_env( envv );

  pip_spin_lock( &pip_root->spawn_lock );
  do {
    /*** begin lock region ***/
    if( pipid != PIP_PIPID_ANY ) {
      if( pip_root->tasks[pipid].pipid != PIP_PIPID_NONE ) {
	DBG;
	err = EAGAIN;
	goto unlock;
      }
    } else {
      static int pipid_curr = 0;
      int i;

      for( i=pipid_curr; i<pip_root->ntasks; i++ ) {
	if( pip_root->tasks[i].pipid == PIP_PIPID_NONE ) goto found;
      }
      for( i=0; i<pipid_curr; i++ ) {
	if( pip_root->tasks[i].pipid == PIP_PIPID_NONE ) goto found;
      }
      err = EAGAIN;
      goto unlock;

    found:
      pipid      = i;
      pipid_curr = pipid + 1;
    }
    args->pipid       = pipid;
    args->hook_before = before;
    args->hook_after  = after;
    args->hook_arg    = hookarg;

    task = &pip_root->tasks[pipid];

    if( ( err = pip_corebind( coreno, &cpuset ) ) == 0 ) {
      /* corebinding should take place before loading solibs,        */
      /* hoping anon maps would be mapped ontto the closer numa node */

      if( ( err = pip_load_prog( prog, task ) ) == 0 ) {
	pip_clone_wrap_begin();	/* tell clone wrapper, if any*/
	err = pthread_create( &task->thread,
			      NULL,
			      (void*(*)(void*)) pip_do_spawn,
			      (void*) args );
	if( err == 0 ) {
	  task->pipid = *pipidp = pipid;
	  if( pip_root->cloneinfo != NULL ) {
	    task->pid = pip_root->cloneinfo->pid_clone;
	  }
	  pip_root->ntasks_accum ++;
	  pip_root->ntasks_curr  ++;
	  if( pip_root->cloneinfo != NULL ) {
	    pip_root->cloneinfo->pid_clone  = 0;
	  }
	}
      }
      /* and of course, the corebinding must be undone */
      (void) pip_undo_corebind( coreno, &cpuset );
    }
    /*** end lock region ***/
  } while( 0 );
 unlock:
  pip_spin_unlock( &pip_root->spawn_lock );

  if( err != 0 ) {		/* undo */
    DBGF( "err=%d", err );
    pip_finalize_task( &pip_root->tasks[pipid] );
    if( args != NULL ) free( args );
  }
  RETURN( err );
}

int pip_fin( void ) {
  int ntasks;
  int i;
  int err = 0;

  DBG;
  if( pip_root == NULL || !pip_root_p() ) RETURN( EPERM );
  DBG;

  ntasks = pip_root->ntasks;
  for( i=0; i<ntasks; i++ ) {
    if( pip_root->tasks[i].pipid != PIP_PIPID_NONE ) {
      DBGF( "%d/%d [%d] -- BUSY", i, ntasks, pip_root->tasks[i].pipid );
      err = EBUSY;
      break;
    }
  }
  if( err == 0 ) {
    memset( pip_root,
	    0,
	    sizeof( pip_root_t ) + sizeof( pip_task_t ) * ntasks );
    free( pip_root );
    pip_root = NULL;
    (void) unsetenv( PIP_ROOT_ENV );
  }
  RETURN( err );
}

int pip_get_pid( int pipid, pid_t *pidp ) {
  if( pip_root == NULL ) RETURN( EPERM );
  if( pidp     == NULL ) RETURN( EINVAL );
  if( pipid    <  PIP_PIPID_ROOT ||
      pipid    >= pip_root->ntasks ) RETURN( EINVAL );
  if( pip_if_pthread_() ) RETURN( ENOSYS );

  if( pipid == PIP_PIPID_ROOT ) {
    *pidp = (pid_t) pip_root->pid;
  } else {
    *pidp = (pid_t) pip_root->tasks[pipid].pid;
  }
  RETURN( 0 );
}

int pip_kill( int pipid, int signal ) {
  pip_task_t *task;
  int err  = 0;

  if( signal    < 0              ) RETURN( EINVAL );
  if( pipid     < PIP_PIPID_ROOT ) RETURN( EINVAL );
  if( pip_root == NULL           ) RETURN( EPERM  );
  if( pipid >= pip_root->ntasks  ) RETURN( ENOENT );

  if( pipid == PIP_PIPID_ROOT ) {
    if( pip_if_pthread_() ) {
      err = pthread_kill( pip_root->thread, signal );
      DBGF( "pthread_kill(ROOT,sig=%d)=%d", signal, err );
    } else {
      if( kill( pip_root->pid, signal ) < 0 ) err = errno;
      DBGF( "kill(ROOT(PID=%d),sig=%d)=%d", pip_root->pid, signal, err );
    }
  } else {
    task = &pip_root->tasks[pipid];
    if( task->pipid != pipid ) RETURN( ENOENT );
    if( pip_if_pthread_() ) {
      err = pthread_kill( task->thread, signal );
      DBGF( "pthread_kill()=%d", err );
    } else {
      if( kill( task->pid, signal ) < 0 ) err = errno;
      DBGF( "kill(PIPID=%d(PID=%d),sig=%d)=%d",
	    task->pipid, task->pid, signal, err );
    }
  }
  RETURN( err );
}

int pip_exit( int retval ) {
  if( pip_root == NULL ) RETURN( EPERM );
  if( pip_root_p() && 0 ) {
    exit( retval );
  } else {
    pip_self->retval = retval;
    DBGF( "[PIPID=%d] pip_exit(%d)!!!", pip_self->pipid, retval );
    (void) setcontext( pip_self->ctx );
    DBGF( "[PIPID=%d] pip_exit() ????", pip_self->pipid );
  }
  /* never reach here */
  return 0;
}

/*
 * The following functions must be called at root process
 */

static void pip_finalize_thread( pip_task_t *task, int *retvalp ) {
  DBGF( "pipid=%d", task->pipid );

  /* dlclose() must only be called from the root process since */
  /* corresponding dlopen() calls are made by the root process */
  DBGF( "retval=%d", task->retval );
  //pip_print_maps();

  if( retvalp != NULL ) *retvalp = task->retval;

  if( task->loaded != NULL && dlclose( task->loaded ) != 0 ) {
    DBGF( "dlclose(): %s", dlerror() );
  }
  task->pipid  = PIP_PIPID_NONE;
  task->pid    = 0;
  task->retval = 0;
  task->loaded = NULL;
  task->thread = pip_root->thread;
  pip_root->ntasks_curr --;

  //pip_print_maps();
}

static int pip_check_join_arg( int pipid, pip_task_t **taskp ) {
  if( pipid     < 0                ) RETURN( EINVAL );
  if( pip_root == NULL             ) RETURN( EPERM  );
  if( pip_self != NULL             ) RETURN( EPERM  );
  if( pipid    >= pip_root->ntasks ) RETURN( ENOENT );
  if( !pthread_equal( pthread_self(), pip_root->thread ) ) {
    RETURN( EPERM );
  } else {
    pip_task_t *task = &pip_root->tasks[pipid];
    if( task->pipid != pipid ) RETURN( ENOENT );
    *taskp = task;
  }
  RETURN( 0 );
}

int pip_get_thread( int pipid, pthread_t *threadp ) {
  pip_task_t *task;
  int err;

  if( ( err = pip_check_join_arg( pipid, &task ) ) == 0 ) {
    if( !pip_if_pthread_() ) RETURN( ENOSYS );
    if( threadp != NULL ) *threadp = task->thread;
  }
  RETURN( err );
}

int pip_join( int pipid ) {
  pip_task_t *task;
  int err;

  if( ( err = pip_check_join_arg( pipid, &task ) ) == 0 ) {
    if( !pip_if_pthread_() ) RETURN( ENOSYS );
    err = pthread_join( task->thread, NULL );
    if( !err ) pip_finalize_thread( task, NULL );
  }
  RETURN( err );
}

int pip_tryjoin( int pipid ) {
  pip_task_t *task;
  int err;

  if( ( err = pip_check_join_arg( pipid, &task ) ) == 0 ) {
    if( !pip_if_pthread_() ) RETURN( ENOSYS );
    err = pthread_tryjoin_np( task->thread, NULL );
    if( !err ) pip_finalize_thread( task, NULL );
  }
  RETURN( err );
}

int pip_timedjoin( int pipid, struct timespec *time ) {
  pip_task_t *task;
  int err;

  if( ( err = pip_check_join_arg( pipid, &task ) ) == 0 ) {
    if( !pip_if_pthread_() ) RETURN( ENOSYS );
    err = pthread_timedjoin_np( task->thread, NULL, time );
    if( !err ) pip_finalize_thread( task, NULL );
  }
  RETURN( err );
}

int pip_wait( int pipid, int *retvalp ) {
  pip_task_t *task;
  int err;

  if( ( err = pip_check_join_arg( pipid, &task ) ) == 0 ) {
    if( pip_if_pthread_() ) { /* thread model */
      err = pthread_join( task->thread, NULL );
      DBGF( "pthread_join()=%d", err );

    } else { 	/* process model */
      int status;
      pid_t pid;
      DBG;
      while( 1 ) {
	if( ( pid = waitpid( task->pid, &status, 0 ) ) >= 0 ) break;
	if( errno != EINTR ) {
	  err = errno;
	  break;
	}
      }
      DBGF( "wait(status=%d)=%d (errno=%d)", status, pid, err );
    }
  }
  if( !err ) {
    pip_finalize_thread( task, retvalp );
  }
  RETURN( err );
}

int pip_trywait( int pipid, int *retvalp ) {
  pip_task_t *task;
  int err;

  if( ( err = pip_check_join_arg( pipid, &task ) ) == 0 ) {
    if( pip_if_pthread_() ) { /* thread model */
      err = pthread_tryjoin_np( task->thread, NULL );
      DBGF( "pthread_tryjoin_np()=%d", err );
    } else {			/* process model */
      int status;
      pid_t pid;
      DBG;
      while( 1 ) {
	if( ( pid = waitpid( task->pid, &status, WNOHANG ) ) >= 0 ) break;
	if( errno != EINTR ) {
	  err = errno;
	  break;
	}
      }
      DBGF( "wait(status=%d)=%d (errno=%d)", status, pid, err );
    }
  }
  if( !err ) {
    pip_finalize_thread( task, retvalp );
  }
  RETURN( err );
}

pip_clone_t *pip_get_cloneinfo( void ) {
  return pip_root->cloneinfo;
}

/*** some utility functions ***/

#include <link.h>

int pip_print_loaded_solibs( FILE *file ) {
  struct link_map *map;
  void *handle = NULL;
  char idstr[64];
  char *fname;

  /* pip_init() must be called in advance */
  if( pip_root == NULL ) RETURN( EPERM );

  if( pip_root_p() ) {
    handle = dlopen( NULL, RTLD_NOW );
    strcpy( idstr, "ROOT" );
  } else if( pip_self != NULL ) {
    handle = pip_self->loaded;
    sprintf( idstr, "%d", pip_self->pipid );
  }
  if( handle == NULL ) {
    fprintf( file, "[%s] (no solibs loaded)\n", idstr );
  } else {
    map = (struct link_map*) handle;
    for( ; map!=NULL; map=map->l_next ) {
      if( *map->l_name == '\0' ) {
	fname = "(noname)";
      } else {
	fname = map->l_name;
      }
      fprintf( file, "[PIP:%s] %s at %p\n", idstr, fname, (void*)map->l_addr );
    }
  }
  return 0;
}
