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

//#define DEBUG
//#define PRINT_MAPS
//#define PRINT_FDS

/* the EVAL define symbol is to measure the time for calling dlmopen() */
//#define EVAL

#include <limits.h>		/* for PTHREAD_STACK_MIN */
#define PIP_SLEEP_STACKSZ	PTHREAD_STACK_MIN

#include <pip.h>
#include <pip_internal.h>
#include <pip_gdbif.h>

#define ROUNDUP(X,Y)		((((X)+(Y)-1)/(Y))*(Y))

//#define ATTR_NOINLINE		__attribute__ ((noinline))
//#define ATTR_NOINLINE

extern char 		**environ;

/*** note that the following static variables are   ***/
/*** located at each PIP task and the root process. ***/
pip_root_t		*pip_root_ = NULL;
pip_task_internal_t	*pip_task_ = NULL;

static pip_clone_t*	pip_cloneinfo = NULL;

static void pip_set_magic( pip_root_t *root ) {
  memcpy( root->magic, PIP_MAGIC_WORD, PIP_MAGIC_WLEN );
}

static int pip_is_magic_ok( pip_root_t *root ) {
  return root != NULL &&
    strncmp( root->magic, PIP_MAGIC_WORD, PIP_MAGIC_WLEN ) == 0;
}

static int pip_is_version_ok( pip_root_t *root ) {
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
  if( env == NULL || *env == '\0' ) {
    pip_err_mesg( "No PiP root found" );
    ERRJ;
  }
  pip_root_ = (pip_root_t*) strtoll( env, NULL, 16 );
  if( pip_root_ == NULL ) {
    pip_err_mesg( "Invalid PiP root" );
    ERRJ;
  }
  if( !pip_is_magic_ok( pip_root_ ) ) {
    pip_err_mesg( "%s environment not found", PIP_ROOT_ENV );
    ERRJ;
  }
  if( !pip_is_version_ok( pip_root_ ) ) {
    pip_err_mesg( "Version miss-match between PiP root and task" );
    ERRJ;
  }
  return 0;

 error:
  pip_root_ = NULL;
  RETURN( EPROTO );
}

static void pip_blocking_init( pip_blocking_t *blocking ) {
  //(void) sem_init( &blocking->semaphore, 1, 0 );
  (void) sem_init( blocking, 1, 0 );
}

pip_clone_mostly_pthread_t pip_clone_mostly_pthread_ptr = NULL;

