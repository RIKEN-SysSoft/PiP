/*
 * $PIP_license: <Simplified BSD License>
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *     Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 * $
 * $RIKEN_copyright: Riken Center for Computational Sceience (R-CCS),
 * System Software Development Team, 2016-2020
 * $
 * $PIP_VERSION: Version 2.0.0$
 *
 * $Author: Atsushi Hori (R-CCS) mailto: ahori@riken.jp or ahori@me.com
 * $
 */

#include <pip/pip_internal.h>
#include <pip/pip.h>

#include <sys/mman.h>
#include <sys/wait.h>
#include <sched.h>
#include <malloc.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/prctl.h>

//#define PIP_NO_MALLOPT

//#define DEBUG
//#define PRINT_MAPS
//#define PRINT_FDS

/* the EVAL env. is to measure the time for calling dlmopen() */
//#define EVAL

#define PIP_INTERNAL_FUNCS
#include <pip/pip.h>
#include <pip/pip_util.h>
#include <pip/pip_gdbif.h>

extern char 		**environ;

/*** note that the following static variables are   ***/
/*** located at each PIP task and the root process. ***/
pip_root_t		*pip_root = NULL;
pip_task_t		*pip_task = NULL;

static pip_clone_t*	pip_cloneinfo = NULL;
pip_spinlock_t *pip_lock_clone PIP_PRIVATE;

static int (*pip_clone_mostly_pthread_ptr) (
	pthread_t *newthread,
	int clone_flags,
	int core_no,
	size_t stack_size,
	void *(*start_routine) (void *),
	void *arg,
	pid_t *pidp) = NULL;

struct pip_gdbif_root	*pip_gdbif_root;

int pip_root_p_( void ) {
  return pip_root != NULL && pip_task != NULL &&
    pip_root->task_root == pip_task;
}

int pip_is_pthread_( void ) {
  return (pip_root->opts & PIP_MODE_PTHREAD) != 0 ? CLONE_THREAD : 0;
}

static void pip_set_name( char *symbol, char *progname ) {
#ifdef PR_SET_NAME
  /* the following code is to set the right */
  /* name shown by the ps and top commands  */
  char nam[16];

  if( progname == NULL ) {
    char prg[16];
    prctl( PR_GET_NAME, prg, 0, 0, 0 );
    snprintf( nam, 16, "%s%s", symbol, prg );
  } else {
    char *p;
    if( ( p = strrchr( progname, '/' ) ) != NULL) {
      progname = p + 1;
    }
    snprintf( nam, 16, "%s%s", symbol, progname );
  }
  if( !pip_is_pthread_() ) {
#define FMT "/proc/self/task/%u/comm"
    char fname[sizeof(FMT)+8];
    int fd;

    (void) prctl( PR_SET_NAME, nam, 0, 0, 0 );
    sprintf( fname, FMT, (unsigned int) pip_gettid() );
    if( ( fd = open( fname, O_RDWR ) ) >= 0 ) {
      (void) write( fd, nam, strlen(nam) );
      (void) close( fd );
    }
  } else {
    (void) pthread_setname_np( pthread_self(), nam );
  }
#endif
}

static char pip_cmd_name_symbol( int opts ) {
  char sym;

  switch( opts & PIP_MODE_MASK ) {
  case PIP_MODE_PROCESS_PRELOAD:
    sym = ':';
    break;
  case PIP_MODE_PROCESS_PIPCLONE:
    sym = ';';
    break;
  case PIP_MODE_PROCESS_GOT:
    sym = '.';
    break;
  case PIP_MODE_PTHREAD:
    sym = '|';
    break;
  default:
    sym = '?';
    break;
  }
  return sym;
}

static int pip_count_vec( char **vecsrc ) {
  int n;
  for( n=0; vecsrc[n]!= NULL; n++ );
  return( n );
}

static void pip_set_magic( pip_root_t *root ) {
  memcpy( root->magic, PIP_MAGIC_WORD, PIP_MAGIC_WLEN );
}

void pip_reset_task_struct( pip_task_t *taskp ) {
  void	*namexp = taskp->named_exptab;
  memset( (void*) taskp, 0, sizeof(pip_task_t) );
  taskp->pipid = PIP_PIPID_NONE;
  taskp->type  = PIP_TYPE_NULL;
  taskp->named_exptab = namexp;
}

#include <elf.h>

int pip_check_pie( const char *path, int flag_verbose ) {
  struct stat stbuf;
  Elf64_Ehdr elfh;
  int fd;
  int err = 0;

  if( strchr( path, '/' ) == NULL ) {
    if( flag_verbose ) {
      pip_err_mesg( "'%s' is not a path (no slash '/')", path );
    }
    err = ENOENT;
  } else if( ( fd = open( path, O_RDONLY ) ) < 0 ) {
    err = errno;
    if( flag_verbose ) {
      pip_err_mesg( "'%s': open() fails (%s)", path, strerror( errno ) );
    }
  } else {
    if( fstat( fd, &stbuf ) < 0 ) {
      err = errno;
      if( flag_verbose ) {
	pip_err_mesg( "'%s': stat() fails (%s)", path, strerror( errno ) );
      }
    } else if( ( stbuf.st_mode & (S_IXUSR|S_IXGRP|S_IXOTH) ) == 0 ) {
      if( flag_verbose ) {
	pip_err_mesg( "'%s' is not executable", path );
      }
      err = EACCES;
    } else if( read( fd, &elfh, sizeof( elfh ) ) != sizeof( elfh ) ) {
      if( flag_verbose ) {
	pip_err_mesg( "Unable to read '%s'", path );
      }
      err = EUNATCH;
    } else if( elfh.e_ident[EI_MAG0] != ELFMAG0 ||
	       elfh.e_ident[EI_MAG1] != ELFMAG1 ||
	       elfh.e_ident[EI_MAG2] != ELFMAG2 ||
	       elfh.e_ident[EI_MAG3] != ELFMAG3 ) {
      if( flag_verbose ) {
	pip_err_mesg( "'%s' is not ELF", path );
      }
      err = ELIBBAD;
    } else if( elfh.e_type != ET_DYN ) {
      if( flag_verbose ) {
	pip_err_mesg( "'%s' is not PIE", path );
      }
      err = ELIBEXEC;
    }
    (void) close( fd );
  }
  return err;
}

const char *pip_get_mode_str( void ) {
  char *mode;

  if( pip_root == NULL ) return NULL;
  switch( pip_root->opts & PIP_MODE_MASK ) {
  case PIP_MODE_PTHREAD:
    mode = PIP_ENV_MODE_PTHREAD;
    break;
  case PIP_MODE_PROCESS:
    mode = PIP_ENV_MODE_PROCESS;
    break;
  case PIP_MODE_PROCESS_PRELOAD:
    mode = PIP_ENV_MODE_PROCESS_PRELOAD;
    break;
  case PIP_MODE_PROCESS_GOT:
    mode = PIP_ENV_MODE_PROCESS_GOT;
    break;
  case PIP_MODE_PROCESS_PIPCLONE:
    mode = PIP_ENV_MODE_PROCESS_PIPCLONE;
    break;
  default:
    mode = "(unknown)";
  }
  return mode;
}

