/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#define _GNU_SOURCE

#include <sys/wait.h>
#include <dlfcn.h>
#include <dirent.h>
#include <sched.h>
#include <pthread.h>
#include <malloc.h>
#include <signal.h>

//#define PIP_CLONE_AND_DLMOPEN
#define PIP_DLMOPEN_AND_CLONE

#if      defined(PIP_CLONE_AND_DLMOPEN) &&  defined(PIP_DLMOPEN_AND_CLONE)
#error  "defined(PIP_CLONE_AND_DLMOPEN) &&  defined(PIP_DLMOPEN_AND_CLONE)"
#elif   !defined(PIP_CLONE_AND_DLMOPEN) && !defined(PIP_DLMOPEN_AND_CLONE)
#error "!defined(PIP_CLONE_AND_DLMOPEN) && !defined(PIP_DLMOPEN_AND_CLONE)"
#endif

//#define SHOULD_HAVE_CTYPE_INIT
//#define HAVE_GLIBC_INIT
//#define PIP_EXPLICT_EXIT
//#define PIP_NO_MALLOPT

#define PIP_FREE(P)	free(P)

//#define DEBUG
//#define PRINT_MAPS
//#define PRINT_FDS

//#define EVAL

#include <pip.h>
#include <pip_internal.h>

#ifdef EVAL

#define ES(V,F)		\
  do { double __st=pip_gettime(); (F); (V) += pip_gettime()-__st; } while(0)
double time_dlmopen   = 0.0;
double time_load_so   = 0.0;
double time_load_prog = 0.0;
#define REPORT(V)	 printf( "%s: %g\n", #V, V );

#else

#define ES(V,F)		(F)
#define REPORT(V)

#endif

/*** note that the following static variables are   ***/
/*** located at each PIP task and the root process. ***/
static pip_root_t	*pip_root = NULL;
static pip_task_t	*pip_self = NULL;

static int (*pip_clone_mostly_pthread_ptr) (
	pthread_t *newthread,
	int clone_flags,
	int core_no,
	size_t stack_size,
	void *(*start_routine) (void *),
	void *arg,
	pid_t *pidp) = NULL;