static int pip_check_opt_and_env( int *optsp ) {
  int opts   = *optsp;
  int mode   = ( opts & PIP_MODE_MASK );
  int newmod = 0;
  char *env  = getenv( PIP_ENV_MODE );

  enum PIP_MODE_BITS {
    PIP_MODE_PTHREAD_BIT          = 1,
    PIP_MODE_PROCESS_PRELOAD_BIT  = 2,
    PIP_MODE_PROCESS_PIPCLONE_BIT = 4
  } desired = 0;

  if( ( opts & ~PIP_VALID_OPTS ) != 0 ) {
    /* unknown option(s) specified */
    RETURN( EINVAL );
  }

  if( opts & PIP_MODE_PTHREAD &&
      opts & PIP_MODE_PROCESS ) RETURN( EINVAL );
  if( opts & PIP_MODE_PROCESS ) {
    if( ( opts & PIP_MODE_PROCESS_PRELOAD  ) == PIP_MODE_PROCESS_PRELOAD &&
	( opts & PIP_MODE_PROCESS_PIPCLONE ) == PIP_MODE_PROCESS_PIPCLONE){
      RETURN (EINVAL );
    }
  }

  switch( mode ) {
  case 0:
    if( env == NULL ) {
      desired =
	PIP_MODE_PTHREAD_BIT |
	PIP_MODE_PROCESS_PRELOAD_BIT |
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
	PIP_MODE_PROCESS_PRELOAD_BIT|
	PIP_MODE_PROCESS_PIPCLONE_BIT;
    } else if( strcasecmp( env, PIP_ENV_MODE_PROCESS_PRELOAD  ) == 0 ) {
      desired = PIP_MODE_PROCESS_PRELOAD_BIT;
    } else if( strcasecmp( env, PIP_ENV_MODE_PROCESS_PIPCLONE ) == 0 ) {
      desired = PIP_MODE_PROCESS_PIPCLONE_BIT;
    } else if( strcasecmp( env, PIP_ENV_MODE_THREAD  ) == 0 ||
	       strcasecmp( env, PIP_ENV_MODE_PTHREAD ) == 0 ||
	       strcasecmp( env, PIP_ENV_MODE_PROCESS ) == 0 ) {
      /* ignore PIP_MODE=thread in this case */
      desired =
	PIP_MODE_PROCESS_PRELOAD_BIT|
	PIP_MODE_PROCESS_PIPCLONE_BIT;
    } else {
      pip_warn_mesg( "unknown environment setting PIP_MODE='%s'", env );
      RETURN( EPERM );
    }
    break;
  case PIP_MODE_PROCESS_PRELOAD:
    desired = PIP_MODE_PROCESS_PRELOAD_BIT;
    break;
  case PIP_MODE_PROCESS_PIPCLONE:
    desired = PIP_MODE_PROCESS_PIPCLONE_BIT;
    break;
  default:
    pip_warn_mesg( "pip_init() invalid argument opts=0x%x", opts );
    RETURN( EINVAL );
  }

  if( desired & PIP_MODE_PROCESS_PRELOAD_BIT ) {
    /* check if the __clone() systemcall wrapper exists or not */
    if( pip_cloneinfo == NULL ) {
      pip_cloneinfo = (pip_clone_t*) dlsym( RTLD_DEFAULT, "pip_clone_info");
    }
    DBGF( "cloneinfo-%p", pip_cloneinfo );
    if( pip_cloneinfo != NULL ) {
      newmod = PIP_MODE_PROCESS_PRELOAD;
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
      RETURN( EOPNOTSUPP );
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
	pip_warn_mesg( "process mode is requested but pip_clone_info symbol "
		       "is not found in $LD_PRELOAD and "
		       "pip_clone_mostly_pthread() symbol is not found in "
		       "glibc" );
      } else {
	pip_warn_mesg( "process:pipclone mode is requested but "
		       "pip_clone_mostly_pthread() is not found in glibc" );
      }
      RETURN( EOPNOTSUPP );
    }
  }
  if( desired & PIP_MODE_PTHREAD_BIT ) {
    newmod = PIP_MODE_PTHREAD;
    goto done;
  }
  if( newmod == 0 ) {
    pip_warn_mesg( "pip_init() implemenation error. desired = 0x%x", desired );
    RETURN( EOPNOTSUPP );
  }
 done:
  *optsp = ( opts & ~PIP_MODE_MASK ) | newmod;
  RETURN( 0 );
}

static pip_task_internal_t *pip_get_myself_( void ) {
  pip_task_internal_t *taski;
  if( pip_isa_root() ) {
    taski = pip_root_->task_root;
  } else {
    taski = pip_task_;
  }
  return taski;
}

/* internal funcs */

void pip_page_alloc_( size_t sz, void **allocp ) {
  size_t pgsz;

  if( pip_root_ == NULL ) {	/* no pip_root yet */
    pgsz = sysconf( _SC_PAGESIZE );
  } else if( pip_root_->page_size == 0 ) {
    pip_root_->page_size = pgsz = sysconf( _SC_PAGESIZE );
  } else {
    pgsz = pip_root_->page_size;
  }
  sz = ROUNDUP( sz, pgsz );
  ASSERT( posix_memalign( allocp, pgsz, sz ) != 0 &&
	  *allocp == NULL );
}