static int pip_check_opt_and_env( int *optsp ) {
  extern pip_spinlock_t	pip_lock_got_clone;
  int opts   = *optsp;
  int mode   = ( opts & PIP_MODE_MASK );
  int newmod = 0;
  char *env  = getenv( PIP_ENV_MODE );

  enum PIP_MODE_BITS {
    PIP_MODE_PTHREAD_BIT          = 1,
    PIP_MODE_PROCESS_PRELOAD_BIT  = 2,
    PIP_MODE_PROCESS_GOT_BIT      = 4,
    PIP_MODE_PROCESS_PIPCLONE_BIT = 8
  } desired = 0;

  if( ( opts & ~PIP_VALID_OPTS ) != 0 ) {
    /* unknown option(s) specified */
    RETURN( EINVAL );
  }

  if( mode & PIP_MODE_PTHREAD &&
      mode & PIP_MODE_PROCESS ) RETURN( EINVAL );
  if( mode & PIP_MODE_PROCESS ) {
    if( mode != PIP_MODE_PROCESS_PRELOAD &&
	mode != PIP_MODE_PROCESS_GOT     &&
	mode != PIP_MODE_PROCESS_PIPCLONE ) {
      DBGF( "mode:0x%x", mode );
      RETURN (EINVAL );
    }
  }

  switch( mode ) {
  case 0:
    if( env == NULL || env[0] == '\0' ) {
      desired =
	PIP_MODE_PTHREAD_BIT 	     |
	PIP_MODE_PROCESS_PRELOAD_BIT |
	PIP_MODE_PROCESS_GOT_BIT     |
	PIP_MODE_PROCESS_PIPCLONE_BIT;
    } else if( strcasecmp( env, PIP_ENV_MODE_THREAD  ) == 0 ||
	       strcasecmp( env, PIP_ENV_MODE_PTHREAD ) == 0 ) {
      desired = PIP_MODE_PTHREAD_BIT;
    } else if( strcasecmp( env, PIP_ENV_MODE_PROCESS ) == 0 ) {
      desired =
	PIP_MODE_PROCESS_PRELOAD_BIT |
	PIP_MODE_PROCESS_GOT_BIT     |
	PIP_MODE_PROCESS_PIPCLONE_BIT;
    } else if( strcasecmp( env, PIP_ENV_MODE_PROCESS_PRELOAD  ) == 0 ) {
      desired = PIP_MODE_PROCESS_PRELOAD_BIT;
    } else if( strcasecmp( env, PIP_ENV_MODE_PROCESS_GOT  ) == 0 ) {
      desired = PIP_MODE_PROCESS_GOT_BIT;
    } else if( strcasecmp( env, PIP_ENV_MODE_PROCESS_PIPCLONE ) == 0 ) {
      desired = PIP_MODE_PROCESS_PIPCLONE_BIT;
    } else {
      pip_warn_mesg( "unknown environment setting PIP_MODE='%s'", env );
      RETURN( EOPNOTSUPP );
    }
    break;
  case PIP_MODE_PTHREAD:
    desired = PIP_MODE_PTHREAD_BIT;
    break;
  case PIP_MODE_PROCESS:
    if ( env == NULL ) {
      desired =
	PIP_MODE_PROCESS_PRELOAD_BIT |
	PIP_MODE_PROCESS_GOT_BIT     |
	PIP_MODE_PROCESS_PIPCLONE_BIT;
    } else if( strcasecmp( env, PIP_ENV_MODE_PROCESS_PRELOAD  ) == 0 ) {
      desired = PIP_MODE_PROCESS_PRELOAD_BIT;
    } else if( strcasecmp( env, PIP_ENV_MODE_PROCESS_GOT      ) == 0 ) {
      desired = PIP_MODE_PROCESS_GOT_BIT;
    } else if( strcasecmp( env, PIP_ENV_MODE_PROCESS_PIPCLONE ) == 0 ) {
      desired = PIP_MODE_PROCESS_PIPCLONE_BIT;
    } else if( strcasecmp( env, PIP_ENV_MODE_THREAD  ) == 0 ||
	       strcasecmp( env, PIP_ENV_MODE_PTHREAD ) == 0 ||
	       strcasecmp( env, PIP_ENV_MODE_PROCESS ) == 0 ) {
      /* ignore PIP_MODE=thread in this case */
      desired =
	PIP_MODE_PROCESS_PRELOAD_BIT |
	PIP_MODE_PROCESS_GOT_BIT     |
	PIP_MODE_PROCESS_PIPCLONE_BIT;
    } else {
      pip_warn_mesg( "unknown environment setting PIP_MODE='%s'", env );
      RETURN( EPERM );
    }
    break;
  case PIP_MODE_PROCESS_PRELOAD:
    desired = PIP_MODE_PROCESS_PRELOAD_BIT;
    break;
  case PIP_MODE_PROCESS_GOT:
    desired = PIP_MODE_PROCESS_GOT_BIT;
    break;
  case PIP_MODE_PROCESS_PIPCLONE:
    desired = PIP_MODE_PROCESS_PIPCLONE_BIT;
    break;
  default:
    pip_warn_mesg( "pip_init() invalid argument opts=0x%x", opts );
    RETURN( EINVAL );
  }

  if( desired & PIP_MODE_PROCESS_GOT_BIT ) {
    int pip_wrap_clone( void );
    /* check if the __clone() systemcall wrapper exists or not */
    if( pip_wrap_clone() == 0 ) {
      newmod = PIP_MODE_PROCESS_GOT;
      pip_lock_clone = &pip_lock_got_clone;
      goto done;
    } else if( !( desired & ( PIP_MODE_PTHREAD_BIT         |
			      PIP_MODE_PROCESS_PRELOAD_BIT |
			      PIP_MODE_PROCESS_PIPCLONE_BIT ) ) ) {
      /* no wrapper found */
      if( ( env = getenv( "LD_PRELOAD" ) ) == NULL ) {
	pip_warn_mesg( "process:preload mode is requested but "
		       "LD_PRELOAD environment variable is empty." );
      } else {
	pip_warn_mesg( "process:preload mode is requested but "
		       "LD_PRELOAD='%s'",
		       env );
      }
      RETURN( EPERM );
    }
  }
  if( desired & PIP_MODE_PROCESS_PRELOAD_BIT ) {
    /* check if the __clone() systemcall wrapper exists or not */
    if( pip_cloneinfo == NULL ) {
      pip_cloneinfo = (pip_clone_t*) dlsym( RTLD_DEFAULT, "pip_clone_info");
    }
    DBGF( "cloneinfo-%p", pip_cloneinfo );
    if( pip_cloneinfo != NULL ) {
      newmod = PIP_MODE_PROCESS_PRELOAD;
      pip_lock_clone = &pip_cloneinfo->lock;
      goto done;
    } else if( !( desired & ( PIP_MODE_PTHREAD_BIT |
			      PIP_MODE_PROCESS_PIPCLONE_BIT ) ) ) {
      /* no wrapper found */
      if( ( env = getenv( "LD_PRELOAD" ) ) == NULL ) {
	pip_warn_mesg( "process:preload mode is requested but "
		       "LD_PRELOAD environment variable is empty." );
      } else {
	pip_warn_mesg( "process:preload mode is requested but "
		       "LD_PRELOAD='%s'",
		       env );
      }
      RETURN( EPERM );
    }
  }
  if( desired & PIP_MODE_PROCESS_PIPCLONE_BIT ) {
    if ( pip_clone_mostly_pthread_ptr == NULL )
      pip_clone_mostly_pthread_ptr =
	dlsym( RTLD_DEFAULT, "pip_clone_mostly_pthread" );
    if ( pip_clone_mostly_pthread_ptr != NULL ) {
      newmod = PIP_MODE_PROCESS_PIPCLONE;
      goto done;
    } else if( !( desired & PIP_MODE_PTHREAD_BIT) ) {
      if( desired & PIP_MODE_PROCESS_PRELOAD_BIT ) {
	pip_warn_mesg("process mode is requested but pip_clone_info symbol "
		      "is not found in $LD_PRELOAD and "
		      "pip_clone_mostly_pthread() symbol is not found in "
		      "glibc" );
      } else {
	pip_warn_mesg( "process:pipclone mode is requested but "
		       "pip_clone_mostly_pthread() is not found in glibc" );
      }
      RETURN( EPERM );
    }
  }
  if( desired & PIP_MODE_PTHREAD_BIT ) {
    newmod = PIP_MODE_PTHREAD;
  }

 done:
  *optsp = ( opts & ~PIP_MODE_MASK ) | newmod;
  RETURN( 0 );
}