static int pip_page_alloc( size_t sz, void **allocp ) {
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

static int pip_is_version_ok( pip_root_t *root ) {
  return root->version == PIP_VERSION;
}

static void pip_init_task_struct( pip_task_t *taskp ) {
  memset( (void*) taskp, 0, sizeof(pip_task_t) );
  taskp->pipid  = PIP_PIPID_NONE;
  taskp->thread = pip_root->thread;
}

#include <elf.h>

static int pip_check_pie( char *path ) {
  Elf64_Ehdr elfh;
  int fd;
  int err = 0;

  if( ( fd = open( path, O_RDONLY ) ) < 0 ) {
    err = errno;
  } else {
    if( read( fd, &elfh, sizeof( elfh ) ) != sizeof( elfh ) ) {
      fprintf( stderr, PIP_ERRMSG_TAG "Unable to read '%s'\n",
	       getpid(), path );
      err = EUNATCH;
    } else if( elfh.e_ident[EI_MAG0] != ELFMAG0 ||
	       elfh.e_ident[EI_MAG1] != ELFMAG1 ||
	       elfh.e_ident[EI_MAG2] != ELFMAG2 ||
	       elfh.e_ident[EI_MAG3] != ELFMAG3 ) {
      fprintf( stderr, "'%s' is not an ELF file\n", path );
      err = EUNATCH;
    } else if( elfh.e_type != ET_DYN ) {
      fprintf( stderr, PIP_ERRMSG_TAG "'%s' is not DYNAMIC (PIE)\n",
	       getpid(), path );
      err = ELIBEXEC;
    }
    (void) close( fd );
  }
  return err;
}

int pip_init( int *pipidp, int *ntasksp, void **rt_expp, int opts ) {
  pip_clone_t*	cloneinfo = NULL;
  size_t	sz;
  char		*env = NULL;
  int		ntasks;
  int 		pipid;
  int 		i;
  int 		err = 0;
  enum PIP_MODE_BITS {
    PIP_MODE_PTHREAD_BIT = 1,
    PIP_MODE_PROCESS_PRELOAD_BIT = 2,
    PIP_MODE_PROCESS_PIPCLONE_BIT = 4
  } desired = 0;

  if( pip_root != NULL ) RETURN( EBUSY ); /* already initialized */

  if( ( env = getenv( PIP_ROOT_ENV ) ) == NULL ) {
    /* root process ? */

    if( ntasksp == NULL ) {
      ntasks = PIP_NTASKS_MAX;
    } else if( *ntasksp <= 0 ) {
      RETURN( EINVAL );
    } else {
      ntasks = *ntasksp;
    }
    if( ntasks > PIP_NTASKS_MAX ) RETURN( EOVERFLOW );

    if( opts & PIP_MODE_PTHREAD &&
	opts & PIP_MODE_PROCESS ) RETURN( EINVAL );
    if( opts & PIP_MODE_PROCESS ) {
      if( ( opts & PIP_MODE_PROCESS_PRELOAD  ) == PIP_MODE_PROCESS_PRELOAD &&
	  ( opts & PIP_MODE_PROCESS_PIPCLONE ) == PIP_MODE_PROCESS_PIPCLONE){
	RETURN (EINVAL );
      }
    }

    switch( opts ) {
    case 0:
      env = getenv( PIP_ENV_MODE );
      if ( env == NULL ) {
	desired =
	  PIP_MODE_PTHREAD_BIT|
	  PIP_MODE_PROCESS_PRELOAD_BIT|
	  PIP_MODE_PROCESS_PIPCLONE_BIT;
      } else if( strcasecmp( env, PIP_ENV_MODE_THREAD  ) == 0 ||
		 strcasecmp( env, PIP_ENV_MODE_PTHREAD ) == 0 ) {
	desired = PIP_MODE_PTHREAD_BIT;
      } else if( strcasecmp( env, PIP_ENV_MODE_PROCESS ) == 0 ) {
	desired =
	  PIP_MODE_PROCESS_PRELOAD_BIT|
	  PIP_MODE_PROCESS_PIPCLONE_BIT;
      } else if( strcasecmp( env, PIP_ENV_MODE_PROCESS_PRELOAD  ) == 0 ) {
	desired = PIP_MODE_PROCESS_PRELOAD_BIT;
      } else if( strcasecmp( env, PIP_ENV_MODE_PROCESS_PIPCLONE ) == 0 ) {
	desired = PIP_MODE_PROCESS_PIPCLONE_BIT;
      }
      break;
    case PIP_MODE_PTHREAD:
      desired = PIP_MODE_PTHREAD_BIT;
      break;
    case PIP_MODE_PROCESS:
      env = getenv( PIP_ENV_MODE );
      if ( env == NULL ) {
	desired =
	  PIP_MODE_PROCESS_PRELOAD_BIT|
	  PIP_MODE_PROCESS_PIPCLONE_BIT;
      } else if( strcasecmp( env, PIP_ENV_MODE_PROCESS_PRELOAD  ) == 0 ) {
	desired = PIP_MODE_PROCESS_PRELOAD_BIT;
      } else if( strcasecmp( env, PIP_ENV_MODE_PROCESS_PIPCLONE ) == 0 ) {
	desired = PIP_MODE_PROCESS_PIPCLONE_BIT;
      } else {
	desired =
	  PIP_MODE_PROCESS_PRELOAD_BIT|
	  PIP_MODE_PROCESS_PIPCLONE_BIT;
      }
      break;
    case PIP_MODE_PROCESS_PRELOAD:
	desired = PIP_MODE_PROCESS_PRELOAD_BIT;
	break;
    case PIP_MODE_PROCESS_PIPCLONE:
	desired = PIP_MODE_PROCESS_PIPCLONE_BIT;
	break;
    }

    if( desired & PIP_MODE_PROCESS_PRELOAD_BIT ) {
      /* check if the __clone() systemcall wrapper exists or not */
      cloneinfo = (pip_clone_t*) dlsym( RTLD_DEFAULT, "pip_clone_info");
      DBGF( "cloneinfo-%p", cloneinfo );
      if( cloneinfo == NULL &&
	  !( desired & (PIP_MODE_PTHREAD_BIT|PIP_MODE_PROCESS_PIPCLONE_BIT) )
	  ) {
	/* no wrapper found */
	if( ( env = getenv( "LD_PRELOAD" ) ) == NULL ) {
	  fprintf( stderr,
		   PIP_ERRMSG_TAG "process mode is requested but "
		   "LD_PRELOAD environment variable is empty.\n",
		   getpid() );
	} else {
	  fprintf( stderr,
		   PIP_ERRMSG_TAG
		   "process mode is requested but LD_PRELOAD='%s'\n",
		   getpid(),
		   env );
	}
	RETURN( EPERM );
      }
      opts = PIP_MODE_PROCESS_PRELOAD;
    }
    if( desired & PIP_MODE_PROCESS_PIPCLONE_BIT ) {
      if ( pip_clone_mostly_pthread_ptr == NULL )
	pip_clone_mostly_pthread_ptr =
	  dlsym( RTLD_DEFAULT, "pip_clone_mostly_pthread" );
      if ( pip_clone_mostly_pthread_ptr == NULL &&
	   !( desired & PIP_MODE_PTHREAD_BIT) ) {
	fprintf( stderr,
		 PIP_ERRMSG_TAG
		 "process model is desired "
		 "but pip_clone_mostly_pthread() is not found\n",
		 getpid() );
	RETURN( EPERM );
      }
      opts = PIP_MODE_PROCESS_PIPCLONE;
    }

    if( desired & PIP_MODE_PTHREAD_BIT ) {
      opts = PIP_MODE_PTHREAD;
    } else {
      fprintf( stderr,
	       PIP_ERRMSG_TAG
	       "process model is desired "
	       "but both pip_clone_info and _pip_clone_mostly_pthread() "
	       "are not found\n",
	       getpid() );
      RETURN( EPERM );
    }

#ifdef DEBUG
    if( ( opts & PIP_MODE_PROCESS_PRELOAD ) == PIP_MODE_PROCESS_PRELOAD ) {
      DBGF( "(((( PROCESS MODE )))) LD_PRELOAD=%s", getenv( "LD_PRELOAD" ) );
    } else if((opts&PIP_MODE_PROCESS_PIPCLONE)==PIP_MODE_PROCESS_PIPCLONE ) {
      DBGF( "(((( PROCESS MODE : PIPCLONE ))))" );
    } else {
      DBGF( "[[[[ PTHREAD MODE ]]]] -------------------------" );
    }
#endif

    sz = sizeof( pip_root_t ) + sizeof( pip_task_t ) * ntasks;
    if( ( err = pip_page_alloc( sz, (void**) &pip_root ) ) != 0 ) {
      RETURN( err );
    }
    (void) memset( pip_root, 0, sz );
    pip_root->size = sz;

    DBGF( "ROOTROOT (%p)", pip_root );

    pip_spin_init( &pip_root->spawn_lock );

    if( asprintf( &env, "%s=%p", PIP_ROOT_ENV, pip_root ) == 0 ) {
      err = ENOMEM;
    } else {
      pipid = PIP_PIPID_ROOT;
      pip_set_magic( pip_root );
      pip_root->version      = PIP_VERSION;
      pip_root->ntasks       = ntasks;
      pip_root->thread       = pthread_self();
      pip_root->cloneinfo    = cloneinfo;
      pip_root->pip_root_env = env;
      pip_root->opts         = opts;
      pip_root->pid          = getpid();
      pip_root->free         = (free_t) dlsym( RTLD_DEFAULT, "free");
      if( rt_expp != NULL ) pip_root->export = *rt_expp;

      for( i=0; i<ntasks; i++ ) {
	pip_init_task_struct( &pip_root->tasks[i] );
      }
    }
    pip_self = NULL;
    if( err ) {		/* undo */
      if( pip_root != NULL ) PIP_FREE( pip_root );
      pip_root = NULL;
    }
  } else {
    /* child task */

    pip_root = (pip_root_t*) strtoll( env, NULL, 16 );
    if( !pip_is_magic_ok(   pip_root ) ) RETURN( EPERM );
    if( !pip_is_version_ok( pip_root ) ) {
      fprintf( stderr,
	       PIP_ERRMSG_TAG "Version miss-match between root and child\n",
	       getpid() );
      RETURN( EPERM );
    }

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
  if( pip_root->cloneinfo == NULL )
    return (pip_root->opts & PIP_MODE_PTHREAD) != 0 ? CLONE_THREAD : 0;
  return pip_root->cloneinfo->flag_clone & CLONE_THREAD;
}

int pip_if_pthread( int *flagp ) {
  if( pip_root == NULL ) RETURN( EPERM  );
  if( flagp    == NULL ) RETURN( EINVAL );
  *flagp = pip_if_pthread_();
  RETURN( 0 );
}

static int pip_if_shared_fd_( void ) {
  if( pip_root->cloneinfo == NULL )
    return (pip_root->opts & PIP_MODE_PTHREAD) != 0 ? CLONE_FILES : 0;
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
    *flagp = (pip_root->opts & PIP_MODE_PTHREAD) != 0 ? CLONE_SIGHAND : 0;
  } else {
    *flagp = pip_root->cloneinfo->flag_clone & CLONE_SIGHAND;
  }
  RETURN( 0 );
}

static int pip_root_p_( void ) {
  return pip_self == NULL && pip_root != NULL;
}

static int pip_task_p_( void ) {
  return pip_self != NULL;
}

static void *pip_get_dso_( void ) {
  if( pip_self != NULL ) return pip_self->loaded;
  return NULL;
}

int pip_get_pipid_( void ) {
  int pipid;
  if( pip_root == NULL ) {
    pipid = PIP_PIPID_ANY;
  } else if( pip_root_p_() ) {
    pipid = PIP_PIPID_ROOT;
  } else {
    pipid = pip_self->pipid;
  }
  return pipid;
}

int pip_get_pipid( int *pipidp ) {
  if( pipidp == NULL ) RETURN( EINVAL );
  *pipidp = pip_get_pipid_();
  RETURN( 0 );
}

int pip_get_ntasks( int *ntasksp ) {
  if( ntasksp  == NULL ) RETURN( EINVAL );
  if( pip_root == NULL ) return( EPERM  ); /* intentionally using return */

  *ntasksp = pip_root->ntasks_curr;
  RETURN( 0 );
}

int pip_export( void *export ) {
  if( export             == NULL ) RETURN( EINVAL );
  if( pip_root_p_() ) {
    if( pip_root->export != NULL ) RETURN( EBUSY  );
    pip_root->export = export;
  } else {
    if( pip_self->export != NULL ) RETURN( EBUSY  );
    pip_self->export = export;
  }
  RETURN( 0 );
}

static int pip_check_pipid( int *pipidp ) {
  int pipid = *pipidp;

  if( pip_root == NULL          ) RETURN( EPERM  );
  if( pipid >= pip_root->ntasks ) RETURN( ENOENT );
  if( pipid != PIP_PIPID_MYSELF &&
      pipid <  PIP_PIPID_ROOT   ) RETURN( EINVAL );
  if( pipid >= pip_root->ntasks ) RETURN( EINVAL );
  if( pipid == PIP_PIPID_MYSELF ) {
    if( pip_root_p_() ) {
      *pipidp = PIP_PIPID_ROOT;
    } else {
      if( pip_root->tasks[*pipidp].pipid != *pipidp ) RETURN( ENOENT );
      *pipidp = pip_self->pipid;
    }
  }
  return 0;
}

int pip_import( int pipid, void **exportp  ) {
  int err;

  if( exportp == NULL ) RETURN( EINVAL );
  if( ( err = pip_check_pipid( &pipid ) ) != 0 ) RETURN( err );

  if( pipid == PIP_PIPID_ROOT ) {
    *exportp = (void*) pip_root->export;
  } else {
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
  PIP_FREE( rootenv );
  return envdst;
}

static void pip_close_on_exec( void ) {
  DIR *dir;
  struct dirent *direntp;
  int fd;
  int flags;

#ifdef PRINT_FDS
  pip_print_fds();
#endif

#define PROCFD_PATH		"/proc/self/fd"
  if( ( dir = opendir( PROCFD_PATH ) ) != NULL ) {
    int fd_dir = dirfd( dir );
    while( ( direntp = readdir( dir ) ) != NULL ) {
      if( ( fd = atoi( direntp->d_name ) ) >= 0 &&
	  fd != fd_dir                          &&
	  ( flags = fcntl( fd, F_GETFD ) ) >= 0 &&
	  flags & FD_CLOEXEC ) {
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

static int pip_load_so( void **handlep, char *path ) {
  Lmid_t	lmid;
  int 		flags = RTLD_NOW | RTLD_LOCAL;
  /* RTLD_GLOBAL is NOT accepted and dlmopen() returns EINVAL */
  void 		*loaded;
  int		err;

  DBGF( "hanlde=%p", *handlep );
  if( *handlep == NULL ) {
    lmid = LM_ID_NEWLM;
  } else if( dlinfo( *handlep, RTLD_DI_LMID, (void*) &lmid ) != 0 ) {
    DBGF( "dlinfo(%p): %s", handlep, dlerror() );
    RETURN( ENXIO );
  }

  DBGF( "calling dlmopen(%s)", path );
  ES( time_dlmopen, ( loaded = dlmopen( lmid, path, flags ) ) );
  if( loaded == NULL ) {
    if( ( err = pip_check_pie( path ) ) != 0 ) RETURN( err );
    fprintf( stderr, PIP_ERRMSG_TAG "dlmopen(%s): %s\n",
	     getpid(), path, dlerror() );
    RETURN( ENOEXEC );
  } else {
    DBGF( "dlmopen(%s): SUCCEEDED", path );
    *handlep = loaded;
  }
  RETURN( 0 );
}

static int pip_load_prog( char *prog, pip_task_t *task ) {
  void		*loaded = NULL;
  char 		***envvp;
  main_func_t 	main_func;
#ifndef HAVE_GLIBC_INIT
  ctype_init_t	ctype_init;
#else
  glibc_init_t	glibc_init;
#endif
  fflush_t	libc_fflush;
  int 		err;

  DBGF( "prog=%s", prog );

#ifdef PRINT_MAPS
  pip_print_maps();
#endif
  ES( time_load_so, ( err = pip_load_so( &loaded, prog ) ) );
#ifdef PRINT_MAPS
  pip_print_maps();
#endif

  if( err == 0 ) {
    DBG;
    if( ( main_func = (main_func_t) dlsym( loaded, MAIN_FUNC ) ) == NULL ) {
	/* getting main function address to invoke */
      DBG;
      fprintf( stderr,
	       PIP_ERRMSG_TAG "%s seems not to be linked "
	       "with '-rdynamic' option\n",
	       getpid(), prog );
      err = ENOEXEC;
      goto error;
    } else if( ( libc_fflush = (fflush_t) dlsym( loaded, "fflush" ) )
	       == NULL ) {
      /* getting address of fflush function to flush messages */
      DBG;
      err = ENOEXEC;
      goto error;
    } else if( ( envvp = (char***) dlsym( loaded, "environ" ) ) == NULL ) {
      /* getting address of environmanet variable to be set */
      DBG;
      err = ENOEXEC;
      goto error;
    }
#ifndef HAVE_GLIBC_INIT
    if( ( ctype_init = (ctype_init_t) dlsym( loaded, "__ctype_init" ) )
	       == NULL ) {
#ifdef SHOULD_HAVE_CTYPE_INIT
      /* getting address of __ctype_init function to initialize ctype tables */
      DBG;
      err = ENOEXEC;
      goto error;
#else
      ctype_init = NULL;
    }
#endif
#else
    if( ( glibc_init = (glibc_init_t) dlsym( loaded, "glibc_init" ) ) == NULL ) {
      DBG;
      err = ENOEXEC;
      goto error;
    }
#endif
  }
 error:
  if( err == 0 ) {
#ifdef DEBUG
    pip_check_addr( "MAIN", main_func );
    pip_check_addr( "ENVP", envvp );
#endif
    task->mainf       = main_func;
#ifndef HAVE_GLIBC_INIT
    task->ctype_init  = ctype_init;
#else
    task->glibc_init  = glibc_init;
#endif
    task->libc_fflush = libc_fflush;
    task->free        = (free_t) dlsym( loaded, "free"   );
    task->envvp       = envvp;
    task->loaded      = loaded;
  } else if( loaded != NULL ) {
    (void) dlclose( loaded );
  }
  RETURN( err );
}

#ifdef PIP_DLMOPEN_AND_CLONE
static int pip_do_corebind( int coreno, cpu_set_t *oldsetp ) {
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
#endif

static int pip_corebind( int coreno ) {
  cpu_set_t cpuset;

  if( coreno != PIP_CPUCORE_ASIS ) {
    CPU_ZERO( &cpuset );
    CPU_SET( coreno, &cpuset );
    if( sched_setaffinity( 0, sizeof(cpuset), &cpuset ) != 0 ) {
      RETURN( errno );
    }
  }
  RETURN( 0 );
}

static int pip_do_spawn( void *thargs )  {
  pip_spawn_args_t *args = (pip_spawn_args_t*) thargs;
  int 	pipid      = args->pipid;
#ifdef PIP_CLONE_AND_DLMOPEN
  char *prog       = args->prog;
#endif
  char **argv      = args->argv;
  char **envv      = args->envv;
  int   coreno     = args->coreno;
  pip_spawnhook_t before = args->hook_before;
  pip_spawnhook_t after  = args->hook_after;
  void	*hook_arg  = args->hook_arg;
  pip_task_t *self = &pip_root->tasks[pipid];
  void *loaded	   = self->loaded;
  int err = 0;

  DBG;

  if( ( err = pip_corebind( coreno ) ) != 0 ) RETURN( err );

#ifdef DEBUG
  if( pip_if_pthread_() ) {
    pthread_attr_t attr;
    size_t sz;
    int _err;
    if( ( _err = pthread_getattr_np( self->thread, &attr    ) ) != 0 ) {
      DBGF( "pthread_getattr_np()=%d", _err );
    } else if( ( _err = pthread_attr_getstacksize( &attr, &sz ) ) != 0 ) {
      DBGF( "pthread_attr_getstacksize()=%d", _err );
    } else {
      DBGF( "stacksize = %ld [KiB]", sz/1024 );
    }
  }
#endif

  if( !pip_if_shared_fd_() ) pip_close_on_exec();

#ifdef PIP_CLONE_AND_DLMOPEN
  ES( time_load_prog, ( err = pip_load_prog( prog, self ) ) );
  if( err != 0 ) RETURN( err );
#endif

  /* calling hook, if any */
  if( before == NULL || ( err = before( hook_arg ) ) == 0 ) {
    /* argv and/or envv might be changed in the hook function */
    int   		argc;
    ucontext_t 		ctx;
    volatile int	flag_exit;	/* must be volatile */

    for( argc=0; argv[argc]!=NULL; argc++ );
    *self->envvp = envv;	/* setting environment vars */

    flag_exit = 0;
    (void) getcontext( &ctx );
    if( !flag_exit ) {
      flag_exit = 1;
      self->ctx = &ctx;

      {
	char ***argvp;
	int *argcp;

	if( ( argvp = (char***) dlsym( loaded, "__libc_argv" ) ) != NULL ) {
	  DBGF( "&__libc_argv=%p\n", argvp );
	  *argvp = argv;
	}
	if( ( argcp = (int*)    dlsym( loaded, "__libc_argc" ) ) != NULL ) {
	  DBGF( "&__libc_argc=argcp\n" );
	  *argcp = argc;
	}
      }
#ifndef PIP_NO_MALLOPT
      {
	int(*mallopt_func)(int,int);
	if( ( mallopt_func = dlsym( loaded, "mallopt" ) ) != NULL ) {
	  DBGF( ">> mallopt()" );
	  if(  mallopt_func( M_MMAP_THRESHOLD, 0 ) == 1 ) {
	    DBGF( "<< mallopt(M_MMAP_THRESHOLD): succeeded" );
	  } else {
	    DBGF( "<< mallopt(M_MMAP_THRESHOLD): failed !!!!!!" );
	  }
	  if(  mallopt_func( M_TRIM_THRESHOLD, -1 ) == 1 ) {
	    DBGF( "<< mallopt(M_TRIM_THRESHOLD): succeeded" );
	  } else {
	    DBGF( "<< mallopt(M_TRIM_THRESHOLD): failed !!!!!!" );
	  }
	}
      }
#endif

#ifndef HAVE_GLIBC_INIT
      /* __ctype_init() must be called at the very beginning of */
      /* process or theread. Since __libc_start_main is not     */
      /* called here, we have to call it explicitly             */
#ifdef SHOULD_HAVE_CTYPE_INIT
      if( self->ctype_init != NULL )
#endif
	{
	  DBGF( "[%d] >> __ctype_init@%p()", pipid, self->ctype_init );
	  self->ctype_init();
	  DBGF( "[%d] << __ctype_init@%p()", pipid, self->ctype_init );
	}
#else
      DBGF( "[%d] >> glibc_init@%p()", pipid, self->glibc_init );
      self->glibc_init( argc, argv, envv );
      DBGF( "[%d] << glibc_init@%p()", pipid, self->glibc_init );
#endif
      DBGF( "[%d] >> main@%p(%d,%s,%s,...)",
	    pipid, self->mainf, argc, argv[0], argv[1] );

      self->retval = self->mainf( argc, argv );

      DBGF( "[%d] << main@%p(%d,%s,%s,...)",
	    pipid, self->mainf, argc, argv[0], argv[1] );
      /* call fflush() in the target context to flush out messages */
      DBGF( "[%d] >> fflush@%p()", pipid, self->libc_fflush );
      self->libc_fflush( NULL );
      DBGF( "[%d] << fflush@%p()", pipid, self->libc_fflush );
      CHECK_CTYPE;

    } else {
      DBGF( "[%d] !! main(%d,%s,%s,...)", pipid, argc, argv[0], argv[1] );
    }
    if( after != NULL ) (void) after( hook_arg );

  } else if( err != 0 ) {
    fprintf( stderr,
	     PIP_ERRMSG_TAG
	     "try to spawn(%s), but the before hook at %p returns %d\n",
	     getpid(),
	     argv[0],
	     before,
	     err );
    self->retval = err;
  }

#ifdef PIP_EXPLICIT_EXIT
  if( pip_if_pthread_() ) {	/* thread mode */
    pthread_exit( NULL );
  } else {			/* process mode */
    exit( self->retval );
  }
#endif

  RETURN( 0 );
}

static void pip_clone_wrap_begin( void ) {
  if( pip_root->cloneinfo != NULL &&
      pip_root->opts & PIP_MODE_PROCESS_PRELOAD ) {
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
  extern char 		**environ;
  pthread_attr_t 	attr;
  cpu_set_t 		cpuset;
  size_t		stack_size = 0;
  pip_spawn_args_t	*args = NULL;
  pip_task_t		*task = NULL;
  int 			pipid;
  pid_t			pid = 0;
  int 			err = 0;

  DBGF( ">> pip_spawn()" );

  if( pip_root == NULL ) RETURN( EPERM );
  if( prog == NULL ) {
    if( argv == NULL )   RETURN( EINVAL );
    prog = argv[0];
  }
  if( pipidp   == NULL ) RETURN( EINVAL );
  pipid = *pipidp;
  if( pipid < PIP_PIPID_ANY || pipid >= pip_root->ntasks ) {
    DBGF( "pipid=%d", pipid );
    RETURN( EINVAL );
  }

  DBGF( "pip_spawn(pipid=%d)", *pipidp );

  if( pip_root->ntasks_accum >= PIP_NTASKS_MAX ) RETURN( EOVERFLOW );
#ifdef PIP_NO_TASKSPAWN
  if( !pthread_equal( pthread_self(), pip_root->thread ) ) {
    RETURN( EPERM );
  }
#endif
  if( envv == NULL ) envv = environ;

  if( ( err = pthread_attr_init( &attr ) ) != 0 ) RETURN( err );

#ifdef PIP_CLONE_AND_DLMOPEN
  if( coreno != PIP_CPUCORE_ASIS ) {
    CPU_ZERO( &cpuset );
    CPU_SET( coreno, &cpuset );
    err = pthread_attr_setaffinity_np( &attr, sizeof(cpuset), &cpuset );
    if( err != 0 ) RETURN( err );
  }
#endif

  {
    char *env;
    size_t sz;

    if( ( env = getenv( PIP_ENV_STACKSZ ) ) != NULL &&
	( sz = (size_t) strtol( env, NULL, 10 ) ) >  0 ) {
      stack_size = sz; /* for pip_clone() */
      err = pthread_attr_setstacksize( &attr, sz );
      DBGF( "pthread_attr_setstacksize( %ld )= %d", sz, err );
    }
  }

  args = (pip_spawn_args_t*) malloc( sizeof(pip_spawn_args_t) );
  if( args == NULL ) RETURN( ENOMEM );

  args->prog        = strdup( prog );
  if( args->prog == NULL ) RETURN( ENOMEM );
  args->coreno      = coreno;
  args->hook_before = before;
  args->hook_after  = after;
  args->hook_arg    = hookarg;
  if( ( args->argv = pip_copy_vec( NULL, argv ) ) == NULL ) {
    PIP_FREE( args->prog );
    PIP_FREE( args );
    RETURN( ENOMEM );
  }
  if( ( args->envv = pip_copy_env( envv ) ) == NULL ) {
    PIP_FREE( args->prog );
    PIP_FREE( args->argv );
    PIP_FREE( args );
    RETURN( ENOMEM );
  }

  pip_spin_lock( &pip_root->spawn_lock );
  /*** begin lock region ***/
  do {
    if( pipid != PIP_PIPID_ANY ) {
      if( pip_root->tasks[pipid].pipid != PIP_PIPID_NONE ) {
	DBG;
	err = EAGAIN;
	goto unlock;
      }
    } else {
      int i;

      for( i=pip_root->pipid_curr; i<pip_root->ntasks; i++ ) {
	if( pip_root->tasks[i].pipid == PIP_PIPID_NONE ) {
	  pipid = i;
	  goto found;
	}
      }
      for( i=0; i<pip_root->pipid_curr; i++ ) {
	if( pip_root->tasks[i].pipid == PIP_PIPID_NONE ) {
	  pipid = i;
	  goto found;
	}
      }
      err = EAGAIN;
      goto unlock;
    }
  found:
    pip_root->pipid_curr = pipid + 1;
    task = &pip_root->tasks[pipid];
    pip_init_task_struct( task );
    task->pipid = pipid;	/* mark it as occupied */
    args->pipid = pipid;
  } while( 0 );
  /*** end lock region ***/
 unlock:
  pip_spin_unlock( &pip_root->spawn_lock );

  /**** beyond this point, we can 'goto' the 'error' label ****/

  if( err != 0 ) goto error;

#ifdef PIP_DLMOPEN_AND_CLONE
  {
    if( ( err = pip_do_corebind( coreno, &cpuset ) ) == 0 ) {
      /* corebinding should take place before loading solibs,        */
      /* hoping anon maps would be mapped ontto the closer numa node */

      ES( time_load_prog, ( err = pip_load_prog( prog, task ) ) );

      /* and of course, the corebinding must be undone */
      (void) pip_undo_corebind( coreno, &cpuset );
    }
    if( err != 0 ) goto error;
  }
#endif

  pip_clone_wrap_begin();   /* tell clone wrapper (preload), if any */
  /* FIXME: there is a race condition between the above and below */
  if( ( pip_root->opts & PIP_MODE_PROCESS_PIPCLONE ) ==
      PIP_MODE_PROCESS_PIPCLONE ) {
    err = pip_clone_mostly_pthread_ptr(
		&task->thread,
		CLONE_VM |
		/* CLONE_FS | CLONE_FILES | */ /* for pip */
		/* CLONE_SIGHAND | CLONE_THREAD | */ /* for pip */
		CLONE_SETTLS |
		CLONE_PARENT_SETTID |
		CLONE_CHILD_CLEARTID |
		CLONE_SYSVSEM |
		CLONE_PTRACE | /* for pip */
		SIGCHLD /* for pip */,
		coreno, stack_size,
		(void*(*)(void*)) pip_do_spawn, args, &pid );
  } else {
    task->args = args;
    err = pthread_create( &task->thread,
			  &attr,
			  (void*(*)(void*)) pip_do_spawn,
			  (void*) args );
    if( pip_root->cloneinfo != NULL ) {
      pid = pip_root->cloneinfo->pid_clone;
    }
  }

  DBG;
  if( err == 0 ) {
    *pipidp = pipid;
    task->pid = pid;
    pip_root->ntasks_accum ++;
    pip_root->ntasks_curr  ++;
    if( pip_root->cloneinfo != NULL ) {
      pip_root->cloneinfo->pid_clone  = 0;
    }
  } else {
  error:			/* undo */
    DBGF( "err=%d", err );
    PIP_FREE( args->prog );
    PIP_FREE( args->argv );
    PIP_FREE( args->envv );
    PIP_FREE( args );
    if( task != NULL ) pip_init_task_struct( task );
  }
  DBGF( "<< pip_spawn(pipid=%d)", *pipidp );
  RETURN( err );
}

int pip_fin( void ) {
  int ntasks;
  int i;
  int err = 0;

  fflush( NULL );
  if( pip_root_p_() ) {
    ntasks = pip_root->ntasks;
    for( i=0; i<ntasks; i++ ) {
      if( pip_root->tasks[i].pipid != PIP_PIPID_NONE ) {
	DBGF( "%d/%d [%d] -- BUSY", i, ntasks, pip_root->tasks[i].pipid );
	err = EBUSY;
	break;
      }
    }
    if( err == 0 ) {
      if( pip_root->pip_root_env != NULL ) {
	free( pip_root->pip_root_env );
      }
      memset( pip_root, 0, pip_root->size );
      free( pip_root );
    }
  }
  pip_root = NULL;

  REPORT( time_load_so   );
  REPORT( time_load_prog );
  REPORT( time_dlmopen   );

  RETURN( err );
}

int pip_get_mode( int *mode ) {
  if( pip_root == NULL ) RETURN( EINVAL );
  if( mode     == NULL ) RETURN( EINVAL );
  *mode = ( pip_root->opts & PIP_MODE_MASK );
  RETURN( 0 );
}

int pip_get_pid( int pipid, intptr_t *pidp ) {
  int err;

  if( ( err = pip_check_pipid( &pipid ) ) != 0 ) RETURN( err );
  if( pidp == NULL )      RETURN( EINVAL );
  if( pip_if_pthread_() ) {
    if( pipid == PIP_PIPID_ROOT ) {
      *pidp = (intptr_t) pip_root->thread;
    } else {
      *pidp = (intptr_t) pip_root->tasks[pipid].pid;
    }
  } else {
    if( pipid == PIP_PIPID_ROOT ) {
      *pidp = (intptr_t) pip_root->pid;
    } else {
      *pidp = (intptr_t) pip_root->tasks[pipid].pid;
    }
  }
  RETURN( 0 );
}

int pip_kill( int pipid, int signal ) {
  int err  = 0;

  if( ( err = pip_check_pipid( &pipid ) ) != 0 ) RETURN( err );
  if( signal < 0 ) RETURN( EINVAL );

  if( pipid == PIP_PIPID_ROOT ) {
    if( pip_if_pthread_() ) {
      err = pthread_kill( pip_root->thread, signal );
      DBGF( "pthread_kill(ROOT,sig=%d)=%d", signal, err );
    } else {
      if( kill( pip_root->pid, signal ) < 0 ) err = errno;
      DBGF( "kill(ROOT(PID=%d),sig=%d)=%d", pip_root->pid, signal, err );
    }
  } else {
    pip_task_t *task = &pip_root->tasks[pipid];

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
  if( !pip_root_p_() && !pip_task_p_() ) {
    /* since we must replace exit() with pip_exit(), pip_exit() */
    /* must be able to use even if it is NOT a PIP environment. */
    exit( retval );
  } else if( pip_if_pthread_() ) {	/* thread mode */
    pip_self->retval = retval;
    DBGF( "[PIPID=%d] pip_exit(%d)!!!", pip_self->pipid, retval );
    (void) setcontext( pip_self->ctx );
    DBGF( "[PIPID=%d] pip_exit() ????", pip_self->pipid );
  } else {				/* process mode */
    exit( retval );
  }
  /* never reach here */
  return 0;
}

/*
 * The following functions must be called at root process
 */

static void pip_finalize_task( pip_task_t *task, int *retvalp ) {
  DBGF( "pipid=%d", task->pipid );

  /* dlclose() must only be called from the root process since */
  /* corresponding dlopen() calls are made by the root process */
  DBGF( "retval=%d", task->retval );

  if( retvalp != NULL ) *retvalp = task->retval;

  if( task->loaded != NULL && dlclose( task->loaded ) != 0 ) {
    DBGF( "dlclose(): %s", dlerror() );
  }

  if( task->args != NULL ) {
    if( task->args->prog != NULL ) PIP_FREE( task->args->prog );
    if( task->args->argv != NULL ) PIP_FREE( task->args->argv );
    if( task->args->envv != NULL ) PIP_FREE( task->args->envv );
    PIP_FREE( task->args );
  }
  pip_init_task_struct( task );
  pip_root->ntasks_curr --;
}

#ifdef PIP_PTHREAD
static int pip_check_join_arg( int pipid, pip_task_t **taskp ) {
  int err;

  if( ( err = pip_check_pipid( &pipid ) ) != 0 ) RETURN( err );
  if( pipid == PIP_PIPID_ROOT ) RETURN( EINVAL );
  if( pip_self != NULL ) RETURN( EPERM );
  if( !pthread_equal( pthread_self(), pip_root->thread ) ) RETURN( EPERM );
  *taskp = &pip_root->tasks[pipid];
  if( *taskp == pip_self ) RETURN( EPERM );

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
    if( !err ) pip_finalize_task( task, NULL );
  }
  RETURN( err );
}

int pip_tryjoin( int pipid ) {
  pip_task_t *task;
  int err;

  if( ( err = pip_check_join_arg( pipid, &task ) ) == 0 ) {
    if( !pip_if_pthread_() ) RETURN( ENOSYS );
    err = pthread_tryjoin_np( task->thread, NULL );
    if( !err ) pip_finalize_task( task, NULL );
  }
  RETURN( err );
}

int pip_timedjoin( int pipid, struct timespec *time ) {
  pip_task_t *task;
  int err;

  if( ( err = pip_check_join_arg( pipid, &task ) ) == 0 ) {
    if( !pip_if_pthread_() ) RETURN( ENOSYS );
    err = pthread_timedjoin_np( task->thread, NULL, time );
    if( !err ) pip_finalize_task( task, NULL );
  }
  RETURN( err );
}
#endif

int pip_wait( int pipid, int *retvalp ) {
  pip_task_t *task;
  int err;

  if( ( err = pip_check_pipid( &pipid ) ) != 0 ) RETURN( err );
  if( pipid == PIP_PIPID_ROOT  ) RETURN( EINVAL );
  task = &pip_root->tasks[pipid];
  if( task == pip_self ) RETURN( EINVAL );

  if( pip_if_pthread_() ) { /* thread mode */
    err = pthread_join( task->thread, NULL );
    DBGF( "pthread_join()=%d", err );

  } else { 	/* process mode */
    int status;
    pid_t pid;
    DBG;
    while( 1 ) {
      if( ( pid = waitpid( task->pid, &status, 0 ) ) >= 0 ) break;
      if( errno != EINTR && errno != EDEADLK ) {
	err = errno;
	break;
      }
    }
    if( WIFEXITED( status ) ) {
      task->retval = WEXITSTATUS( status );
    }
    DBGF( "wait(status=%x)=%d (errno=%d)", status, pid, err );
  }
  if( !err ) pip_finalize_task( task, retvalp );
  RETURN( err );
}

int pip_trywait( int pipid, int *retvalp ) {

  pip_task_t *task;
  int err;

  if( ( err = pip_check_pipid( &pipid ) ) != 0 ) RETURN( err );
  if( pipid == PIP_PIPID_ROOT  ) RETURN( EINVAL );
  task = &pip_root->tasks[pipid];
  if( task == pip_self ) RETURN (EINVAL );

  if( pip_if_pthread_() ) { /* thread mode */
    err = pthread_tryjoin_np( task->thread, NULL );
    DBGF( "pthread_tryjoin_np()=%d", err );
  } else {			/* process mode */
    int status = 0;
    pid_t pid;
    DBG;
    while( 1 ) {
      if( ( pid = waitpid( task->pid, &status, WNOHANG ) ) >= 0 ) break;
      if( errno != EINTR ) {
	err = errno;
	break;
      }
    }
    if( WIFEXITED( status ) ) {
      task->retval = WEXITSTATUS( status );
    }
    DBGF( "wait(status=%x)=%d (errno=%d)", status, pid, err );
  }
  if( !err ) pip_finalize_task( task, retvalp );
  RETURN( err );
}

pip_clone_t *pip_get_cloneinfo_( void ) {
  return pip_root->cloneinfo;
}

/*** The following malloc/free functions are just for functional test    ***/
/*** We should hvae the other functions allocating memory doing the same ***/

/* long long to align */
#define PIP_ALIGN_TYPE	long

void *pip_malloc( size_t size ) {
  void *p = malloc( size + sizeof(PIP_ALIGN_TYPE) );
  int pipid;
  if( pip_get_pipid( &pipid ) == 0 ) {
    *(int*) p = pipid;
    p += sizeof(PIP_ALIGN_TYPE);
  } else {
    free( p );
    p = NULL;
  }
  return p;
}

void pip_free( void *ptr ) {
  int pipid;
  free_t free_func;
  ptr  -= sizeof(PIP_ALIGN_TYPE);
  pipid = *(int*) ptr;
  /* need of sanity check on pipid */
  if( pipid == PIP_PIPID_ROOT ) {
    free_func = pip_root->free;
  } else {
    free_func = pip_root->tasks[pipid].free;
  }
  if( free_func == NULL ) {
    fprintf( stderr, "No free function (pipid=%d)\n", pipid );
  } else {
    free_func( ptr );
  }
}

void pip_print_loaded_solibs( FILE *file ) {
  static void *root_handle = NULL;
  struct link_map *map;
  void *handle = NULL;
  char idstr[64];
  char *fname;

  /* pip_init() must be called in advance */
  if( pip_root != NULL ) {
    if( pip_root_p_() ) {
      if( root_handle == NULL ) root_handle = dlopen( NULL, RTLD_NOW );
      handle = root_handle;
      strcpy( idstr, "ROOT" );
    } else {
      handle = pip_get_dso_();
      sprintf( idstr, "%d", pip_self->pipid );
    }
  }
  if( handle == NULL ) {
    fprintf( file, "[%s] (no solibs loaded)\n", idstr );
  } else {
    if( file == NULL ) file = stderr;
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
}