void pip_reset_task_struct_( pip_task_internal_t *taski ) {
  pip_task_annex_t 	*annex = taski->annex;
  void			*sleep_stack = annex->sleep_stack;
  void			*namexp = annex->named_exptab;

  memset( (void*) taski, 0, sizeof( pip_task_internal_t ) );
  PIP_TASKQ_INIT( &taski->queue  );
  PIP_TASKQ_INIT( &taski->schedq );
  taski->type  = PIP_TYPE_NONE;
  taski->pipid = PIP_PIPID_NULL;

  memset( (void*) annex, 0, sizeof( pip_task_annex_t ) );
  annex->sleep_stack  = sleep_stack;
  annex->named_exptab = namexp;
  annex->tid          = -1; /* pip_gdbif_init_task_struct() refers this */
  taski->annex        = annex;
  PIP_TASKQ_INIT(    &taski->annex->oodq );
  pip_spin_init(     &taski->annex->lock_oodq );
  pip_blocking_init( &taski->annex->sleep );
}

uint32_t pip_check_sync_flag_( uint32_t flags ) {
  if( flags & PIP_SYNC_MASK ) {
    if( ( flags & ~PIP_SYNC_AUTO     ) != ~PIP_SYNC_AUTO     ) return 0;
    if( ( flags & ~PIP_SYNC_BUSYWAIT ) != ~PIP_SYNC_BUSYWAIT ) return 0;
    if( ( flags & ~PIP_SYNC_YIELD    ) != ~PIP_SYNC_YIELD    ) return 0;
    if( ( flags & ~PIP_SYNC_BLOCKING ) != ~PIP_SYNC_BLOCKING ) return 0;
  } else if( !pip_is_initialized() ) {
    char *env = getenv( PIP_ENV_SYNC );
    if( env == NULL ) {
      flags |= PIP_SYNC_AUTO;
    } else if( strcasecmp( env, PIP_ENV_SYNC_AUTO     ) == 0 ) {
      flags |= PIP_SYNC_AUTO;
    } else if( strcasecmp( env, PIP_ENV_SYNC_BUSY     ) == 0 ||
	       strcasecmp( env, PIP_ENV_SYNC_BUSYWAIT ) == 0 ) {
      flags |= PIP_SYNC_BUSYWAIT;
    } else if( strcasecmp( env, PIP_ENV_SYNC_YIELD    ) == 0 ) {
      flags |= PIP_SYNC_YIELD;
    } else if( strcasecmp( env, PIP_ENV_SYNC_BLOCK     ) == 0 ||
	       strcasecmp( env, PIP_ENV_SYNC_BLOCKING  ) == 0 ) {
      flags |= PIP_SYNC_BLOCKING;
    }
  }
  return flags;
}

/* API */

#define PIP_CACHE_ALIGN(X) \
  ( ( (X) + PIP_CACHEBLK_SZ - 1 ) & ~( PIP_CACHEBLK_SZ -1 ) )