/* signal handlers */

void pip_set_signal_handler( int sig,
			     void(*handler)(),
			     struct sigaction *oldp ) {
  struct sigaction	sigact;

  memset( &sigact, 0, sizeof( sigact ) );
  sigact.sa_sigaction = handler;
  ASSERT( sigemptyset( &sigact.sa_mask )    != 0 );
  ASSERT( sigaddset( &sigact.sa_mask, sig ) != 0 );
  ASSERT( sigaction( sig, &sigact, oldp )   != 0 );
}

void pip_unset_signal_handler( int sig, struct sigaction *oldp ) {
  ASSERT( sigaction( sig, oldp, NULL ) != 0 );
}

static void pip_sigchld_handler( int sig, siginfo_t *info, void *extra ) {}

static void pip_sigterm_handler( int sig, siginfo_t *info, void *extra ) {
  ENTER;
  ASSERT( pip_task->pipid != PIP_PIPID_ROOT );
  (void) pip_kill_all_tasks();
  (void) kill( getpid(), SIGKILL );
}

void pip_set_sigmask( int sig ) {
  sigset_t sigmask;

  ASSERT( sigemptyset( &sigmask ) );
  ASSERT( sigaddset(   &sigmask, sig ) );
  ASSERT( sigprocmask( SIG_BLOCK, &sigmask, &pip_root->old_sigmask ) );
}

void pip_unset_sigmask( void ) {
  ASSERT( sigprocmask( SIG_SETMASK, &pip_root->old_sigmask, NULL ) );
}

/* save PiP environments */

static void pip_save_debug_envs( pip_root_t *root ) {
  char *env;

  if( ( env = getenv( PIP_ENV_STOP_ON_START ) ) != NULL )
    root->envs.stop_on_start = strdup( env );
  if( ( env = getenv( PIP_ENV_GDB_PATH      ) ) != NULL )
    root->envs.gdb_path      = strdup( env );
  if( ( env = getenv( PIP_ENV_GDB_COMMAND   ) ) != NULL )
    root->envs.gdb_command   = strdup( env );
  if( ( env = getenv( PIP_ENV_GDB_SIGNALS   ) ) != NULL )
    root->envs.gdb_signals   = strdup( env );
  if( ( env = getenv( PIP_ENV_SHOW_MAPS     ) ) != NULL )
    root->envs.show_maps     = strdup( env );
  if( ( env = getenv( PIP_ENV_SHOW_PIPS     ) ) != NULL )
    root->envs.show_pips    = strdup( env );
}

int pip_init( int *pipidp, int *ntasksp, void **rt_expp, int opts ) {
  void pip_named_export_init( pip_task_t* );
  pip_task_t	*task;
  size_t	sz;
  char		*envroot = NULL;
  char		*envtask = NULL;
  int		ntasks;
  int 		pipid;
  int 		i, err = 0;

  if( ( envroot = getenv( PIP_ROOT_ENV ) ) == NULL ) {
    /* root process ? */
    if( pip_root != NULL ) RETURN( EBUSY ); /* already initialized */
    if( ntasksp == NULL ) {
      ntasks = PIP_NTASKS_MAX;
    } else if( *ntasksp <= 0 ) {
      RETURN( EINVAL );
    } else {
      ntasks = *ntasksp;
    }
    if( ntasks > PIP_NTASKS_MAX ) RETURN( EOVERFLOW );

    if( ( err = pip_check_opt_and_env( &opts ) ) != 0 ) RETURN( err );
    sz = sizeof( pip_root_t ) + sizeof( pip_task_t ) * ( ntasks + 1 );
    pip_page_alloc( sz, (void**) &pip_root );
    (void) memset( pip_root, 0, sz );
    pip_root->size_whole = sz;
    pip_root->size_root  = sizeof( pip_root_t );
    pip_root->size_task  = sizeof( pip_task_t );

    pip_spin_init( &pip_root->lock_ldlinux     );
    pip_spin_init( &pip_root->lock_tasks       );
    /* beyond this point, we can call the       */
    /* pip_dlsymc() and pip_dlclose() functions */
    pipid = PIP_PIPID_ROOT;
    pip_set_magic( pip_root );
    pip_root->version      = PIP_API_VERSION;
    pip_root->ntasks       = ntasks;
    pip_root->ntasks_count = 1; /* root is also a PiP task */
    pip_root->cloneinfo    = pip_cloneinfo;
    pip_root->opts         = opts;
    pip_root->page_size    = sysconf( _SC_PAGESIZE );
    pip_root->task_root    = &pip_root->tasks[ntasks];
    pip_sem_init( &pip_root->lock_glibc );
    pip_sem_post( &pip_root->lock_glibc );
    pip_sem_init( &pip_root->sync_spawn );
    for( i=0; i<ntasks+1; i++ ) {
      pip_reset_task_struct( &pip_root->tasks[i] );
      pip_named_export_init( &pip_root->tasks[i] );
    }
    if( rt_expp != NULL ) {
      pip_root->export_root = *rt_expp;
    }
    pip_root->task_root->pipid  = pipid;
    pip_root->task_root->type   = PIP_TYPE_ROOT;
    pip_root->task_root->loaded = dlopen( NULL, RTLD_NOW );
    pip_root->task_root->thread = pthread_self();
    pip_root->task_root->tid    = pip_gettid();
    pip_task = pip_root->task_root;
    unsetenv( PIP_ROOT_ENV );
    {
      char sym[] = "R*";
      sym[1] = pip_cmd_name_symbol( pip_root->opts );
      pip_set_name( sym, NULL );
    }
    DBGF( "PiP Execution Mode: %s", pip_get_mode_str() );

    pip_set_sigmask( SIGCHLD );
    pip_set_signal_handler( SIGCHLD,
			    pip_sigchld_handler,
			    &pip_root->old_sigchld );
    pip_set_signal_handler( SIGTERM,
			    pip_sigterm_handler,
			    &pip_root->old_sigterm );

    pip_gdbif_initialize_root( ntasks );
    pip_gdbif_task_commit( pip_task );

    pip_save_debug_envs( pip_root );

  } else if( ( envtask = getenv( PIP_TASK_ENV ) ) != NULL ) {
    /* child task */
    pip_root_t 	*root;
    int		rv;

    root = (pip_root_t*) strtoll( envroot, NULL, 16 );
    pipid = (int) strtol( envtask, NULL, 10 );
    ASSERT( pipid < 0 || pipid > root->ntasks );
    task = &root->tasks[pipid];
    if( ( rv = pip_init_task_implicitly( root, task ) ) == 0 ) {
      ntasks = pip_root->ntasks;
      /* succeeded */
      if( ntasksp != NULL ) *ntasksp = ntasks;
      if( rt_expp != NULL ) {
	*rt_expp = task->import_root;
      }
      unsetenv( PIP_ROOT_ENV );
      unsetenv( PIP_TASK_ENV );
    } else {
      DBGF( "rv:%d", rv );
      switch( rv ) {
      case 1:
	pip_err_mesg( "Invalid PiP root" );
	break;
      case 2:
	pip_err_mesg( "Magic number error" );
	break;
      case 3:
	pip_err_mesg( "Version miss-match between PiP root and task" );
	break;
      case 4:
	pip_err_mesg( "Size miss-match between PiP root and task" );
	break;
      default:
	pip_err_mesg( "Something wrong with PiP root and task" );
	break;
      }
      RETURN( EPERM );
    }
  } else {
    RETURN( EPERM );
  }
  /* root and child */
  if( pipidp != NULL ) *pipidp = pipid;
  DBGF( "pip_root=%p  pip_task=%p", pip_root, pip_task );

  RETURN( err );
}

