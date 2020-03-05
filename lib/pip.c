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

//#define DEBUG
//#define PRINT_MAPS
//#define PRINT_FDS

/* the EVAL define symbol is to measure the time for calling dlmopen() */
//#define EVAL

#include <limits.h>		/* for PTHREAD_STACK_MIN */
#define PIP_SLEEP_STACKSZ	PTHREAD_STACK_MIN

#include <pip.h>
#include <pip_dlfcn.h>
#include <pip_internal.h>
#include <pip_gdbif.h>

#define ROUNDUP(X,Y)		((((X)+(Y)-1)/(Y))*(Y))

//#define ATTR_NOINLINE		__attribute__ ((noinline))
//#define ATTR_NOINLINE

extern char 		**environ;

extern pip_spinlock_t 	*pip_lock_clone;

/*** note that the following static variables are   ***/
/*** located at each PIP task and the root process. ***/
pip_root_t		*pip_root PIP_PRIVATE;
pip_task_internal_t	*pip_task PIP_PRIVATE;

static pip_clone_t*	pip_cloneinfo = NULL;

static void pip_set_magic( pip_root_t *root ) {
  memcpy( root->magic, PIP_MAGIC_WORD, PIP_MAGIC_WLEN );
}

int pip_is_magic_ok( pip_root_t *root ) {
  return root != NULL &&
    strncmp( root->magic, PIP_MAGIC_WORD, PIP_MAGIC_WLEN ) == 0;
}

int pip_is_version_ok( pip_root_t *root ) {
  if( root            != NULL                           &&
      root->version    == PIP_API_VERSION               &&
      root->size_root  == sizeof( pip_root_t )          &&
      root->size_task  == sizeof( pip_task_internal_t ) &&
      root->size_annex == sizeof( pip_task_annex_t ) ) {
    return 1;
  }
  return 0;
}

static int pip_set_root( char *env ) {
  if( *env == '\0' ) ERRJ;
  pip_root = (pip_root_t*) strtoll( env, NULL, 16 );
  if( pip_root == NULL ) {
    pip_err_mesg( "Invalid PiP root" );
    ERRJ;
  }
  if( !pip_is_magic_ok( pip_root ) ) {
    pip_err_mesg( "%s environment not found", PIP_ROOT_ENV );
    ERRJ;
  }
  if( !pip_is_version_ok( pip_root ) ) {
    pip_err_mesg( "Version miss-match between PiP root and task" );
    ERRJ;
  }
  return 0;

 error:
  pip_root = NULL;
  RETURN( EPROTO );
}

#define NITERS		(100)
#define FACTOR_INIT	(10)
static uint64_t pip_measure_yieldtime( void ) {
  double dt, xt;
  uint64_t c;
  int i;

  for( i=0; i<NITERS/10; i++ ) pip_system_yield();
  dt = -pip_gettime();
  for( i=0; i<NITERS; i++ ) pip_system_yield();
  dt += pip_gettime();
  DBGF( "DT:%g", dt );

  for( i=0; i<NITERS/10; i++ ) pip_pause();
  xt = 0.0;
  for( c=FACTOR_INIT; ; c*=2 ) {
    xt = -pip_gettime();
    for( i=0; i<NITERS*c; i++ ) pip_pause();
    xt += pip_gettime();
    DBGF( "c:%lu  XT:%g  DT:%g", c, xt, dt );
    if( xt > dt ) break;
  }
  c *= 10;
  DBGF( "yield:%lu", c );
  return c;
}

pip_clone_mostly_pthread_t pip_clone_mostly_pthread_ptr = NULL;