int pip_init( int *pipidp, int *ntasksp, void **rt_expp, int opts ) {
  extern int pip_named_export_init_( pip_task_internal_t* );
  size_t	sz;
  char		*envroot = NULL;
  char		*envtask = NULL;
  int		ntasks, pipid;
  int		i, err = 0;

#ifdef MCHECK
  mcheck( NULL );
#endif

  if( pip_root_ != NULL ) RETURN( EBUSY ); /* already initialized */
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
    if( ( opts = pip_check_sync_flag_(   opts ) ) == 0 ) RETURN( EINVAL );

    sz = PIP_CACHE_ALIGN( sizeof( pip_root_t ) ) +
         PIP_CACHE_ALIGN( sizeof( pip_task_internal_t ) * ( ntasks + 1 ) ) +
         PIP_CACHE_ALIGN( sizeof( pip_task_annex_t    ) * ( ntasks + 1 ) );
    pip_page_alloc_( sz, (void**) &pip_root_ );
    (void) memset( pip_root_, 0, sz );

    pip_root_->size_whole = sz;
    pip_root_->size_root  = sizeof( pip_root_t );
    pip_root_->size_task  = sizeof( pip_task_internal_t );
    pip_root_->size_annex = sizeof( pip_task_annex_t );

    DBGF( "sizeof(root):%d  sizoef(task):%d  sizeof(annex):%d",
	  (int) pip_root_->size_root, (int) pip_root_->size_task,
	  (int) pip_root_->size_annex );

    pip_root_->annex = (pip_task_annex_t*)
      ( ((intptr_t)pip_root_) +
	PIP_CACHE_ALIGN( sizeof( pip_root_t ) ) +
	PIP_CACHE_ALIGN( sizeof( pip_task_internal_t ) * ( ntasks + 1 ) ) );

    DBGF( "ROOTROOT (%p)", pip_root_ );

    pip_spin_init( &pip_root_->lock_ldlinux );
    pip_spin_init( &pip_root_->lock_tasks   );

    pipid = PIP_PIPID_ROOT;
    pip_set_magic( pip_root_ );
    pip_root_->version      = PIP_API_VERSION;
    pip_root_->ntasks       = ntasks;
    pip_root_->ntasks_count = 1;
    pip_root_->cloneinfo    = pip_cloneinfo;
    pip_root_->opts         = opts;
    for( i=0; i<ntasks+1; i++ ) {
      pip_root_->tasks[i].annex = &pip_root_->annex[i];
      pip_reset_task_struct_( &pip_root_->tasks[i] );
      pip_named_export_init_( &pip_root_->tasks[i] );
    }
    pip_root_->task_root = &pip_root_->tasks[ntasks];

    pip_task_ = pip_root_->task_root;
    pip_task_->pipid      = pipid;
    pip_task_->type       = PIP_TYPE_ROOT;
    pip_task_->task_sched = pip_task_;

    pip_task_->annex->loaded = dlopen( NULL, RTLD_NOW );
    pip_task_->annex->tid    = pip_gettid();
    pip_task_->annex->thread = pthread_self();
#ifdef PIP_SAVE_TLS
    pip_save_tls( &pip_task_->tls );
#endif
    if( rt_expp != NULL ) {
      pip_task_->annex->exp = *rt_expp;
    }
    pip_root_->stack_size_sleep = PIP_SLEEP_STACKSZ;
    pip_page_alloc_( pip_root_->stack_size_sleep,
		     &pip_task_->annex->sleep_stack );
    if( pip_task_->annex->sleep_stack == NULL ) {
      free( pip_root_ );
      RETURN( err );
    }
    unsetenv( PIP_ROOT_ENV );

    pip_gdbif_initialize_root_( ntasks );

    DBGF( "PiP Execution Mode: %s", pip_get_mode_str() );

  } else if( ( envtask = getenv( PIP_TASK_ENV ) ) != NULL ) {
    /* child task */
    if( ( err = pip_set_root( envroot ) ) != 0 ) RETURN( err );
    pipid = (int) strtoll( envtask, NULL, 10 );
    if( pipid < 0 || pipid >= pip_root_->ntasks ) {
      RETURN( ERANGE );
    }
    pip_task_ = &pip_root_->tasks[pipid];
    ntasks    = pip_root_->ntasks;
    if( ntasksp != NULL ) *ntasksp = ntasks;
    if( rt_expp != NULL ) {
      *rt_expp = (void*) pip_root_->task_root->annex->exp;
    }
    unsetenv( PIP_ROOT_ENV );
    unsetenv( PIP_TASK_ENV );

  } else {
    RETURN( EPERM );
  }
  DBGF( "pip_root=%p  pip_task=%p", pip_root_, pip_task_ );
  /* root and child */
  if( pipidp != NULL ) *pipidp = pipid;
  RETURN( err );
}

int pip_export( void *exp ) {
  pip_get_myself_()->annex->exp = exp;
  RETURN( 0 );
}

int pip_import( int pipid, void **exportp  ) {
  pip_task_internal_t *taski;
  int err;

  ENTER;
  if( exportp == NULL ) RETURN( EINVAL );
  if( ( err = pip_check_pipid_( &pipid ) ) != 0 ) RETURN( err );
  taski = pip_get_task_( pipid );
  *exportp = (void*) taski->annex->exp;
  RETURN( 0 );
}