int pip_is_pthread( int *flagp ) {
  if( pip_root == NULL ) RETURN( EPERM  );
  if( flagp    == NULL ) RETURN( EINVAL );
  *flagp = pip_is_pthread_();
  RETURN( 0 );
}

static int pip_is_shared_fd_( void ) {
  return pip_is_threaded_();
}

int pip_is_shared_fd( int *flagp ) {
  if( pip_root == NULL ) RETURN( EPERM  );
  if( flagp != NULL ) *flagp = pip_is_shared_fd_();
  RETURN( 0 );
}

int pip_isa_piptask( void ) {
  /* this function might be called before calling pip_init() */
  return getenv( PIP_ROOT_ENV ) != NULL;
}

pip_task_t *pip_get_task_( int pipid ) {
  pip_task_t 	*task = NULL;

  switch( pipid ) {
  case PIP_PIPID_ROOT:
    task = pip_root->task_root;
    break;
  default:
    if( pipid >= 0 && pipid < pip_root->ntasks ) {
      task = &pip_root->tasks[pipid];
    }
    break;
  }
  return task;
}

int pip_get_dso( int pipid, void **loaded ) {
  pip_task_t *task;
  int err;

  if( ( err = pip_check_pipid( &pipid ) ) != 0 ) RETURN( err );
  task = pip_get_task_( pipid );
  if( loaded != NULL ) *loaded = task->loaded;
  RETURN( 0 );
}

int pip_get_pipid_( void ) {
  int pipid;
  if( pip_root == NULL ) {
    pipid = PIP_PIPID_ANY;
  } else if( pip_root_p_() ) {
    pipid = PIP_PIPID_ROOT;
  } else {
    pipid = pip_task->pipid;
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
  if( pip_root == NULL ) return( EPERM  ); /* intentionally using small return */
  *ntasksp = pip_root->ntasks_curr;
  RETURN( 0 );
}

static pip_task_t *pip_get_myself( void ) {
  pip_task_t *task;
  if( pip_root_p_() ) {
    task = pip_root->task_root;
  } else {
    task = pip_task;
  }
  return task;
}

int pip_export( void *export ) {
  if( export == NULL ) RETURN( EINVAL );
  pip_get_myself()->export = export;
  RETURN( 0 );
}

int pip_import( int pipid, void **exportp  ) {
  pip_task_t *task;
  int err;

  if( exportp == NULL ) RETURN( EINVAL );
  if( ( err = pip_check_pipid( &pipid ) ) != 0 ) RETURN( err );

  task = pip_get_task_( pipid );
  *exportp = (void*) task->export;
  pip_memory_barrier();
  RETURN( 0 );
}

int pip_get_addr( int pipid, const char *name, void **addrp ) {
  void *handle;
  int err;

  if( ( err = pip_check_pipid( &pipid ) ) != 0 ) RETURN( err );
  if( name == NULL || addrp == NULL            ) RETURN( EINVAL );
  DBGF( "pipid=%d", pipid );
  if( pipid == PIP_PIPID_ROOT ) {;
    *addrp = pip_dlsym( pip_root->task_root->loaded, name );
  } else if( pipid == PIP_PIPID_MYSELF ) {
    *addrp = pip_dlsym( pip_task->loaded, name );
  } else if( ( handle = pip_root->tasks[pipid].loaded ) != NULL ) {
    *addrp = pip_dlsym( handle, name );
    /* FIXME: pip_dlsym() has a lock but this does not prevent for user */
    /*        programs to directly call dl*() functions without lock    */
    DBGF( "=%p", *addrp );
  } else {
    DBG;
    err = ESRCH;		/* tentative */
  }
  RETURN( err );
}

static int pip_copy_vec( char **vecadd,
			 char **vecsrc,		   /* input */
			 pip_char_vec_t *cvecp ) { /* output */
  char 		**vecdst, *p, *strs;
  size_t	veccc, sz;
  int 		vecln, i, j;

  vecln = 0;
  veccc = 0;
  if( vecadd != NULL ) {
    for( i=0; vecadd[i]!=NULL; i++ ) {
      vecln ++;
      veccc += strlen( vecadd[i] ) + 1;
    }
  }
  for( i=0; vecsrc[i]!=NULL; i++ ) {
    vecln ++;
    veccc += strlen( vecsrc[i] ) + 1;
  }
  vecln ++;		/* plus final NULL */

  sz = sizeof(char*) * vecln;
  if( ( vecdst = (char**) malloc( sz    ) ) == NULL ) {
    return ENOMEM;
  }
  if( ( strs   = (char*)  malloc( veccc ) ) == NULL ) {
    free( vecdst );
    free( strs   );
    return ENOMEM;
  }
  p = strs;
  j = 0;
  if( vecadd != NULL ) {
    for( i=0; vecadd[i]!=NULL; i++ ) {
      vecdst[j++] = p;
      p = stpcpy( p, vecadd[i] ) + 1;
      //ASSERT( j      > vecln );
      //ASSERT( p-strs > veccc );
    }
  }
  for( i=0; vecsrc[i]!=NULL; i++ ) {
    vecdst[j++] = p;
    p = stpcpy( p, vecsrc[i] ) + 1;
    //ASSERT( j      > vecln );
    //ASSERT( p-strs > veccc );
  }
  vecdst[j] = NULL;
  cvecp->vec  = vecdst;
  cvecp->strs = strs;
  return 0;
}

#define ENVLEN	(64)
static int pip_copy_env( char **envsrc, int pipid,
			 pip_char_vec_t *vecp ) {
  char rootenv[ENVLEN], taskenv[ENVLEN];
  char *preload_env = getenv( "LD_PRELOAD" );
  char *addenv[] = { rootenv, taskenv, preload_env, NULL };

  ASSERT( snprintf( rootenv, ENVLEN, "%s=%p", PIP_ROOT_ENV, pip_root ) <= 0 );
  ASSERT( snprintf( taskenv, ENVLEN, "%s=%d", PIP_TASK_ENV, pipid    ) <= 0 );
  return pip_copy_vec( addenv, envsrc, vecp );
}

size_t pip_stack_size( void ) {
  char 		*env, *endptr;
  ssize_t 	s, sz, scale, smax;
  struct rlimit rlimit;

  if( ( sz = pip_root->stack_size ) == 0 ) {
    if( ( env = getenv( PIP_ENV_STACKSZ ) ) == NULL &&
	( env = getenv( "KMP_STACKSIZE" ) ) == NULL &&
	( env = getenv( "OMP_STACKSIZE" ) ) == NULL ) {
      sz = PIP_STACK_SIZE;	/* default */
    } else {
      if( ( sz = (ssize_t) strtoll( env, &endptr, 10 ) ) <= 0 ) {
	pip_err_mesg( "stacksize: '%s' is illegal and "
		      "default size (%lu KiB) is set",
		      env,
		      PIP_STACK_SIZE / 1024 );
	sz = PIP_STACK_SIZE;	/* default */
      } else {
	if( getrlimit( RLIMIT_STACK, &rlimit ) != 0 ) {
	  smax = PIP_STACK_SIZE_MAX;
	} else {
	  smax = rlimit.rlim_cur;
	}
	scale = 1;
	switch( *endptr ) {
	case 'T': case 't':
	  scale *= 1024;
	  /* fall through */
	case 'G': case 'g':
	  scale *= 1024;
	  /* fall through */
	case 'M': case 'm':
	  scale *= 1024;
	  /* fall through */
	case 'K': case 'k':
	case '\0':		/* default is KiB */
	  scale *= 1024;
	  sz *= scale;
	  break;
	case 'B': case 'b':
	  for( s=PIP_STACK_SIZE_MIN; s<sz && s<smax; s*=2 );
	  break;
	default:
	  sz = PIP_STACK_SIZE;
	  pip_err_mesg( "stacksize: '%s' is illegal and "
			"default size (%ldB) is used instead",
			env, sz );
	  break;
	}
	sz = ( sz < PIP_STACK_SIZE_MIN ) ? PIP_STACK_SIZE_MIN : sz;
	sz = ( sz > smax               ) ? smax               : sz;
      }
    }
    pip_root->stack_size = sz;
  }
  return sz;
}

int pip_is_coefd( int fd ) {
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
      if( direntp->d_name[0] != '.' &&
	  ( fd = atoi( direntp->d_name ) ) >= 0 &&
	  fd != fd_dir &&
	  pip_is_coefd( fd ) ) {
#ifdef DEBUG
	pip_print_fd( fd );
#endif
	(void) close( fd );
	DBGF( "<PID=%d> fd[%d] is closed (CLOEXEC)", pip_gettid(), fd );
      }
    }
    (void) closedir( dir );
    (void) close( fd_dir );
  }