static int pip_check_opt_and_env( int *optsp ) {
  extern pip_spinlock_t pip_lock_got_clone;
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
    if( env == NULL ) {
      desired =
	PIP_MODE_PTHREAD_BIT         |
	PIP_MODE_PROCESS_PRELOAD_BIT |
	PIP_MODE_PROCESS_GOT_BIT     |
	PIP_MODE_PROCESS_PIPCLONE_BIT;
    } else if( strcasecmp( env, PIP_ENV_MODE_THREAD  ) == 0 ||
	       strcasecmp( env, PIP_ENV_MODE_PTHREAD ) == 0 ) {
      desired = PIP_MODE_PTHREAD_BIT;
    } else if( strcasecmp( env, PIP_ENV_MODE_PROCESS ) == 0 ) {
      desired =
	PIP_MODE_PROCESS_PRELOAD_BIT |
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
    /* check if the __clone() systemcall wrapper exists or not */
    if( pip_replace_GOT( "libpthread.so", "__clone" ) == 0 ) {
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
  DBGF( "newmod:0x%x", newmod );
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
  DBGF( "newmod:0x%x", newmod );
  if( desired & PIP_MODE_PTHREAD_BIT ) {
    newmod = PIP_MODE_PTHREAD;
    goto done;
  }
  DBGF( "newmod:0x%x", newmod );
  if( newmod == 0 ) {
    pip_warn_mesg( "pip_init() implemenation error. desired = 0x%x", desired );
    RETURN( EOPNOTSUPP );
  }
 done:
  DBGF( "newmod:0x%x", newmod );
  *optsp = ( opts & ~PIP_MODE_MASK ) | newmod;
  RETURN( 0 );
}

static pip_task_internal_t *pip_get_myself( void ) {
  pip_task_internal_t *taski;
  if( pip_isa_root() ) {
    taski = pip_root->task_root;
  } else {
    taski = pip_task;
  }
  return taski;
}

/* internal funcs */

void pip_page_alloc( size_t sz, void **allocp ) {
  size_t pgsz;

  if( pip_root == NULL ) {	/* no pip_root yet */
    pgsz = sysconf( _SC_PAGESIZE );
  } else if( pip_root->page_size == 0 ) {
    pip_root->page_size = pgsz = sysconf( _SC_PAGESIZE );
  } else {
    pgsz = pip_root->page_size;
  }
  sz = ROUNDUP( sz, pgsz );
  ASSERT( posix_memalign( allocp, pgsz, sz ) != 0 &&
	  *allocp == NULL );
}

void pip_reset_task_struct( pip_task_internal_t *taski ) {
  pip_task_annex_t 	*annex = taski->annex;
  void			*stack_sleep = annex->stack_sleep;
  void			*namexp = annex->named_exptab;

  memset( (void*) taski, 0, sizeof( pip_task_internal_t ) );
  PIP_TASKQ_INIT( &taski->queue  );
  PIP_TASKQ_INIT( &taski->schedq );
  taski->type  = PIP_TYPE_NONE;
  taski->pipid = PIP_PIPID_NULL;
  taski->annex = annex;

  memset( (void*) annex, 0, sizeof( pip_task_annex_t ) );
  annex->stack_sleep  = stack_sleep;
  annex->named_exptab = namexp;
  annex->tid          = -1; /* pip_gdbif_init_task_struct() refers this */
  PIP_TASKQ_INIT( &taski->annex->oodq      );
  pip_spin_init(  &taski->annex->lock_oodq );
  pip_sem_init(   &taski->annex->sleep     );
}

static int
pip_is_flag_excl( uint32_t flags, uint32_t val ) {
  return ( flags & val ) == ( flags | val );
}

int pip_check_sync_flag( uint32_t flags ) {
  uint32_t f = flags & PIP_SYNC_MASK;

  DBGF( "flags:0x%x", f );
  if( f ) {
    if( pip_is_flag_excl( f, PIP_SYNC_AUTO     ) ) goto OK;
    if( pip_is_flag_excl( f, PIP_SYNC_BUSYWAIT ) ) goto OK;
    if( pip_is_flag_excl( f, PIP_SYNC_YIELD    ) ) goto OK;
    if( pip_is_flag_excl( f, PIP_SYNC_BLOCKING ) ) goto OK;
    return -1;
  } else {
    char *env = getenv( PIP_ENV_SYNC );
    if( env == NULL ) {
      f |= PIP_SYNC_AUTO;
    } else if( strcasecmp( env, PIP_ENV_SYNC_AUTO     ) == 0 ) {
      f |= PIP_SYNC_AUTO;
    } else if( strcasecmp( env, PIP_ENV_SYNC_BUSY     ) == 0 ||
	       strcasecmp( env, PIP_ENV_SYNC_BUSYWAIT ) == 0 ) {
      f |= PIP_SYNC_BUSYWAIT;
    } else if( strcasecmp( env, PIP_ENV_SYNC_YIELD    ) == 0 ) {
      f |= PIP_SYNC_YIELD;
    } else if( strcasecmp( env, PIP_ENV_SYNC_BLOCK    ) == 0 ||
	       strcasecmp( env, PIP_ENV_SYNC_BLOCKING ) == 0 ) {
      f |= PIP_SYNC_BLOCKING;
    }
  }
 OK:
  return flags;
}

int pip_check_task_flag( uint32_t flags ) {
  uint32_t f = flags & PIP_TASK_MASK;

  DBGF( "flags:0x%x", f );
  if( f ) {
    if( pip_is_flag_excl( f, PIP_TASK_ACTIVE  ) ) goto OK;
    if( pip_is_flag_excl( f, PIP_TASK_PASSIVE ) ) goto OK;
    return -1;
  }
 OK:
  return flags;
}

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

static void pip_unset_signal_handler( int sig,
				      struct sigaction *oldp ) {
  ASSERT( sigaction( sig, oldp, NULL ) != 0 );
}

/* signal handlers */

static void pip_sigchld_handler( int sig, siginfo_t *info, void *extra ) {
  if( pip_root != NULL ) {
    pip_spin_unlock( &pip_root->lock_ldlinux );
  }
  DBG;
}

static void pip_sigterm_handler( int sig, siginfo_t *info, void *extra ) {
  ENTER;
  ASSERTD( pip_task->pipid != PIP_PIPID_ROOT );
#ifdef AH
  int i;
  for( i=0; i<pip_root->ntasks; i++ ) {
    pip_task_internal_t *taski = &pip_root->tasks[i];
    char *ststr = "(unknown)";
    char idstr[64];
    if( taski->type == PIP_TYPE_NONE ) continue;
    if( pip_root->tasks[i].flag_exit == PIP_EXITED ) {
      ststr = "EXITED";
    } else {
      if( PIP_IS_RUNNING( taski ) ) {
	if( taski->task_sched == taski ) {
	  ststr = "RUNNING";
	} else if( !taski->flag_semwait ) {
	  ststr = "running";
	} else {
	  ststr = "sleeping";
	}
      } else if( PIP_IS_SUSPENDED( taski ) ) {
	if( taski->task_sched == taski ) {
	  ststr = "SUSPENDED";
	} else if( !taski->flag_semwait ) {
	  ststr = "suspended";
	} else {
	  ststr = "sleeping";
	}
      }
    }
    pip_pipidstr( taski, idstr );
    DBGF( "%s(PID:%d) %s", idstr, taski->annex->tid, ststr );
  }
#endif
  (void) pip_kill_all_tasks();
  RETURNV;
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

static void pip_sighup_handler( int sig, siginfo_t *info, void *extra ) {
  ENTER;
  pip_err_mesg( "Hangup signal !!" );
  pip_backtrace_fd( 0, 1 );
  fflush( NULL );
  pip_sigterm_handler( sig, info, extra );
  RETURNV;
}

/* API */

#define PIP_CACHE_ALIGN(X) \
  ( ( (X) + PIP_CACHEBLK_SZ - 1 ) & ~( PIP_CACHEBLK_SZ -1 ) )

int pip_init( int *pipidp, int *ntasksp, void **rt_expp, int opts ) {
  size_t	sz;
  char		*envroot = NULL;
  char		*envtask = NULL;
  int		ntasks, pipid;
  int		i, err = 0;

#ifdef MCHECK
  mcheck( NULL );
#endif

  if( pip_root != NULL ) RETURN( EBUSY ); /* already initialized */
  if( ( envroot = getenv( PIP_ROOT_ENV ) ) == NULL ) {
    /* root process */

    if( ntasksp == NULL ) {
      ntasks = PIP_NTASKS_MAX;
    } else {
      ntasks = *ntasksp;
    }
    if( ntasks <= 0             ) RETURN( EINVAL );
    if( ntasks > PIP_NTASKS_MAX ) RETURN( EOVERFLOW );

    if( ( err  = pip_check_opt_and_env( &opts ) ) != 0 ) RETURN( err );
    if( ( opts = pip_check_sync_flag(    opts ) )  < 0 ) RETURN( EINVAL );

    sz = PIP_CACHE_ALIGN( sizeof( pip_root_t ) ) +
         PIP_CACHE_ALIGN( sizeof( pip_task_internal_t ) * ( ntasks + 1 ) ) +
         PIP_CACHE_ALIGN( sizeof( pip_task_annex_t    ) * ( ntasks + 1 ) );
    pip_page_alloc( sz, (void**) &pip_root );
    (void) memset( pip_root, 0, sz );

    pip_root->size_whole = sz;
    pip_root->size_root  = sizeof( pip_root_t );
    pip_root->size_task  = sizeof( pip_task_internal_t );
    pip_root->size_annex = sizeof( pip_task_annex_t );

    DBGF( "sizeof(root):%d  sizoef(task):%d  sizeof(annex):%d",
	  (int) pip_root->size_root, (int) pip_root->size_task,
	  (int) pip_root->size_annex );

    pip_root->annex = (pip_task_annex_t*)
      ( ((intptr_t)pip_root) +
	PIP_CACHE_ALIGN( sizeof( pip_root_t ) ) +
	PIP_CACHE_ALIGN( sizeof( pip_task_internal_t ) * ( ntasks + 1 ) ) );

    DBGF( "ROOTROOT (%p)", pip_root );

    pip_spin_init( &pip_root->lock_ldlinux );
    pip_spin_init( &pip_root->lock_tasks   );
    pip_spin_init( &pip_root->lock_bt      );

    pip_sem_init( &pip_root->sync_root );
    pip_sem_init( &pip_root->sync_task );

    pipid = PIP_PIPID_ROOT;
    pip_set_magic( pip_root );
    pip_root->version      = PIP_API_VERSION;
    pip_root->ntasks       = ntasks;
    pip_root->ntasks_count = 1; /* root is also a PiP task */
    pip_root->cloneinfo    = pip_cloneinfo;
    pip_root->opts         = opts;
    pip_root->yield_iters  = pip_measure_yieldtime();
    for( i=0; i<ntasks+1; i++ ) {
      pip_root->tasks[i].annex = &pip_root->annex[i];
      pip_reset_task_struct( &pip_root->tasks[i] );
      pip_named_export_init( &pip_root->tasks[i] );
    }
    pip_root->task_root = &pip_root->tasks[ntasks];

    pip_task = pip_root->task_root;
    pip_task->pipid      = pipid;
    pip_task->type       = PIP_TYPE_ROOT;
    pip_task->task_sched = pip_task;

    pip_task->annex->loaded = dlopen( NULL, RTLD_NOW );
    pip_task->annex->tid    = pip_gettid();
    pip_task->annex->thread = pthread_self();
#ifdef PIP_SAVE_TLS
    pip_save_tls( &pip_task->tls );
#endif
    if( rt_expp != NULL ) {
      pip_task->annex->exp = *rt_expp;
    }
    pip_root->stack_size_sleep = PIP_SLEEP_STACKSZ;
    pip_page_alloc( pip_root->stack_size_sleep,
		     &pip_task->annex->stack_sleep );
    if( pip_task->annex->stack_sleep == NULL ) {
      free( pip_root );
      RETURN( err );
    }
    {
      char *sym;
      switch( pip_root->opts & PIP_MODE_MASK ) {
      case PIP_MODE_PROCESS_PRELOAD:
	sym = "R:";
	break;
      case PIP_MODE_PROCESS_PIPCLONE:
	sym = "R;";
	break;
      case PIP_MODE_PTHREAD:
	sym = "R|";
	break;
      default:
	sym = "R?";
	break;
      }
      pip_set_name( sym, NULL, NULL );
    }
    pip_set_sigmask( SIGCHLD );
    pip_set_signal_handler( SIGCHLD,
			    pip_sigchld_handler,
			    &pip_root->old_sigchld );
    pip_set_signal_handler( SIGTERM,
			    pip_sigterm_handler,
			    &pip_root->old_sigterm );
    pip_set_signal_handler( SIGHUP,
			    pip_sighup_handler,
			    &pip_root->old_sighup );

    DBGF( "PiP Execution Mode: %s", pip_get_mode_str() );

    pip_gdbif_initialize_root( ntasks );

  } else if( ( envtask = getenv( PIP_TASK_ENV ) ) != NULL ) {
    /* child task */
    if( ( err = pip_set_root( envroot ) ) != 0 ) RETURN( err );
    pipid = (int) strtol( envtask, NULL, 10 );
    if( pipid < 0 || pipid >= pip_root->ntasks ) {
      RETURN( ERANGE );
    }
    pip_task = &pip_root->tasks[pipid];
    ntasks    = pip_root->ntasks;
    if( ntasksp != NULL ) *ntasksp = ntasks;
    if( rt_expp != NULL ) {
      *rt_expp = (void*) pip_root->task_root->annex->exp;
    }
    unsetenv( PIP_ROOT_ENV );
    unsetenv( PIP_TASK_ENV );
    pip_task->annex->pip_root_p = (void**) &pip_root;
    pip_task->annex->pip_task_p = &pip_task;

  } else {
    RETURN( EPERM );
  }
  DBGF( "pip_root=%p  pip_task=%p", pip_root, pip_task );
  /* root and child */
  if( pipidp != NULL ) *pipidp = pipid;
  RETURN( err );
}

int pip_fin( void ) {
  int ntasks, i;

  ENTER;
  if( !pip_is_initialized() ) RETURN( EPERM );
  if( pip_isa_root() ) {		/* root */
    ntasks = pip_root->ntasks;
    for( i=0; i<ntasks; i++ ) {
      pip_task_internal_t *taski = &pip_root->tasks[i];
      if( taski->pipid != PIP_PIPID_NULL ) {
	if( taski->flag_exit != PIP_EXIT_WAITED) {
	  DBGF( "%d/%d [pipid=%d (type=0x%x)] -- BUSY",
		i, ntasks, taski->pipid, taski->type );
	  RETURN( EBUSY );
	}
      }
    }
    pip_named_export_fin_all();
    /* report accumulated timer values, if set */
    PIP_REPORT( time_load_dso  );
    PIP_REPORT( time_load_prog );
    PIP_REPORT( time_dlmopen   );

    pip_sem_fin( &pip_root->sync_root );
    pip_sem_fin( &pip_root->sync_task );

    /* SIGCHLD */
    pip_unset_sigmask();
    pip_unset_signal_handler( SIGCHLD,
			      &pip_root->old_sigchld );
    /* SIGTERM */
    pip_unset_signal_handler( SIGTERM,
			      &pip_root->old_sigterm );
    /* deadlock? */
    pip_unset_signal_handler( SIGHUP,
			      &pip_root->old_sighup );

    memset( pip_root, 0, pip_root->size_whole );
    /* after this point DBG(F) macros cannot be used */
    free( pip_root );
    pip_root = NULL;
    pip_task = NULL;
  }
  RETURN( 0 );
}

int pip_export( void *exp ) {
  pip_get_myself()->annex->exp = exp;
  RETURN( 0 );
}

int pip_import( int pipid, void **exportp  ) {
  pip_task_internal_t *taski;
  int err;

  ENTER;
  if( exportp == NULL ) RETURN( EINVAL );
  if( ( err = pip_check_pipid( &pipid ) ) != 0 ) RETURN( err );
  taski = pip_get_task( pipid );
  *exportp = (void*) taski->annex->exp;
  RETURN( 0 );
}

int pip_is_initialized( void ) {
  return pip_task != NULL && pip_root != NULL;
}

int pip_isa_root( void ) {
  return pip_is_initialized() && PIP_ISA_ROOT( pip_task );
}

int pip_isa_task( void ) {
  return
    pip_is_initialized() &&
    PIP_ISA_TASK( pip_task ) && /* root is also a task */
    !PIP_ISA_ROOT( pip_task );
}

int pip_get_pipid( int *pipidp ) {
  int pipid;
  pipid = pip_get_pipid_();
  if( pipid == PIP_PIPID_NULL ) RETURN( EPERM );
  if( pipidp != NULL ) *pipidp = pipid;
  RETURN( 0 );
}

int pip_get_ntasks( int *ntasksp ) {
  if( pip_root == NULL ) return( EPERM  ); /* intentionally small return */
  if( ntasksp != NULL ) {
    *ntasksp = pip_root->ntasks;
  }
  RETURN( 0 );
}

int pip_get_mode( int *modep ) {
  if( pip_root == NULL ) RETURN( EPERM  );
  if( modep != NULL ) {
    *modep = ( pip_root->opts & PIP_MODE_MASK );
  }
  RETURN( 0 );
}

const char *pip_get_mode_str( void ) {
  char *mode;

  if( pip_root == NULL ) return NULL;
  switch( pip_root->opts & PIP_MODE_MASK ) {
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
  case PIP_MODE_PTHREAD:
    mode = PIP_ENV_MODE_PTHREAD;
    break;
  default:
    mode = "(unknown)";
  }
  return mode;
}

int pip_is_threaded_( void ) {
  return pip_root->opts & PIP_MODE_PTHREAD;
}

int pip_is_threaded( int *flagp ) {
  if( pip_is_threaded_() ) {
    *flagp = 1;
  } else {
    *flagp = 0;
  }
  return 0;
}

int pip_is_shared_fd_( void ) {
  return pip_is_threaded_();
}

int pip_is_shared_fd( int *flagp ) {
  if( pip_is_shared_fd_() ) {
    *flagp = 1;
  } else {
    *flagp = 0;
  }
  return 0;
}

int pip_kill_all_tasks( void ) {
  int pipid, err;

  err = 0;
  if( pip_is_initialized() ) {
    if( !pip_isa_root() ) {
      err = EPERM;
    } else {
      for( pipid=0; pipid<pip_root->ntasks; pipid++ ) {
	if( pip_check_pipid( &pipid ) == 0 ) {
	  if( pip_is_threaded_() ) {
	    pip_task_internal_t *taski = &pip_root->tasks[pipid];
	    taski->annex->status = PIP_W_EXITCODE( 0, SIGTERM );
	    (void) pip_kill( pipid, SIGQUIT );
	  } else {
	    (void) pip_kill( pipid, SIGKILL );
	  }
	}
      }
    }
  }
  return err;
}