int pip_is_initialized( void ) {
  return pip_task_ != NULL;
}

int pip_isa_root( void ) {
  return pip_is_initialized() && PIP_ISA_ROOT( pip_task_ );
}

int pip_isa_task( void ) {
  return
    pip_is_initialized() &&
    PIP_ISA_TASK( pip_task_ ) &&
    !PIP_ISA_ROOT( pip_task_ );
}

int pip_is_alive( int pipid ) {
  if( pip_check_pipid_( &pipid ) == 0 ) {
    return pip_is_alive_( pip_get_task_( pipid ) );
  }
  return 0;
}

int pip_get_pipid( int *pipidp ) {
  int pipid;
  pipid = pip_get_pipid_();
  if( pipid == PIP_PIPID_NULL ) RETURN( EPERM );
  if( pipidp != NULL ) *pipidp = pipid;
  RETURN( 0 );
}

int pip_get_ntasks( int *ntasksp ) {
  if( pip_root_ == NULL ) return( EPERM  ); /* intentionally small return */
  if( ntasksp != NULL ) {
    *ntasksp = pip_root_->ntasks;
  }
  RETURN( 0 );
}

int pip_get_curr_ntasks( int *ntasksp ) {
  if( pip_root_ == NULL ) return( EPERM  ); /* intentionally small return */
  if( ntasksp != NULL ) {
    *ntasksp = pip_root_->ntasks_curr;
  }
  RETURN( 0 );
}

int pip_get_mode( int *modep ) {
  if( pip_root_ == NULL ) RETURN( EPERM  );
  if( modep != NULL ) {
    *modep = ( pip_root_->opts & PIP_MODE_MASK );
  }
  RETURN( 0 );
}

const char *pip_get_mode_str( void ) {
  char *mode;

  if( pip_root_ == NULL ) return NULL;
  switch( pip_root_->opts & PIP_MODE_MASK ) {
  case PIP_MODE_PTHREAD:
    mode = PIP_ENV_MODE_PTHREAD;
    break;
  case PIP_MODE_PROCESS:
    mode = PIP_ENV_MODE_PROCESS;
    break;
  case PIP_MODE_PROCESS_PRELOAD:
    mode = PIP_ENV_MODE_PROCESS_PRELOAD;
    break;
  case PIP_MODE_PROCESS_PIPCLONE:
    mode = PIP_ENV_MODE_PROCESS_PIPCLONE;
    break;
  default:
    mode = "(unknown)";
  }
  return mode;
}

extern int pip_is_threaded_( void );
int pip_is_threaded( int *flagp ) {
  if( pip_is_threaded_() ) {
    *flagp = 1;
  } else {
    *flagp = 0;
  }
  return 0;
}

int pip_is_shared_fd( int *flagp ) {
  extern int pip_is_shared_fd_( void );
  if( pip_is_shared_fd_() ) {
    *flagp = 1;
  } else {
    *flagp = 0;
  }
  ASSERT( pip_task_->annex->named_exptab == NULL );
  return 0;
}

int pip_kill( int pipid, int signal ) {
  extern int pip_raise_signal_( pip_task_internal_t*, int );
  int err;

  if( signal < 0 ) RETURN( EINVAL );
  if( ( err = pip_check_pipid_( &pipid ) ) == 0 ) {
    err = pip_raise_signal_( pip_get_task_( pipid ), signal );
  }
  RETURN( err );
}

int pip_sigmask( int how, sigset_t *sigmask, sigset_t *oldmask ) {
  int err;
  if( sigmask == NULL ) RETURN( EINVAL );
  if( pip_is_threaded_() ) {
    err = pthread_sigmask( how, sigmask, oldmask );
  } else {
    err = 0;
    if( sigprocmask( how, sigmask, oldmask ) != 0 ) err = errno;
  }
  RETURN( err );
}

int pip_is_debug_build( void ) {
#ifdef DEBUG
  return 1;
#else
  return 0;
#endif
}