#ifdef PRINT_FDS
  pip_print_fds();
#endif
}

static int pip_find_user_symbols( pip_spawn_program_t *progp,
				  void *handle,
				  pip_task_t *task ) {
  pip_symbols_t *symp = &task->symbols;
  int err = 0;

  pip_glibc_lock();		/* to protect dlsym */
  {
    if( progp->funcname == NULL ) {
      symp->main           = dlsym( handle, "main"                  );
    } else {
      symp->start          = dlsym( handle, progp->funcname         );
    }
  }
  pip_glibc_unlock();

  DBGF( "func(%s):%p  main:%p", progp->funcname, symp->start, symp->main );

  /* check start function */
  if( progp->funcname == NULL ) {
    if( symp->main == NULL ) {
      pip_err_mesg( "Unable to find main "
		    "(possibly not linked with '-rdynamic' option)" );
      err = ENOEXEC;
    }
  } else if( symp->start == NULL ) {
    pip_err_mesg( "Unable to find start function (%s)",
		  progp->funcname );
    err = ENOEXEC;
  }
  RETURN( err );
}

static int
pip_find_glibc_symbols( void *handle, pip_task_t *task ) {
  pip_symbols_t *symp = &task->symbols;
  int err = 0;

  pip_glibc_lock();		/* to protect dlsym */
  {
    /* the GLIBC _init() seems not callable. It seems that */
    /* dlmopen()ed name space does not setup VDSO properly */
    symp->ctype_init       = dlsym( handle, "__ctype_init"  );
    symp->mallopt          = dlsym( handle, "mallopt"       );
    symp->libc_fflush      = dlsym( handle, "fflush"        );
    symp->malloc_hook      = dlsym( handle, "__malloc_hook" );
    symp->exit	           = dlsym( handle, "exit"          );
    symp->pthread_exit     = dlsym( handle, "pthread_exit"  );
    /* GLIBC variables */
    symp->libc_argcp     = dlsym( handle, "__libc_argc"     );
    symp->libc_argvp     = dlsym( handle, "__libc_argv"     );
    symp->environ        = dlsym( handle, "environ"         );
    /* GLIBC misc. variables */
    symp->prog           = dlsym( handle, "__progname"      );
    symp->prog_full      = dlsym( handle, "__progname_full" );
  }
  pip_glibc_unlock();
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
pip_load_dsos( pip_spawn_program_t *progp, pip_task_t *task) {
  const char 	*path = progp->prog;
  Lmid_t	lmid;
  char		*libpipinit = NULL;
  pip_init_t	impinit     = NULL;
  void 		*loaded     = NULL;
  void		*ld_glibc   = NULL;
  void 		*ld_pipinit = NULL;
  int 		flags = RTLD_NOW;
  /* RTLD_GLOBAL is NOT accepted and dlmopen() returns EINVAL */
  int		err = 0;

#ifdef RTLD_DEEPBIND
  flags |= RTLD_DEEPBIND;
#endif

  ENTERF( "path:%s", path );
  lmid = LM_ID_NEWLM;
  if( ( loaded = pip_dlmopen( lmid, path, flags ) ) == NULL ) {
    char *dle = pip_dlerror();
    if( ( err = pip_check_pie( path, 1 ) ) != 0 ) goto error;
    pip_err_mesg( "dlmopen(%s): %s", path, dle );
    err = ENOEXEC;
    goto error;
  }
  if( ( err = pip_find_user_symbols( progp, loaded, task ) ) ) {
    goto error;
  }
  if( pip_dlinfo( loaded, RTLD_DI_LMID, &lmid ) != 0 ) {
    pip_err_mesg( "Unable to obtain Lmid (%s): %s",
		  libpipinit, pip_dlerror() );
    err = ENXIO;
    goto error;
  }
  DBGF( "lmid:%d", (int) lmid );
  /* load libc explicitly */
  if( ( ld_glibc = pip_dlmopen( lmid, PIP_INSTALL_GLIBC, flags ) ) == NULL ) {
    DBGF( "dlmopen(%s): %s", PIP_INSTALL_GLIBC, pip_dlerror() );
    err = ENXIO;
    goto error;
  }
  if( ( err = pip_find_glibc_symbols( ld_glibc, task ) ) ) {
    goto error;
  }
  /* call pip_init_task_implicitly */
  /*** we cannot call pip_ini_task_implicitly() directly here  ***/
  /*** the name space contexts of here and there are different ***/
  impinit = (pip_init_t) pip_dlsym( loaded, "pip_init_task_implicitly" );
  if( impinit == NULL ) {
    DBGF( "dlsym: %s", pip_dlerror() );
    if( ( libpipinit = pip_find_newer_libpipinit() ) != NULL ) {
      DBGF( "libpipinit: %s", libpipinit );
      if( ( ld_pipinit = pip_dlmopen( lmid, libpipinit, flags ) ) == NULL ) {
	pip_warn_mesg( "Unable to load %s: %s", libpipinit, pip_dlerror() );
      } else {
	impinit = (pip_init_t)
	  pip_dlsym( ld_pipinit, "pip_init_task_implicitly" );
	DBGF( "dlsym: %s", pip_dlerror() );
      }
    }
  }
  task->symbols.pip_init = impinit;
  task->loaded = loaded;
  RETURN( 0 );

 error:
  if( loaded     != NULL ) pip_dlclose( loaded    );
  if( ld_glibc   != NULL ) pip_dlclose( ld_glibc  );
  if( ld_pipinit != NULL ) pip_dlclose( ld_pipinit);
  RETURN( err );
}

static int pip_load_prog( pip_spawn_program_t *progp,
			  pip_spawn_args_t *args,
			  pip_task_t *task ) {
  int 	err;

  ENTERF( "prog=%s", progp->prog );

  PIP_ACCUM( time_load_dso,
	     ( err = pip_load_dsos( progp, task ) ) == 0 );
  if( !err ) pip_gdbif_load( task );
  RETURN( err );
}

int pip_do_corebind( pid_t tid, uint32_t coreno, cpu_set_t *oldsetp ) {
  cpu_set_t cpuset;
  int flags  = coreno & PIP_CPUCORE_FLAG_MASK;
  int i, err = 0;

  ENTER;
  /* PIP_CPUCORE_* flags are exclusive */
  if( ( flags & PIP_CPUCORE_ASIS ) != flags &&
      ( flags & PIP_CPUCORE_ABS  ) != flags ) RETURN( EINVAL );
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
  } else if( ( flags & PIP_CPUCORE_ABS ) == PIP_CPUCORE_ABS ) {
    CPU_ZERO( &cpuset );
    CPU_SET( coreno, &cpuset );
    /* here, do not call pthread_setaffinity(). This MAY fail */
    /* because pd->tid is NOT set yet.  I do not know why.    */
    /* But it is OK to call sched_setaffinity() with tid.     */
    if( sched_setaffinity( tid, sizeof(cpuset), &cpuset ) != 0 ) {
      RETURN( errno );
    }
  }
  RETURN( err );
}

static int pip_undo_corebind( pid_t tid, uint32_t coreno, cpu_set_t *oldsetp ) {
  int flags = coreno >> PIP_CPUCORE_FLAG_SHIFT;
  int err = 0;

  ENTER;
  if( flags != PIP_CPUCORE_ASIS ) {
    if( tid == 0 ) tid = pip_gettid();
    /* here, do not call pthread_setaffinity().  See above comment. */
    if( sched_setaffinity( tid, sizeof(cpu_set_t), oldsetp ) != 0 ) {
      err = errno;
    }
  }
  RETURN( err );
}

static void pip_glibc_init( pip_symbols_t *symbols,
			    pip_spawn_args_t *args ) {
  /* setting GLIBC variables */
  if( symbols->libc_argcp != NULL ) {
    DBGF( "&__libc_argc=%p", symbols->libc_argcp );
    *symbols->libc_argcp = args->argc;
  }
  if( symbols->libc_argvp != NULL ) {
    DBGF( "&__libc_argv=%p", symbols->libc_argvp );
    *symbols->libc_argvp = args->argvec.vec;
  }
  if( symbols->environ != NULL ) {
    *symbols->environ = args->envvec.vec;	/* setting environment vars */
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
  if( symbols->malloc_hook != 0x0 ) { /* Kaiming Patch */
    *symbols->malloc_hook = 0x0;
  }
}

static void pip_glibc_fin( pip_symbols_t *symbols ) {
  /* call fflush() in the target context to flush out messages */
  if( symbols->libc_fflush != NULL ) {
    DBGF( ">> fflush@%p()", symbols->libc_fflush );
    symbols->libc_fflush( NULL );
    DBGF( "<< fflush@%p()", symbols->libc_fflush );
  }
}

static void pip_return_from_start_func( pip_task_t *task,
					int extval ) {
  int err = 0;

  ENTER;
  if( pip_gettid() != task->tid ) {
    /* when a PiP task fork()s and returns */
    /* from main this case happens         */
    DBGF( "return from a fork()ed process?" );
    /* here we have to call the exit() in the same context */
    if( task->symbols.exit != NULL ) {
      task->symbols.exit( extval );
    } else {
      exit( extval );
    }
    NEVER_REACH_HERE;
  }
  if( task->hook_after != NULL &&
      ( err = task->hook_after( task->hook_arg ) ) != 0 ) {
    pip_err_mesg( "PIPID:%d after-hook returns %d", task->pipid, err );
    if( extval == 0 ) extval = err;
  }
  pip_glibc_fin( &task->symbols );
  if( task->symbols.named_export_fin != NULL ) {
    task->symbols.named_export_fin( task );
  }
  pip_gdbif_exit( task, WEXITSTATUS(extval) );
  pip_gdbif_hook_after( task );

  DBGF( "PIPID:%d -- FORCE EXIT:%d", task->pipid, extval );
  fflush( NULL );

  if( pip_is_threaded_() ) {	/* thread mode */
    task->flag_sigchld = 1;
    ASSERT( pip_raise_signal( pip_root->task_root, SIGCHLD ) );
    if( task->symbols.pthread_exit != NULL ) {
      task->symbols.pthread_exit( NULL );
    } else {
      pthread_exit( NULL );
    }
  } else {			/* process mode */
    if( task->symbols.exit != NULL ) {
      task->symbols.exit( extval );
    } else {
      exit( extval );
    }
  }
  NEVER_REACH_HERE;
}

static void pip_sigquit_handler( int, void(*)(), struct sigaction* )
  __attribute__((noreturn));
static void pip_sigquit_handler( int sig,
				 void(*handler)(),
				 struct sigaction *oldp ) {
  ENTER;
  pthread_exit( NULL );
  NEVER_REACH_HERE;
}

static void pip_reset_signal_handler( int sig ) {
  if( !pip_is_threaded_() ) {
    struct sigaction	sigact;
    memset( &sigact, 0, sizeof( sigact ) );
    sigact.sa_sigaction = (void(*)(int,siginfo_t*,void*)) SIG_DFL;
    ASSERT( sigaction( sig, &sigact, NULL ) != 0 );
  } else {
    sigset_t sigmask;
    (void) sigemptyset( &sigmask );
    (void) sigaddset( &sigmask, sig );
    ASSERT( pthread_sigmask( SIG_BLOCK, &sigmask, NULL ) != 0 );
  }
}

static void *pip_do_spawn( void *thargs )  {
  pip_spawn_args_t *args = (pip_spawn_args_t*) thargs;
  int 	pipid      = args->pipid;
  char **argv      = args->argvec.vec;
  char **envv      = args->envvec.vec;
  void *start_arg  = args->start_arg;
  int coreno       = args->coreno;
  pip_task_t *self = &pip_root->tasks[pipid];
  pip_spawnhook_t before = self->hook_before;
  void *hook_arg         = self->hook_arg;
  int 	extval = 0, err = 0;

  ENTER;
  self->thread = pthread_self();
  self->tid    = pip_gettid();

  pip_glibc_unlock();
  pip_sem_post( &pip_root->sync_spawn );

  if( ( err = pip_do_corebind( 0, coreno, NULL ) ) != 0 ) {
    pip_warn_mesg( "failed to bound CPU core:%d (%d)", coreno, err );
    err = 0;
  }
  {
    char sym[] = "0*";
    sym[0] += ( pipid % 10 );
    sym[1] = pip_cmd_name_symbol( pip_root->opts );
    pip_set_name( sym, args->prog );
  }
  if( !pip_is_threaded_() ) {
    pip_reset_signal_handler( SIGCHLD );
    pip_reset_signal_handler( SIGTERM );
    (void) setpgid( 0, (pid_t) pip_root->task_root->tid );
  } else {
    pip_set_signal_handler( SIGQUIT, pip_sigquit_handler, NULL );
  }
#ifdef DEBUG
  if( pip_is_pthread_() ) {
    pthread_attr_t attr;
    size_t sz;
    int _err;
    if( ( _err = pthread_getattr_np( self->thread, &attr      ) ) != 0 ) {
      DBGF( "pthread_getattr_np()=%d", _err );
    } else if( ( _err = pthread_attr_getstacksize( &attr, &sz ) ) != 0 ) {
      DBGF( "pthread_attr_getstacksize()=%d", _err );
    } else {
      DBGF( "stacksize = %ld [KiB]", sz/1024 );
    }
  }
#endif
  if( !pip_is_shared_fd_() ) pip_close_on_exec();

  /* let pip-gdb know */
  pip_gdbif_task_commit( self );

  /* calling hook, if any */
  if( before != NULL && ( err = before( hook_arg ) ) != 0 ) {
    pip_warn_mesg( "try to spawn(%s), but the before hook at %p returns %d",
		   argv[0], before, err );
    extval = err;
  } else {
    /* argv and/or envv might be changed in the hook function */

    //(void) pip_init_glibc( &self->symbols, argv, envv, self->loaded );
    pip_glibc_init( &self->symbols, args );
    if( self->symbols.pip_init != NULL ) {
      err = self->symbols.pip_init( pip_root, self );
    }
    if( err ) {
      extval = err;
    } else {
      char *env_stop;

      pip_gdbif_hook_before( self );
      if( ( env_stop = pip_root->envs.stop_on_start ) != NULL ) {
	int pipid_stop = strtol( env_stop, NULL, 10 );
	if( pipid_stop < 0 || pipid_stop == self->pipid ) {
	  pip_info_mesg( "PiP task[%d] is SIGSTOPed (%s=%s)",
			 self->pipid, PIP_ENV_STOP_ON_START, env_stop );
	  pip_kill( self->pipid, SIGSTOP );
	}
      }
      if( self->symbols.start == NULL ) {
	/* calling hook function, if any */
	DBGF( "[%d] >> main@%p(%d,%s,%s,...)",
	      args->pipid, self->symbols.main, args->argc, argv[0], argv[1] );
	extval = self->symbols.main( args->argc, argv, envv );
	DBGF( "[%d] << main@%p(%d,%s,%s,...) = %d",
	      args->pipid, self->symbols.main, args->argc, argv[0], argv[1],
	      extval );
      } else {
	DBGF( "[%d] >> %s:%p(%p)",
	      args->pipid, args->funcname,
	      self->symbols.start, start_arg );
	extval = self->symbols.start( start_arg );
	DBGF( "[%d] << %s:%p(%p) = %d",
	      args->pipid, args->funcname,
	      self->symbols.start, start_arg, extval );
      }
    }
  }
  pip_return_from_start_func( self, extval );
  NEVER_REACH_HERE;
  return NULL;			/* dummy */
}

static int pip_find_a_free_task( int *pipidp ) {
  int pipid = *pipidp;
  int err = 0;

  if( pip_root->ntasks_accum >= PIP_NTASKS_MAX ) RETURN( EOVERFLOW );
  if( pipid < PIP_PIPID_ANY || pipid >= pip_root->ntasks ) {
    DBGF( "pipid=%d", pipid );
    RETURN( EINVAL );
  }

  pip_spin_lock( &pip_root->lock_tasks );
  /*** begin lock region ***/
  do {
    if( pipid != PIP_PIPID_ANY ) {
      if( pip_root->tasks[pipid].pipid != PIP_PIPID_NONE ) {
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
			      pip_task_t **tskp,
			      pip_spawn_hook_t *hookp ) {
  cpu_set_t 		cpuset;
  pip_spawn_args_t	*args = NULL;
  pip_task_t		*task = NULL;
  size_t		stack_size;
  int 			err = 0;

  ENTER;
  if( pip_root == NULL ) RETURN( EPERM );
  if( !pip_isa_root()  ) RETURN( EPERM );
  if( progp       == NULL ) RETURN( EINVAL );
  if( progp->prog == NULL ) RETURN( EINVAL );
  /* starting from main */
  if( progp->funcname == NULL &&
      ( progp->argv == NULL || progp->argv[0] == NULL ) ) {
    RETURN( EINVAL );
  }
  /* starting from an arbitrary func */
  if( progp->funcname == NULL && progp->prog == NULL ) {
    progp->prog = progp->argv[0];
  }
  if( pipid == PIP_PIPID_MYSELF ||
      pipid == PIP_PIPID_NULL ) {
    RETURN( EINVAL );
  }
  if( pipid != PIP_PIPID_ANY ) {
    if( pipid < 0 || pipid > pip_root->ntasks ) RETURN( EINVAL );
  }
  if( ( err = pip_find_a_free_task( &pipid ) ) != 0 ) goto error;
  task = &pip_root->tasks[pipid];
  pip_reset_task_struct( task );
  task->pipid     = pipid;	/* mark it as occupied */
  task->type      = PIP_TYPE_TASK;
  task->task_root = pip_root;

  args = &task->args;
  args->pipid     = pipid;
  args->coreno    = coreno;
  { 				/* GLIBC/misc/init-misc.c */
    char *prog = progp->prog;
    char *p = strrchr( prog, '/' );
    if( p == NULL ) {
      args->prog      = prog;
    } else {
      args->prog      = p + 1;
      args->prog_full = prog;
    }
    DBGF( "prog:%s full:%s", args->prog, args->prog_full );
  }
  err = pip_copy_env( progp->envv, pipid, &args->envvec );
  if( err ) ERRJ_ERR( err );

  if( progp->funcname == NULL ) {
    err = pip_copy_vec( NULL, progp->argv, &args->argvec );
    if( err ) ERRJ_ERR( err );
    args->argc = pip_count_vec( args->argvec.vec );
  } else {
    if( ( args->funcname = strdup( progp->funcname ) ) == NULL ) {
      ERRJ_ERR( ENOMEM );
    }
    args->start_arg = progp->arg;
  }
  task->aux           = progp->aux;
  if( progp->exp != NULL ) {
    task->import_root = progp->exp;
  } else {
    task->import_root = pip_root->export_root;
  }
  if( hookp != NULL ) {
    task->hook_before = hookp->before;
    task->hook_after  = hookp->after;
    task->hook_arg    = hookp->hookarg;
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
  if( err != 0 ) goto error;

  stack_size = pip_stack_size();
  if( ( pip_root->opts & PIP_MODE_PROCESS_PIPCLONE ) ==
      PIP_MODE_PROCESS_PIPCLONE ) {
    int flags = pip_clone_flags( CLONE_PARENT_SETTID |
				 CLONE_CHILD_CLEARTID |
				 CLONE_SYSVSEM |
				 CLONE_PTRACE );
    pid_t pid;

    /* we need lock on ldlinux. supposedly glibc does someting */
    /* before calling main function */
    pip_glibc_lock();
    err = pip_clone_mostly_pthread_ptr( &task->thread,
					flags,
					coreno == PIP_CPUCORE_ASIS ? - 1 :
					coreno,
					stack_size,
					(void*(*)(void*)) pip_do_spawn,
					args,
					&pid );
    DBGF( "pip_clone_mostly_pthread_ptr()=%d", err );
    if( err ) pip_glibc_unlock();
  } else {
    pthread_attr_t 	attr;
    pid_t 		tid = pip_gettid();

    if( ( err = pthread_attr_init( &attr ) ) != 0 ) goto error;
    err = pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_JOINABLE );
    DBGF( "pthread_attr_setdetachstate(JOINABLE)= %d", err );
    if( err ) goto error;
    err = pthread_attr_setstacksize( &attr, stack_size );
    DBGF( "pthread_attr_setstacksize( %ld )= %d", stack_size, err );
    if( err ) goto error;

    DBGF( "tid=%d  cloneinfo@%p", tid, pip_root->cloneinfo );
    pip_glibc_lock();
    do {
      if( pip_lock_clone != NULL ) {
	/* lock is needed, because the pip_clone()
	     might also be called from outside of PiP lib. */
	pip_spin_lock_wv( pip_lock_clone, tid );
      }
      err = pthread_create( &task->thread,
			    &attr,
			    (void*(*)(void*)) pip_do_spawn,
			    (void*) args );
      DBGF( "pthread_create()=%d", err );
      if( err ) pip_glibc_unlock();
    } while( 0 );
    /* unlock is done in the wrapper function */
  }
  if( err == 0 ) {
    pip_sem_wait( &pip_root->sync_spawn );
    pip_root->ntasks_count ++;
    pip_root->ntasks_accum ++;
    pip_root->ntasks_curr  ++;
    *tskp = task;

  } else {
  error:			/* undo */
    DBG;
    if( args != NULL ) {
      if( args->argvec.vec  != NULL ) free( args->argvec.vec  );
      if( args->argvec.strs != NULL ) free( args->argvec.strs );
      if( args->envvec.vec  != NULL ) free( args->envvec.vec  );
      if( args->envvec.strs != NULL ) free( args->envvec.strs );
    }
    if( task != NULL ) {
      if( task->loaded != NULL ) (void) pip_dlclose( task->loaded );
      pip_reset_task_struct( task );
    }
  }
  RETURN( err );
}

int pip_task_spawn( pip_spawn_program_t *progp,
		    uint32_t coreno,
		    uint32_t opts,
		    int *pipidp,
		    pip_spawn_hook_t *hookp ) {
  pip_task_t	*task = NULL;
  int 		pipid;
  int 		err;

  ENTER;
  if( pipidp == NULL ) {
    pipid = PIP_PIPID_ANY;
  } else {
    pipid = *pipidp;
  }
  err = pip_do_task_spawn( progp, pipid, coreno, opts, &task, hookp );
  if( !err ) {
    if( pipidp != NULL ) *pipidp = task->pipid;
  }
  RETURN( err );
}

int pip_spawn( char *prog,
	       char **argv,
	       char **envv,
	       int   coreno,
	       int  *pipidp,
	       pip_spawnhook_t before,
	       pip_spawnhook_t after,
	       void *hookarg ) {
  pip_spawn_program_t program;
  pip_spawn_hook_t hook;

  ENTER;
  if( prog == NULL ) return EINVAL;
  pip_spawn_from_main( &program, prog, argv, envv, NULL, NULL );
  pip_spawn_hook( &hook, before, after, hookarg );
  RETURN( pip_task_spawn( &program, coreno, 0, pipidp, &hook ) );
}

int pip_fin( void ) {
  void pip_undo_patch_GOT( void );
  int ntasks, i, err = 0;

  DBG;
  if( !pip_is_initialized() ) RETURN( EPERM );
  if( pip_root_p_() ) {
    ntasks = pip_root->ntasks;
    for( i=0; i<ntasks; i++ ) {
      if( pip_root->tasks[i].type != PIP_TYPE_NULL ) {
	DBGF( "%d/%d [%d] -- BUSY", i, ntasks, pip_root->tasks[i].pipid );
	err = EBUSY;
	break;
      }
    }
    if( err == 0 ) {
      void pip_named_export_fin_all( void );
      /* SIGCHLD */
      pip_unset_sigmask();
      pip_unset_signal_handler( SIGCHLD,
				&pip_root->old_sigchld );
      /* SIGTERM */
      pip_unset_signal_handler( SIGTERM,
				&pip_root->old_sigterm );

      pip_named_export_fin_all();

      PIP_REPORT( time_load_dso  );
      PIP_REPORT( time_load_prog );
      PIP_REPORT( time_dlmopen   );

      memset( pip_root, 0, sizeof(pip_root_t) );
      free( pip_root );
      pip_root = NULL;
      pip_task = NULL;

      pip_undo_patch_GOT();
    }
  }
  RETURN( err );
}

int pip_get_mode( int *mode ) {
  if( pip_root == NULL ) RETURN( EPERM  );
  if( mode     == NULL ) RETURN( EINVAL );
  *mode = ( pip_root->opts & PIP_MODE_MASK );
  RETURN( 0 );
}

int pip_get_system_id( int pipid, pip_id_t *idp ) {
  pip_task_t 	*task;
  intptr_t	id;
  int 		err;

  if( ( err = pip_check_pipid( &pipid ) ) != 0 ) RETURN( err );
  if( pipid == PIP_PIPID_ROOT ) RETURN( EINVAL );

  task = pip_get_task_( pipid );
  if( pip_is_pthread_() ) {
    /* Do not use gettid(). This is a very Linux-specific function */
    /* The reason of supporintg the thread PiP execution mode is   */
    /* some OSes other than Linux does not support clone()         */
    id = (intptr_t) task->thread;
  } else {
    id = (intptr_t) task->tid;
  }
  if( idp != NULL ) *idp = id;
  RETURN( 0 );
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

pip_clone_t *pip_get_cloneinfo_( void ) {
  return pip_root->cloneinfo;
}

int pip_get_pid_( int pipid, pid_t *pidp ) {
  pid_t	pid;
  int 	err = 0;

  if( pip_root->opts && PIP_MODE_PROCESS ) {
    /* only valid with the "process" execution mode */
    if( ( err = pip_check_pipid( &pipid ) ) == 0 ) {
      if( pipid == PIP_PIPID_ROOT ) {
	err = EPERM;
      } else {
	pid = pip_root->tasks[pipid].tid;
      }
    }
  } else {
    err = EPERM;
  }
  if( !err && pidp != NULL ) *pidp = pid;
  RETURN( err );
}

int pip_barrier_init( pip_barrier_t *barrp, int n ) {
  if( !pip_is_initialized() ) return EPERM;
  if( n < 0 ) return EINVAL;
  barrp->count      = n;
  barrp->count_init = n;
  barrp->gsense     = 0;
  return 0;
}

int pip_barrier_wait( pip_barrier_t *barrp ) {
  if( barrp->count_init > 1 ) {
    int lsense = !barrp->gsense;
    if( __sync_sub_and_fetch( &barrp->count, 1 ) == 0 ) {
      barrp->count  = barrp->count_init;
      pip_memory_barrier();
      barrp->gsense = lsense;
    } else {
      while( barrp->gsense != lsense ) pip_pause();
    }
  }
  return 0;
}

int pip_barrier_fin( pip_barrier_t *barrp ) {
  if( !pip_is_initialized() ) return EPERM;
  if( barrp->count != 0     ) return EBUSY;
  return 0;
}

int pip_yield( int flag ) {
  if( !pip_is_initialized() ) RETURN( EPERM );
  if( pip_root != NULL && pip_is_threaded_() ) {
    int pthread_yield( void );
    (void) pthread_yield();
  } else {
    sched_yield();
  }
  return 0;
}

int pip_set_aux( void *aux ) {
  if( !pip_is_initialized() ) RETURN( EPERM );
  pip_task->aux = aux;
  RETURN( 0 );
}

int pip_get_aux( void **auxp ) {
  if( !pip_is_initialized() ) RETURN( EPERM );
  if( auxp != NULL ) {
    *auxp = pip_task->aux;
  }
  RETURN( 0 );
}

/* energy-saving spin-lock */
void pip_glibc_lock( void ) {
  if( pip_root != NULL ) pip_sem_wait( &pip_root->lock_glibc );
}

void pip_glibc_unlock( void ) {
  if( pip_root != NULL ) pip_sem_post( &pip_root->lock_glibc );
}
