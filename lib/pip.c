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
 * Written by Atsushi HORI <ahori@riken.jp>, 2016, 2017, 2018
 */

#define _GNU_SOURCE

#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <malloc.h>
#include <signal.h>

//#define PIP_NO_MALLOPT
//#define PIP_USE_MUTEX
//#define PIP_USE_SIGQUEUE
//#define PIP_USE_STATIC_CTX  /* this is slower, adds 30ns */

//#define DEBUG
//#define PRINT_MAPS
//#define PRINT_FDS

/* the EVAL define symbol is to measure the time for calling dlmopen() */
//#define EVAL

#include <pip.h>
#include <pip_ulp.h>
#include <pip_internal.h>
#include <pip_util.h>
#include <pip_gdbif_func.h>

#define ROUNDUP(X,Y)		((((X)+(Y)-1)/(Y))*(Y))

#define IF_LIKELY(C)		if( pip_likely( C ) )
#define IF_UNLIKELY(C)		if( pip_unlikely( C ) )

#define NOINLINE		__attribute__ ((noinline))
//#define NOINLINE

extern char 		**environ;

/*** note that the following static variables are   ***/
/*** located at each PIP task and the root process. ***/
pip_root_t	*pip_root = NULL;
pip_task_t	*pip_task = NULL;

static pip_clone_t*	pip_cloneinfo = NULL;

int  pip_named_export_init( pip_task_t* );
int  pip_named_export_fin(  pip_task_t* );
void pip_named_export_fin_all( void );

static inline int pip_ulp_yield_( pip_task_t* );
static int (*pip_clone_mostly_pthread_ptr) (
	pthread_t *newthread,
	int clone_flags,
	int core_no,
	size_t stack_size,
	void *(*start_routine) (void *),
	void *arg,
	pid_t *pidp) = NULL;

void pip_abort( void );

#ifdef DEBUG
#define STACK_GUARD 	\
  int __stack_guard__ = 1234567
#define STACK_CHECK	\
  do { if( __stack_guard__ != 1234567 ) {				\
	 DBGF( "\nSTACK_GUARD ERROR\n" ); pip_abort(); } } while( 0 )
#else
#define STACK_GUARD
#define STACK_CHECK
#endif

int pip_isa_root( void ) {
  return pip_task != NULL && ( pip_task->type & PIP_TYPE_ROOT );
}

int pip_isa_task( void ) {
  return pip_task != NULL && ( pip_task->type & PIP_TYPE_TASK );
}

int pip_isa_ulp( void ) {
  return pip_task != NULL && ( pip_task->type & PIP_TYPE_ULP );
}

static void pip_set_magic( pip_root_t *root ) {
  memcpy( root->magic, PIP_MAGIC_WORD, PIP_MAGIC_LEN );
}

static int pip_is_magic_ok( pip_root_t *root ) {
  return root != NULL &&
    strncmp( root->magic, PIP_MAGIC_WORD, PIP_MAGIC_LEN ) == 0;
}

static int pip_is_version_ok( pip_root_t *root ) {
  if( root            != NULL        &&
      root->version   == PIP_VERSION &&
      root->root_size == sizeof( pip_root_t ) ) return 1;
  return 0;
}

static int pip_set_root( char *env ) {
  if( env == NULL || *env == '\0' ) {
    pip_err_mesg( "No PiP root found" );
    ERRJ;
  }
  pip_root = (pip_root_t*) strtoll( env, NULL, 16 );
  if( pip_root == NULL ) {
    pip_err_mesg( "Invalid PiP root" );
    ERRJ;
  }
  if( !pip_is_magic_ok(   pip_root ) ) {
    pip_err_mesg( "%s environment not found", PIP_ROOT_ENV );
    ERRJ;
  }
  if( !pip_is_version_ok( pip_root ) ) {
    pip_err_mesg( "Version miss-match between root and child" );
    ERRJ;
  }
  return 0;

 error:
  pip_root = NULL;
  RETURN( EPERM );
}

pip_clone_t *pip_get_cloneinfo_( void ) {
  return pip_root->cloneinfo;
}

int pip_page_alloc( size_t sz, void **allocp ) {
  size_t pgsz;

  ENTER;
  if( pip_root == NULL ) {	/* no pip_root yet */
    pgsz = sysconf( _SC_PAGESIZE );
  } else if( pip_root->page_size == 0 ) {
    pip_root->page_size = pgsz = sysconf( _SC_PAGESIZE );
  } else {
    pgsz = pip_root->page_size;
  }
  sz = ROUNDUP( sz, pgsz );
  RETURN( posix_memalign( allocp, pgsz, sz ) );
}

static void pip_blocking_init( pip_blocking_t *blocking ) {
#ifdef PIP_USE_MUTEX
  (void) pthread_mutex_init( &blocking->mutex, NULL );
#else
  (void) sem_init( &blocking->semaphore, 1, 0 );
#endif
}

static void pip_blocking_fin( pip_blocking_t *blocking ) {
#ifdef PIP_USE_MUTEX
  (void) pthread_mutex_destroy( &blocking->mutex, NULL );
#else
  (void) sem_destroy( &blocking->semaphore );
#endif
}

static inline int pip_tryblock( pip_blocking_t *blocking ) {
  int err = 0;
#ifdef PIP_USE_MUTEX
  if( ( err = pthread_mutex_trylock( &blocking->mutex ) ) == 0 &&
      ( err = pthread_mutex_unlock(  &blocking->mutex ) ) == 0 );
#else
  if( sem_trywait( &blocking->semaphore ) != 0 ) {
    if( errno != EINTR ) err = errno;
  }
#endif
  RETURN( err );
}

static inline int pip_block( pip_blocking_t *blocking ) {
  int err = 0;

#ifdef PIP_USE_MUTEX
  if( ( err = pthread_mutex_lock(    &blocking->mutex ) ) == 0 &&
      ( err = pthread_mutex_unlock(  &blocking->mutex ) ) == 0 );
#else
  while( 1 ) {
    if( sem_wait( &blocking->semaphore ) == 0 ) break;
    if( errno != EINTR ) {
      err = errno;
      break;
    }
  }
#endif
  return err;
}

static int pip_is_threaded_( void ) {
  return (pip_root->opts & PIP_MODE_PTHREAD) != 0 ? CLONE_THREAD : 0;
}

static int pip_raise_signal( pip_task_t *task, int sig ) {
  int err = 0;

  ENTER;
  if( !pip_is_threaded_() ) {
    if( kill( task->pid, sig ) != 0 ) err = errno;
  } else {
#ifndef PIP_USE_SIGQUEUE
    if( pthread_kill( task->thread, sig ) == ESRCH ) {
      if( kill( task->pid, sig ) != 0 ) err = errno;
    }
#else
    if( pthread_sigqueue( task->thread,
			  sig,
			  (const union sigval)0 ) == ESRCH ) {
      if( sigqueue( task->pid,
		    sig,
		    (const union sigval) 0 ) != 0 ) {
	err = errno;
      }
    }
#endif
  }
  RETURN( err );
}

static int pip_raise_sigchld( void ) {
  ENTER;
  RETURN( pip_raise_signal( pip_root->task_root, SIGCHLD ) );
}

int pip_count_awake_tasks( void ) {
  int i, c;

  if( pip_root == NULL ) return 0;
  for( i=0, c=0; i<pip_root->ntasks; i++ ) {
    if( pip_root->tasks[i].pipid != PIP_PIPID_NULL ) c ++;
  }
  return c;
}

static void pip_reset_task_struct( pip_task_t *task ) {
  memset( (void*) task, 0, sizeof( pip_task_t ) );
  task->pid  = -1; /* pip_gdbif_init_task_struct() refers this */
  PIP_ULP_INIT( &task->queue  );
  PIP_ULP_INIT( &task->schedq );
  pip_spin_init( &task->lock_wakeup );
  pip_spin_init( &task->lock_malloc );
  pip_blocking_init( &task->sleep );
  pip_memory_barrier();
  task->type  = PIP_TYPE_NONE;
  task->pipid = PIP_PIPID_NULL;
}

int pip_count_vec( char **vecsrc ) {
  int n = 0;
  if( vecsrc != NULL ) {
    for( ; vecsrc[n]!= NULL; n++ );
  }
  return( n );
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
  case PIP_MODE_PROCESS_PIPCLONE:
    mode = PIP_ENV_MODE_PROCESS_PIPCLONE;
    break;
  default:
    mode = "(unknown)";
  }
  return mode;
}

static void *pip_dlsym( void *handle, const char *name ) {
  void *addr;
  pip_spin_lock( &pip_root->lock_ldlinux );
  do {
    (void) dlerror();		/* reset error status */
    if( ( addr = dlsym( handle, name ) ) == NULL ) {
      DBGF( "dlsym(%p,%s): %s", handle, name, dlerror() );
    }
  } while( 0 );
  pip_spin_unlock( &pip_root->lock_ldlinux );
  return( addr );
}

static void pip_dlclose( void *handle ) {
#ifdef AH
  pip_spin_lock( &pip_root->lock_ldlinux );
  do {
    dlclose( handle );
  } while( 0 );
  pip_spin_unlock( &pip_root->lock_ldlinux );
#endif
}

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
      RETURN( EPERM );
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
    goto done;
  }
  if( newmod == 0 ) {
    pip_warn_mesg( "pip_init() implemenation error. desired = 0x%x", desired );
    RETURN( EPERM );
  }
 done:
  if( ( opts & ~PIP_MODE_MASK ) == 0 ) {
    if( ( env = getenv( PIP_ENV_OPTS ) ) != NULL ) {
      if( strcasecmp( env, PIP_ENV_OPTS_FORCEEXIT ) == 0 ) {
	opts |= PIP_OPT_FORCEEXIT;
      } else {
	pip_warn_mesg( "Unknown option %s=%s", PIP_ENV_OPTS, env );
	RETURN( EPERM );
      }
    }
  }
  *optsp = ( opts & ~PIP_MODE_MASK ) | newmod;
  RETURN( 0 );
}

int pip_init( int *pipidp, int *ntasksp, void **rt_expp, int opts ) {
  size_t	sz;
  char		*envroot = NULL;
  char		*envtask = NULL;
  int		ntasks;
  int 		pipid;
  int 		i, err = 0;

  if( pip_root != NULL ) RETURN( EBUSY ); /* already initialized */
  if( ( envroot = getenv( PIP_ROOT_ENV ) ) == NULL ) {
    /* root process */

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
    if( ( err = pip_page_alloc( sz, (void**) &pip_root ) ) != 0 ) {
      RETURN( err );
    }
    (void) memset( pip_root, 0, sz );
    pip_root->root_size = sizeof( pip_root_t );
    pip_root->size      = sz;

    DBGF( "ROOTROOT (%p)", pip_root );

    pip_spin_init( &pip_root->lock_ldlinux     );
    pip_spin_init( &pip_root->lock_tasks       );
    /* beyond this point, we can call the       */
    /* pip_dlsymc() and pip_dlclose() functions */

    pipid = PIP_PIPID_ROOT;
    pip_set_magic( pip_root );
    pip_root->version   = PIP_VERSION;
    pip_root->ntasks    = ntasks;
    pip_root->cloneinfo = pip_cloneinfo;
    pip_root->opts      = opts;
    for( i=0; i<ntasks+1; i++ ) {
      pip_reset_task_struct( &pip_root->tasks[i] );
    }
    pip_root->task_root = &pip_root->tasks[ntasks];
    pip_task               = pip_root->task_root;
    pip_task->pipid        = pipid;
    pip_task->type         = PIP_TYPE_ROOT;
    pip_task->loaded       = dlopen( NULL, RTLD_NOW );
    pip_task->thread       = pthread_self();
    pip_task->pid          = pip_gettid();
    pip_task->pid_actual   = pip_task->pid;
    if( rt_expp != NULL ) {
      pip_task->export     = *rt_expp;
    }
    pip_task->task_sched   = pip_task;
    if( ( err = pip_named_export_init( pip_task ) ) != 0 ) {
      free( pip_root );
      RETURN( err );
    }
    pip_spin_init( &pip_task->lock_malloc );
    unsetenv( PIP_ROOT_ENV );

    if( ( err = pip_gdbif_initialize_root( ntasks ) ) != 0 ) {
      free( pip_root );
      RETURN( err );
    }

    DBGF( "PiP Execution Mode: %s", pip_get_mode_str() );

  } else if( ( envtask = getenv( PIP_TASK_ENV ) ) != NULL ) {
    /* child task */
    DBG;
    if( ( err = pip_set_root( envroot ) ) != 0 ) RETURN( err );
    DBG;
    pipid = (int) strtoll( envtask, NULL, 10 );
    DBG;
    if( pipid < 0 || pipid >= pip_root->ntasks ) {
      pip_err_mesg( "PiP-ID is too large (pipid=%d)" );
      RETURN( EPERM );
    }
    DBG;
    pip_task = &pip_root->tasks[pipid];
    ntasks   = pip_root->ntasks;
    if( ntasksp != NULL ) *ntasksp = ntasks;
    if( rt_expp != NULL ) *rt_expp = (void*) pip_root->task_root->export;
    DBG;
    if( ( err = pip_named_export_init( pip_task ) ) != 0 ) RETURN( err );
    unsetenv( PIP_ROOT_ENV );
    unsetenv( PIP_TASK_ENV );

  } else {
    DBG;
    RETURN( EPERM );
  }
  DBGF( "pip_root=%p  pip_task=%p", pip_root, pip_task );
  /* root and child */
  if( pipidp != NULL ) *pipidp = pipid;
  RETURN( err );
}

/* The following functions;
   pip_is_threaded(),
   pip_is_shared_fd_(),
   pip_is_shared_fd(), and
   can be called from any context
*/

int pip_is_threaded( int *flagp ) {
  if( pip_root == NULL ) RETURN( EPERM  );
  if( flagp    == NULL ) RETURN( EINVAL );
  *flagp = pip_is_threaded_();
  RETURN( 0 );
}

static int pip_is_shared_fd_( void ) {
  if( pip_root->cloneinfo == NULL )
    return (pip_root->opts & PIP_MODE_PTHREAD) != 0 ? CLONE_FILES : 0;
  return pip_root->cloneinfo->flag_clone & CLONE_FILES;
}

int pip_is_shared_fd( int *flagp ) {
  if( pip_root == NULL ) RETURN( EPERM  );
  if( flagp    == NULL ) RETURN( EINVAL );
  *flagp = pip_is_shared_fd_();
  RETURN( 0 );
}

int pip_isa_piptask( void ) {
  /* this function might be called before calling pip_init() */
  return getenv( PIP_ROOT_ENV ) != NULL;
}

int pip_check_pipid( int *pipidp ) {
  int pipid = *pipidp;

  if( pip_root == NULL          ) RETURN( EPERM  );
  if( pipid >= pip_root->ntasks ) RETURN( EINVAL );
  if( pipid != PIP_PIPID_MYSELF &&
      pipid <  PIP_PIPID_ROOT   ) RETURN( EINVAL );
  ENTER;
  switch( pipid ) {
  case PIP_PIPID_ROOT:
    break;
  case PIP_PIPID_ANY:
    RETURN( EINVAL );
  case PIP_PIPID_MYSELF:
    if( pip_isa_root() ) {
      *pipidp = PIP_PIPID_ROOT;
    } else if( pip_isa_task() || pip_isa_ulp() ) {
      *pipidp = pip_task->pipid;
    } else {
      RETURN( ENXIO );
    }
    break;
  }
  RETURN( 0 );
}

pip_task_t *pip_get_task_( int pipid ) {
  pip_task_t 	*task = NULL;

  switch( pipid ) {
  case PIP_PIPID_ROOT:
    task = pip_root->task_root;
    break;
  default:
    task = &pip_root->tasks[pipid];
    break;
  }
  return task;
}

int pip_is_alive( int pipid ) {
  if( pip_check_pipid( &pipid ) == 0 ) {
    pip_task_t	*task = pip_get_task_( pipid );
    return task->pipid == pipid && !task->flag_exit;
  }
  return 0;
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
  if( pip_task == NULL ) return PIP_PIPID_NULL;
  return pip_task->pipid;
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

static pip_task_t *pip_get_myself_( void ) {
  pip_task_t *task;
  if( pip_isa_root() ) {
    task = pip_root->task_root;
  } else {
    task = pip_task;
  }
  return task;
}

int pip_export( void *export ) {
  pip_get_myself_()->export = export;
  RETURN( 0 );
}

int pip_import( int pipid, void **exportp  ) {
  pip_task_t *task;
  int err;

  if( exportp == NULL ) RETURN( EINVAL );
  if( ( err = pip_check_pipid( &pipid ) ) != 0 ) RETURN( err );
  task = pip_get_task_( pipid );
  *exportp = (void*) task->export;
  if( *exportp == NULL ) pip_pause();
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
  } else {
    err = ESRCH;		/* tentative */
  }
  RETURN( err );
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
  ASSERT( ( (intptr_t)p < (intptr_t)(vecdst+sz) ) );
  if( addition1 ) {
    vecdst[j++] = p;
    p = stpcpy( p, addition1 ) + 1;
  }
  ASSERT( ( (intptr_t)p < (intptr_t)(vecdst+sz) ) );
  for( i=0; vecsrc[i]!=NULL; i++ ) {
    vecdst[j++] = p;
    p = stpcpy( p, vecsrc[i] ) + 1;
    ASSERT( ( (intptr_t)p < (intptr_t)(vecdst+sz) ) );
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

  if( sprintf( rootenv, "%s=%p", PIP_ROOT_ENV, pip_root ) <= 0 ||
      sprintf( taskenv, "%s=%d", PIP_TASK_ENV, pipid    ) <= 0 ) {
    return NULL;
  }
  return pip_copy_vec3( rootenv, taskenv, preload_env, envsrc );
}

size_t pip_stack_size( void ) {
  char 		*env, *endptr;
  size_t 	sz, scale;
  int 		i;

  if( ( sz = pip_root->stack_size ) == 0 ) {
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
      if( ( fd = atoi( direntp->d_name ) ) >= 0 &&
	  fd != fd_dir && pip_is_coefd( fd ) ) {
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
  symp->named_export_fin = dlsym( handle, "pip_named_export_fin"         );
  /* unused */
  symp->free             = dlsym( handle, "free"                         );
  symp->glibc_init       = dlsym( handle, "glibc_init"                   );
  /* pip_named_export_fin symbol may not be found when the task
     program is not linked with the PiP lib. (due to not calling
     any PiP functions) */
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

static int pip_load_prog( pip_spawn_program_t *progp, pip_task_t *task ) {
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
    if( ( err = pip_find_symbols( progp, loaded, &task->symbols ) ) != 0 ) {
      DBG;
      (void) pip_dlclose( loaded );
    } else {
      DBG;
      task->loaded = loaded;
      pip_gdbif_load( task );
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

static void pip_task_terminated( pip_task_t *task, int extval ) {
  /* call fflush() in the target context to flush out messages */
  ENTER;
  if( task->symbols.libc_fflush != NULL ) {
    // DBGF( "[%d] fflush@%p() >>", task->pipid, task->symbols.libc_fflush );
    task->symbols.libc_fflush( NULL );
    //DBGF( "[%d] fflush@%p() <<", task->pipid, task->symbols.libc_fflush );
  }
  if( task->symbols.named_export_fin != NULL ) {
    /* pip_named_export_fin symbol may not be found when the task
       program is not linked with the PiP lib. (due to not calling
       any PiP functions) */
    /* In this case, the PiP task never calls pip_named_* functions
       and no need of free() */
    (void) task->symbols.named_export_fin( task );
  }
  if( !task->flag_exit ) {
    //DBGF( "[%d] extval=%d", task->pipid, extval );
    task->flag_exit = PIP_EXITED;
    task->extval    = extval;
    pip_gdbif_exit( task, extval );
  }
  RETURNV;
}

static void pip_force_exit( pip_task_t *task ) {
  ENTER;
  if( pip_is_threaded_() ) {
    (void) pip_raise_sigchld();
    pthread_exit( NULL );
  } else {			/* process mode */
    DBGF( "about to call exit()" );
    exit( task->extval );
  }
}

static void pip_let_prev_task_go( pip_task_t *task ) {
  ENTER;
  if( task->lock_unlock != NULL ) {
    DBGF( "UNLCKD:%p", task->lock_unlock );
    pip_spin_unlock( task->lock_unlock ); /* UNLOCK */
    task->lock_unlock = NULL;
  }
  RETURNV;
}

static int
pip_ulp_switch_( pip_task_t *old_task, pip_ulp_t *ulp ) {
  pip_task_t	*new_task = PIP_TASK( ulp );
  pip_ctx_t	*newctxp;
#ifndef PIP_USE_STATIC_CTX
  pip_ctx_t 	oldctx;
#endif
  int		err = 0;

  if( old_task == new_task ) RETURN( 0 );
  ENTER;
#ifdef PIP_USE_STATIC_CTX
  old_task->ctx_suspend = old_task->context;
#else
  old_task->ctx_suspend = &oldctx;
#endif
  newctxp = new_task->ctx_suspend;
  new_task->ctx_suspend = newctxp;
  DBGF( "old:%p (PIPID:%d)  new:%p (PIPID:%d)",
	old_task->ctx_suspend, old_task->pipid,
	newctxp, new_task->pipid );
  (void) pip_load_tls( new_task->tls );
  err = pip_swap_context( old_task->ctx_suspend, newctxp );
  pip_let_prev_task_go( old_task );
  RETURN( err );
}

static int pip_ulp_sched_next( pip_task_t *sched, pip_spinlock_t *lockp ) {
  pip_ulp_t 	*queue, *next;
  pip_task_t 	*nxt_task;
  pip_ctx_t 	*nxt_ctxp;
  int		err;

  queue = &sched->schedq;
  next = PIP_ULP_NEXT( queue );
  PIP_ULP_DEQ( next );

  nxt_task = PIP_TASK( next );
  nxt_task->lock_unlock = lockp; /* to unlock as soon as it is switched */
  nxt_ctxp = nxt_task->ctx_suspend;
  DBGF( "sched-pipid:%d  next:%p  nxt_ctxp:%p  Next-PIPID:%d  lock:%p",
	sched->pipid, nxt_task, nxt_ctxp, nxt_task->pipid, lockp );
  if( ( err = pip_load_tls( nxt_task->tls ) ) == 0 ) {
    if( ( err = pip_load_context( nxt_ctxp ) ) != 0 ) {
      (void) pip_load_tls( pip_task->tls );
    }
    /* never reach here, hopefully */
  }
  return err;
}

static int pip_wakeup_( pip_task_t *task ) {
  ENTER;
  if( sem_post( &task->sleep.semaphore ) == 0 ) RETURN( 0 );
  RETURN( errno );
}

static int pip_wakeup_and_sched_next( pip_task_t *task ) {
  pip_task_t     *sched = task->task_sched;
  pip_spinlock_t *lockp = &task->lock_wakeup;
  int err = 0;

  ENTER;
  pip_spin_lock( lockp );
  if( ( err = pip_wakeup_( task ) ) == 0 ) {
    if( !PIP_ULP_ISEMPTY( &sched->schedq ) ) {
      err = pip_ulp_sched_next( sched, lockp );
    } else {
      /* the lock must be unlocked because there is       */
      /* no task or ULP to switch to and  unlock the lock */
      pip_spin_unlock( lockp );
    }
  }
  RETURN( err );
}

static int pip_wakeup_to_terminate( pip_task_t *task, int extval ) {
  /* race condition!!! */
  /* when pip_task_terminated() is called and the ULP is wakeup to  */
  /* terminate then the task structure might be cleared by the root */
  /* and the task->task_sched will be lost. So its value must be    */
  /* obtained before tehn  */
  pip_task_t *sched = task->task_sched;
  int 	     err = 0;

  ENTER;
  pip_task_terminated( task, extval );
  if( ( err = pip_wakeup_( task ) ) == 0 ) {
    if( !PIP_ULP_ISEMPTY( &sched->schedq ) ) {
      err = pip_ulp_sched_next( sched, NULL );
    }
  }
  RETURN( err );
}

static int pip_glibc_init( pip_symbols_t *symbols,
			   char *prog,
			   char **argv,
			   char **envv,
			   void *loaded ) {
  int argc = pip_count_vec( argv );

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

static int pip_jump_into( pip_spawn_args_t *args, pip_task_t *self ) {
  char  	*prog      = args->prog;
  char 		**argv     = args->argv;
  char 		**envv     = args->envv;
  void		*start_arg = args->start_arg;
  int 		argc;
  int		extval;

  argc = pip_glibc_init( &self->symbols, prog, argv, envv, self->loaded );
  if( self->symbols.start == NULL ) {
    DBGF( "[%d] >> main@%p(%d,%s,%s,...)",
	  args->pipid, self->symbols.main, argc, argv[0], argv[1] );
    extval = self->symbols.main( argc, argv, envv );
    DBGF( "[%d] << main@%p(%d,%s,%s,...) = %d",
	  args->pipid, self->symbols.main, argc, argv[0], argv[1], extval );
  } else {
    DBGF( "[%d] >> %s:%p(%p)",
	  args->pipid, args->funcname, self->symbols.start, start_arg );
    extval = self->symbols.start( start_arg );
    DBGF( "[%d] << %s:%p(%p) = %d",
	  args->pipid, args->funcname, self->symbols.start, start_arg, extval);
  }
  return extval;
}

static void* NOINLINE /* THIS FUNC MUST NOT BE INLINED */
pip_do_spawn( void *thargs )  {
  /* The context of this function is of the root task                */
  /* so the global var; pip_task (and pip_root) are of the root task */
  pip_spawn_args_t *args   = (pip_spawn_args_t*) thargs;
  int	 	pipid      = args->pipid;
  char 		**argv     = args->argv;
  int 		coreno     = args->coreno;
  pip_task_t 	*self      = &pip_root->tasks[pipid];
  pip_spawnhook_t before   = self->hook_before;
  void 		*hook_arg  = self->hook_arg;
  pip_ctx_t	context;
  int		extval = 0;
  int		err = 0;

  ENTER;
  self->context = &context;
  if( ( err = pip_corebind( coreno ) ) != 0 ) {
    pip_warn_mesg( "failed to bund CPU core (%d)", err );
  }
  self->thread = pthread_self();
  pip_save_tls( &self->tls );	/* save tls register */

#ifdef DEBUG
  if( pip_is_threaded_() ) {
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
  /* calling hook, if any */
  if( before != NULL && ( err = before( hook_arg ) ) != 0 ) {
    pip_warn_mesg( "[%s] before-hook returns %d", argv[0], before, err );
    extval = err;
  } else {
    if( !pip_is_threaded_() ) {
      (void) setpgid( 0, pip_root->task_root->pid );
    }
    extval = pip_jump_into( args, self );
    /* the after hook is supposed to free the hook_arg being malloc()ed */
    /* and this is the reason to call this from the root process        */
    if( self->hook_after != NULL ) (void) self->hook_after( self->hook_arg );
    /* free() must be called in the task's context */
  }
  if( self->type & PIP_TYPE_ULP ) {
    /* ULP */
    DBGF( "ULP" );
    (void) pip_wakeup_to_terminate( self, extval );
    /* never reach here */
  } else {
    /* PiP task */
    DBGF( "TASK" );
    while( !PIP_ULP_ISEMPTY( &self->schedq ) ) {
      /* waiting for the ULP terminations */
      (void) pip_ulp_yield_( self );
    }
    pip_task_terminated( self, extval );
    if( pip_root->opts & PIP_OPT_FORCEEXIT ) {
      pip_force_exit( self );
      /* never reach here */
    } else if( pip_is_threaded_() ) {
      (void) pip_raise_sigchld(); /* to simulate SIGCHLD */
    }
  }
  return NULL;
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

int pip_task_spawn( pip_spawn_program_t *progp,
		    int coreno,
		    int *pipidp,
		    pip_spawn_hook_t *hookp ) {
  cpu_set_t 		cpuset;
  pip_spawn_args_t	*args = NULL;
  pip_task_t		*task = NULL;
  size_t		stack_size = pip_stack_size();
  int 			pipid;
  pid_t			pid = 0;
  int 			err = 0;

  ENTER;
  if( pip_root        == NULL ) RETURN( EPERM  );
  if( pipidp          == NULL ) RETURN( EINVAL );
  if( !pip_isa_root()         ) RETURN( EPERM  );
  if( progp           == NULL ) RETURN( EINVAL );
  if( progp->funcname != NULL &&
      progp->argv     != NULL ) RETURN( EINVAL );
  if( progp->funcname == NULL &&
      progp->argv     == NULL ) RETURN( EINVAL );
  if( progp->funcname == NULL &&
      progp->prog     == NULL ) progp->prog = progp->argv[0];

  pipid = *pipidp;
  if( ( err = pip_find_a_free_task( &pipid ) ) != 0 ) ERRJ;
  task = &pip_root->tasks[pipid];
  pip_reset_task_struct( task );
  task->pipid      = pipid;	/* mark it as occupied */
  task->type       = PIP_TYPE_TASK;
  task->task_sched = task;
  if( hookp != NULL ) {
    task->hook_before = hookp->before;
    task->hook_after  = hookp->after;
    task->hook_arg    = hookp->hookarg;
  }
  DBG;
  if( progp->envv == NULL ) progp->envv = environ;
  args = &task->args;
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

  pip_spin_lock( &pip_root->lock_ldlinux );
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
  pip_spin_unlock( &pip_root->lock_ldlinux );
  ERRJ_CHK(err);

  pip_gdbif_task_new( task );

  if( ( pip_root->opts & PIP_MODE_PROCESS_PIPCLONE ) ==
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

    err = pip_clone_mostly_pthread_ptr( &task->thread,
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
      DBGF( "tid=%d  cloneinfo@%p", tid, pip_root->cloneinfo );
      if( pip_root->cloneinfo != NULL ) {
	/* lock is needed, because the preloaded clone()
	   might also be called from outside of PiP lib. */
	pip_spin_lock_wv( &pip_root->cloneinfo->lock, tid );
      }
      do {
	err = pthread_create( &task->thread,
			      &attr,
			      (void*(*)(void*)) pip_do_spawn,
			      (void*) args );
	DBGF( "pthread_create()=%d", errno );
      } while( 0 );
      /* unlock is done in the wrapper function */
      if( pip_root->cloneinfo != NULL ) {
	pid = pip_root->cloneinfo->pid_clone;
	pip_root->cloneinfo->pid_clone = 0;
      }
    }
  }
  if( err == 0 ) {
    task->pid = pid;
    pip_root->ntasks_accum ++;
    pip_root->ntasks_curr  ++;
    pip_gdbif_task_commit( task );
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
      if( task->loaded != NULL ) (void) pip_dlclose( task->loaded );
      pip_reset_task_struct( task );
    }
  }
  DBGF( "pip_spawn_(pipid=%d)", *pipidp );
  RETURN( err );
}

/*
 * The following functions must be called at root process
 */

static void pip_finalize_task( pip_task_t *task, int *extvalp ) {
  DBGF( "pipid=%d  extval=%d", task->pipid, task->extval );

  pip_gdbif_finalize_task( task );
  if( extvalp != NULL ) *extvalp = ( task->extval & 0xFF );

  /* dlclose() and free() must be called only from the root process since */
  /* corresponding dlmopen() and malloc() is called by the root process   */
  if( task->loaded      != NULL ) pip_dlclose( task->loaded );
  if( task->args.prog   != NULL ) free( task->args.prog );
  if( task->args.argv   != NULL ) free( task->args.argv );
  if( task->args.envv   != NULL ) free( task->args.envv );
  if( task->sleep_stack != NULL ) free( task->sleep_stack );
  if( *task->symbols.progname_full != NULL ) {
    free( *task->symbols.progname_full );
  }
  pip_blocking_fin( &task->sleep );
  pip_reset_task_struct( task );
}

int pip_fin( void ) {
  int ntasks, i, err = 0;

  ENTER;
  if( pip_root == NULL ) RETURN( EPERM );
  if( pip_isa_root() ) {		/* root */
    ntasks = pip_root->ntasks;
    for( i=0; i<ntasks; i++ ) {
      pip_task_t *task = &pip_root->tasks[i];
      if( task->pipid != PIP_PIPID_NULL ) {
	if( !task->flag_exit ) {
	  DBGF( "%d/%d [pipid=%d (type=0x%x)] -- BUSY",
		i, ntasks, task->pipid, task->type );
	  RETURN( EBUSY );
	} else {
	  /* pip_wait*() was not called */
	  pip_finalize_task( task, NULL );
	}
      }
    }
    pip_named_export_fin_all();
    /* report accumulated timer values, if set */
    PIP_REPORT( time_load_dso  );
    PIP_REPORT( time_load_prog );
    PIP_REPORT( time_dlmopen   );
    /* after this point DBG(F) macros cannot be used */
    memset( pip_root, 0, pip_root->size );
    free( pip_root );
    pip_root = NULL;
    pip_task = NULL;
  } else {			/* tasks and ULPs */
    err = pip_named_export_fin( pip_task );
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

  DBGF( "pip_spawn()" );
  pip_spawn_from_main( &program, prog, argv, envv );
  pip_spawn_hook( &hook, before, after, hookarg );
  return pip_task_spawn( &program, coreno, pipidp, &hook );
}

int pip_get_mode( int *mode ) {
  if( pip_root == NULL ) RETURN( EPERM  );
  if( mode     == NULL ) RETURN( EINVAL );
  *mode = ( pip_root->opts & PIP_MODE_MASK );
  RETURN( 0 );
}

int pip_get_id( int pipid, intptr_t *pidp ) {
  pip_task_t *task;
  int err;

  if( ( err = pip_check_pipid( &pipid ) ) != 0 ) RETURN( err );
  if( pipid == PIP_PIPID_ROOT ) RETURN( EINVAL );
  if( pidp  == NULL           ) RETURN( EINVAL );

  task = pip_get_task_( pipid );
  if( pip_is_threaded_() ) {
    /* Do not use gettid(). This is a very Linux-specific function */
    /* The reason of supporintg the thread PiP execution mode is   */
    /* some OSes other than Linux does not support clone()         */
    if( !( task->type & PIP_TYPE_ULP ) ) {
      *pidp = (intptr_t) task->thread; /* task and root */
    } else {
      *pidp = (intptr_t) task->task_sched->thread;
    }
  } else {
    if( !( task->type & PIP_TYPE_ULP ) ) { /* task and root */
      *pidp = (intptr_t) task->pid;
    } else {			/* ULP */
      *pidp = (intptr_t) task->task_sched->pid;
    }
  }
  RETURN( 0 );
}

int pip_kill( int pipid, int signal ) {
  pip_task_t *task;
  int err = 0;

  if( ( err = pip_check_pipid( &pipid ) ) != 0 ) RETURN( err );
  if( signal < 0 ) RETURN( EINVAL );

  task = pip_get_task_( pipid );
  if( task->type & PIP_TYPE_ROOT ||
      task->type & PIP_TYPE_TASK ) {
    if( pip_is_threaded_() ) {
      err = pthread_kill( task->thread, signal );
      DBGF( "pthread_kill(sig=%d)=%d", signal, err );
    } else {
      if( kill( task->pid, signal ) != 0 ) err = errno;
      DBGF( "kill(sig=%d)=%d", signal, err );
    }
  } else {			/* signal cannot be sent to ULPs */
    err = EPERM;
  }
  RETURN( err );
}

int pip_exit( int extval ) {
  pip_task_t *sched;
  int err = 0;

  DBGF( "extval:%d", extval );
  if( pip_root == NULL ) {
    /* not in a PiP environment, i.e., pip_init() has not been called */
    exit( extval );
  }
  /* PiP task or ULP */
  if( pip_isa_ulp() ) {
    pip_task_terminated( pip_task, extval );
    if( PIP_ULP_ISEMPTY( &pip_task->schedq ) ) {
      err = EDEADLK;
    } else {
      err = pip_wakeup_and_sched_next( pip_task );
    }
    /* never reach here, hopefully */
  } else if( pip_isa_task() ) {
    sched = pip_task->task_sched;
    while( !PIP_ULP_ISEMPTY( &sched->schedq ) ) {
      /* task_sched of PiP task is never changed */
      (void) pip_ulp_yield_( pip_task );
    }
    /* if there is no other ULP eligible to run, then terminate itself */
    pip_task_terminated( pip_task, extval );
    pip_force_exit( pip_task );
    /* never reach here */
  } else {
    if( ( err = pip_fin() ) == 0 ) {
      exit( extval );
      /* never reach here */
    }
  }
  RETURN( err );
}

static int pip_do_wait_( int pipid, int flag_try, int *extvalp ) {
  pip_task_t *task;
  int err = 0;

  task = pip_get_task_( pipid );
  if( task->type == PIP_TYPE_NONE ) RETURN( ESRCH );

  DBGF( "pipid=%d  type=0x%x", pipid, task->type );
  if( pip_is_threaded_() ) {
    /* thread mode */
    DBG;
    if( flag_try ) {
      err = pthread_tryjoin_np( task->thread, NULL );
      DBGF( "pthread_tryjoin_np(%d)=%d", task->extval, err );
    } else {
      err = pthread_join( task->thread, NULL );
      DBGF( "pthread_join(%d)=%d", task->extval, err );
    }
  } else {
    /* process mode */
    int status  = 0;
    int options = 0;
    pid_t pid;

#ifdef __WALL
    /* __WALL: Wait for all children, egardless oftype */
    /* ("clone" or "non-clone") [from man page]        */
    options |= __WALL;
#endif
    if( flag_try ) options |= WNOHANG;

    DBGF( "calling waitpid()   task:%p  task->pid:%d  pipid:%d",
	  task, task->pid, task->pipid );
    if( ( pid = waitpid( task->pid, &status, options ) ) < 0 ) {
      err = errno;
    } else {
      int extval;
      if( WIFEXITED( status ) ) {
	extval = WEXITSTATUS( status );
	pip_task_terminated( task, extval );
      } else if( WIFSIGNALED( status ) ) {
  	int sig = WTERMSIG( status );
	pip_warn_mesg( "PiP Task [%d] terminated by %s signal",
		       task->pipid, strsignal(sig) );
	extval = sig + 128;
	pip_task_terminated( task, extval );
      }
      DBGF( "wait(status=%x)=%d (errno=%d)", status, pid, err );
    }
  }
  if( err == 0 ) pip_finalize_task( task, extvalp );
  RETURN( err );
}

static int pip_do_wait( int pipid, int flag_try, int *extvalp ) {
  int err;

  if( !pip_isa_root() )                          RETURN( EPERM   );
  if( ( err = pip_check_pipid( &pipid ) ) != 0 ) RETURN( err     );
  if( pipid == PIP_PIPID_ROOT )                  RETURN( EDEADLK );

  RETURN( pip_do_wait_( pipid, flag_try, extvalp ) );
}

int pip_wait( int pipid, int *extvalp ) {
  RETURN( pip_do_wait( pipid, 0, extvalp ) );
}

int pip_trywait( int pipid, int *extvalp ) {
  RETURN( pip_do_wait( pipid, 1, extvalp ) );
}


static int pip_find_terminated( int *pipidp ) {
  static int	sidx = 0;
  int		count, i;

  pip_spin_lock( &pip_root->lock_tasks );
  /*** start lock region ***/
  {
    count = 0;
    for( i=sidx; i<pip_root->ntasks; i++ ) {
      if( pip_root->tasks[i].type != PIP_TYPE_NONE ) count ++;
      if( pip_root->tasks[i].flag_exit == PIP_EXITED ) {
	/* terminated task found */
	pip_root->tasks[i].flag_exit = PIP_EXIT_FINALIZE;
	pip_memory_barrier();
	*pipidp = i;
	sidx = i + 1;
	goto done;
      }
    }
    for( i=0; i<sidx; i++ ) {
      if( pip_root->tasks[i].type != PIP_TYPE_NONE ) count ++;
      if( pip_root->tasks[i].flag_exit == PIP_EXITED ) {
	/* terminated task found */
	pip_root->tasks[i].flag_exit = PIP_EXIT_FINALIZE;
	pip_memory_barrier();
	*pipidp = i;
	sidx = i + 1;
	goto done;
      }
    }
    *pipidp = PIP_PIPID_NULL;
  }
 done:
  /*** end lock region ***/
  pip_spin_unlock( &pip_root->lock_tasks );

  DBGF( "PIPID=%d  count=%d", *pipidp, count );
  return( count );
}

static int pip_set_sigchld_handler( sigset_t *sigset_oldp ) {
  void pip_sighand_sigchld( int sig, siginfo_t *siginfo, void *null ) {
    ENTER;
    if( pip_root != NULL ) {
      struct sigaction *sigact = &pip_root->sigact_chain;
      if( sigact->sa_sigaction != NULL ) {
	/* if user sets a signal handler, then it must be called */
	if( sigact->sa_sigaction != (void*) SIG_DFL &&  /* SIG_DFL==0 */
	    sigact->sa_sigaction != (void*) SIG_IGN ) { /* SIG_IGN==1 */
	  /* if signal handler is set by user, then call it */
	  sigact->sa_sigaction( sig, siginfo, null );
	}
      }
    }
    RETURNV;
  }
  struct sigaction 	sigact;
  struct sigaction	*sigact_oldp = &pip_root->sigact_chain;
  int err = 0;

  memset( &sigact, 0, sizeof( sigact ) );
  if( sigemptyset( &sigact.sa_mask )        != 0 ) RETURN( errno );
  if( sigaddset( &sigact.sa_mask, SIGCHLD ) != 0 ) RETURN( errno );
  err = pthread_sigmask( SIG_SETMASK, &sigact.sa_mask, sigset_oldp );
  if( !err ) {
    sigact.sa_flags     = SA_SIGINFO;
    sigact.sa_sigaction = (void(*)()) pip_sighand_sigchld;
    if( sigaction( SIGCHLD, &sigact, sigact_oldp ) != 0 ) err = errno;
  }
  RETURN( err );
}

static int pip_unset_sigchld_handler( sigset_t *sigset_oldp ) {
  struct sigaction	*sigact_oldp = &pip_root->sigact_chain;
  int 	err = 0;

  if( sigaction( SIGCHLD, sigact_oldp, NULL ) != 0 ) {
    err = errno;
  } else {
    err = pthread_sigmask( SIG_SETMASK, sigset_oldp, NULL );
    if( !err ) {
      memset( &pip_root->sigact_chain, 0, sizeof( struct sigaction ) );
    }
  }
  RETURN( err );
}

static int pip_do_waitany( int flag_try, int *pipidp, int *extvalp ) {
  sigset_t	sigset_old;
  int		pipid, count;
  int		err = 0;

  if( !pip_isa_root() ) RETURN( EPERM );

  count = pip_find_terminated( &pipid );
  if( pipid != PIP_PIPID_NULL ) {
    err = pip_do_wait_( pipid, flag_try, extvalp );
    if( err == 0 && pipidp != NULL ) *pipidp = pipid;
    RETURN( err );
  } else if( count == 0 || flag_try ) {
    RETURN( ECHILD );
  }
  /* try again, due to the race condition */
  if( ( err = pip_set_sigchld_handler( &sigset_old ) ) == 0 ) {
    while( 1 ) {
      count = pip_find_terminated( &pipid );
      if( pipid != PIP_PIPID_NULL ) {
	err = pip_do_wait_( pipid, flag_try, extvalp );
	if( err == 0 && pipidp != NULL ) *pipidp = pipid;
	break;
      }
      /* no terminated task found */
      if( count == 0 || flag_try ) {
	err = ECHILD;
	break;
      } else {
	sigset_t 	sigset;
	if( sigemptyset( &sigset ) != 0 ) {
	  err = errno;
	  break;
	}
	DBG;
	(void) sigsuspend( &sigset ); /* always returns EINTR */
	DBG;
	continue;
      }
    }
    /* undo signal setting */
    err = pip_unset_sigchld_handler( &sigset_old );
  }
  RETURN( err );
}

int pip_wait_any( int *pipidp, int *extvalp ) {
  RETURN( pip_do_waitany( 0, pipidp, extvalp ) );
}

int pip_trywait_any( int *pipidp, int *extvalp ) {
  RETURN( pip_do_waitany( 1, pipidp, extvalp ) );
}

int pip_get_pid( int pipid, pid_t *pidp ) {
  int err = 0;

  if( pidp == NULL ) RETURN( EINVAL );
  if( pip_root->opts && PIP_MODE_PROCESS ) {
    /* only valid with the "process" execution mode */
    if( ( err = pip_check_pipid( &pipid ) ) == 0 ) {
      if( pipid == PIP_PIPID_ROOT ) {
	*pidp = pip_root->task_root->pid_actual;
      } else {
	*pidp = pip_root->tasks[pipid].pid_actual;
      }
    }
  } else {
    err = EPERM;
  }
  RETURN( err );
}

int pip_barrier_init( pip_task_barrier_t *barrp, int n )
  __attribute__ ((deprecated, weak, alias ("pip_task_barrier_init")));
int pip_task_barrier_init( pip_task_barrier_t *barrp, int n ) {
  if( barrp == NULL ) RETURN( EINVAL );
  if( n < 1         ) RETURN( EINVAL );
  barrp->count      = n;
  barrp->count_init = n;
  barrp->gsense     = 0;
  RETURN( 0 );
}

int pip_barrier_wait( pip_task_barrier_t *barrp )
  __attribute__ ((deprecated, weak, alias ("pip_task_barrier_wait")));
int pip_task_barrier_wait( pip_task_barrier_t *barrp ) {
  ENTER;
  if( barrp == NULL ) RETURN (EINVAL );
  IF_LIKELY( barrp->count_init > 1 ) {
    int lsense = !barrp->gsense;
    IF_UNLIKELY( pip_atomic_sub_and_fetch( &barrp->count, 1 ) == 0 ) {
      barrp->count  = barrp->count_init;
      pip_memory_barrier();
      barrp->gsense = lsense;
    } else {
      while( barrp->gsense != lsense ) pip_pause();
    }
  }
  RETURN( 0 );
}

void pip_abort( void ) {
  ENTER;
  fflush( NULL );
#ifdef DEBUG
  sched_yield();
#endif
  (void) kill( 0, SIGKILL );
  //(void) killpg( pip_root->task_root->pid, SIGKILL );
}

/*-----------------------------------------------------*/
/* ULP ULP ULP ULP ULP ULP ULP ULP ULP ULP ULP ULP ULP */
/*-----------------------------------------------------*/

int pip_ulp_get_sched_task( int *pipidp ) {
  if( pipidp               == NULL ) RETURN( EINVAL );
  if( pip_task             == NULL ) RETURN( EPERM  );
  if( pip_task->task_sched == NULL ) RETURN( EPERM  ); /* i.e., suspended */
  *pipidp = pip_task->task_sched->pipid;
  RETURN( 0 );
}

int pip_locked_queue_init(  pip_locked_queue_t *queue ) {
  if( queue == NULL ) RETURN( EINVAL );
  pip_spin_init( &queue->lock );
  PIP_ULP_INIT( &queue->queue );
  return 0;
}

static void
pip_enqueue_with_policy( pip_ulp_t *queue, pip_ulp_t *ulp, int flags ) {
  if( flags == PIP_ULP_SCHED_LIFO ) {
    PIP_ULP_ENQ_FIRST( queue, ulp );
  } else {
    PIP_ULP_ENQ_LAST(  queue, ulp );
  }
}

int pip_enqueue_with_lock( pip_locked_queue_t *queue,
			   pip_ulp_t *ulp,
			   int flags ) {
  IF_UNLIKELY( queue == NULL || ulp == NULL ) RETURN( EINVAL );
  IF_UNLIKELY( PIP_TASK(ulp)->task_sched != NULL ) RETURN( EPERM  );

  pip_spin_lock( &queue->lock );
  DBGF( "LOCKED: %p", &queue->lock );
  pip_enqueue_with_policy( &queue->queue, ulp, flags );
  DBGF( "UNLCKD:%p", &queue->lock );
  pip_spin_unlock( &queue->lock );
  return 0;
}

int pip_dequeue_with_lock( pip_locked_queue_t *queue, pip_ulp_t **ulpp ) {
  pip_ulp_t	*ulp;
  int 		err = 0;

  IF_UNLIKELY( queue == NULL ) RETURN( EINVAL );

  pip_spin_lock( &queue->lock );
  DBGF( "LOCKED:%p", &queue->lock );
  IF_UNLIKELY( PIP_ULP_ISEMPTY( &queue->queue ) ) {
    err = ENOENT;
  } else {
    ulp = PIP_ULP_NEXT( &queue->queue );
    PIP_ULP_DEQ( ulp );
    IF_LIKELY( ulpp != NULL ) *ulpp = ulp;
  }
  DBGF( "UNLCKD:%p", &queue->lock );
  pip_spin_unlock( &queue->lock );
  RETURN( err );
}

static void pip_sleep_( intptr_t task_H, intptr_t task_L,
			intptr_t lock_H, intptr_t lock_L ) {
  pip_task_t *task = (pip_task_t*)
    ( ( ((intptr_t) task_H)  << 32 ) | ( ((intptr_t) task_L)  & PIP_MASK32 ) );
  pip_spinlock_t *lockp = (pip_spinlock_t*)
    ( ( ((intptr_t) lock_H) << 32 ) | ( ((intptr_t) lock_L) & PIP_MASK32 ) );

  ENTER;
  /* stack is switched and now we can unlock */
  pip_spin_unlock( lockp );
  DBGF( "SLEEPING (and UNLOCK)" );
  (void) sem_wait( &task->sleep.semaphore );
  DBGF( "WOKEUP !!" );
  task->type      &= ~PIP_TYPE_ULP; /* now this becomes task again */
  task->task_sched = NULL;
  task->pid_actual = task->pid;
  if( task->flag_exit ) {
    DBG;
    pip_force_exit( task );
  } else {
    DBG;
    pip_spin_lock(   &task->lock_wakeup ); /* wait for ctx switch */
    pip_spin_unlock( &task->lock_wakeup );
    DBGF( "lock-unlock" );
    (void) pip_load_tls( task->tls );
    (void) pip_load_context( task->ctx_suspend );
  }
  /* never reach here */
  RETURNV;
}

#define STACK_SIZE	(4096*16)

static int NOINLINE /* THIS FUNC MUST NOT BE INLINED */
pip_switch_stack_and_sleep( pip_task_t *task, pip_spinlock_t *lockp ) {
  pip_ctx_t	ctx;
  stack_t	*stk = &(ctx.ctx.uc_stack);
  void		*stack;
  int 		task_H, task_L, lock_H, lock_L;
  int 		err;

  ENTER;
  if( ( stack = task->sleep_stack ) == NULL ) {
    if( ( err = pip_page_alloc( STACK_SIZE, &stack ) ) != 0 ) RETURN( err );
    task->sleep_stack = stack;	/* stack address is kept to free it */
  }
  /* creating new context to siwtch stack */
  if( ( err = pip_save_context( &ctx ) ) != 0 ) RETURN( err );
  ctx.ctx.uc_link = NULL;
  stk->ss_sp      = stack;
  stk->ss_flags   = 0;
  stk->ss_size    = STACK_SIZE;
  task_H = ( ((intptr_t) pip_task) >> 32 ) & PIP_MASK32;
  task_L = (  (intptr_t) pip_task)         & PIP_MASK32;
  lock_H = ( ((intptr_t) lockp)    >> 32 ) & PIP_MASK32;
  lock_L = (  (intptr_t) lockp)	    & PIP_MASK32;
  pip_make_context( &ctx,
		    pip_sleep_,
		    4,
		    task_H,
		    task_L,
		    lock_H,
		    lock_L );
  err = pip_load_context( &ctx );
  /* never reach here, hopefully */
  RETURN( err );
}

int NOINLINE pip_sleep_and_enqueue( pip_locked_queue_t *queue,
				    pip_enqueuehook_t hook,
				    int flags ) {
  pip_task_t 	*task = pip_task;
  pip_ulp_t  	*ulp  = PIP_ULP( task );
#ifndef PIP_USE_STATIC_CTX
  pip_ctx_t 	ctx;
#endif
  pip_ctx_t	*ctxp;
  volatile int 	flag_jump;
  int 		err = 0;

  ENTER;
  STACK_GUARD;
  if( queue    == NULL ) RETURN( EINVAL );
  if( pip_task == NULL ) RETURN( EPERM  );
  if( pip_isa_ulp()    ) RETURN( EPERM  );

  pip_spin_lock( &queue->lock );
  DBGF( "LOCKED:%p", &queue->lock );
  {
    task->type      |= PIP_TYPE_ULP;
    task->task_sched = NULL;
    task->pid_actual = 0;
#ifndef PIP_USE_STATIC_CTX
    ctxp = &ctx;
#else
    ctxp = task->context;
#endif
    task->ctx_suspend = ctxp;
    flag_jump = 0;
    pip_save_context( ctxp );
    DBGF( "ctxp:%p", ctxp );
    if( !flag_jump ) {
      flag_jump = 1;
      pip_enqueue_with_policy( &queue->queue, ulp, flags );
      /* the hook, if any, must be called when enqueued */
      if( hook != NULL ) hook( task->pipid );
      /* unlock is postponed until stack is switched */
      err = pip_switch_stack_and_sleep( task, &queue->lock );
      RETURN( err );
    }
    pip_let_prev_task_go( task );
  }
  STACK_CHECK;
  RETURN( err );
}

int NOINLINE pip_wakeup( void ) {
  pip_task_t	*sched = pip_task->task_sched;
#ifndef PIP_USE_STATIC_CTX
  pip_ctx_t	ctx;
#endif
  pip_ctx_t	*ctxp;
  volatile int	flag_jump;	/* must be volatile */

  ENTER;
  if( !pip_isa_ulp() ) RETURN( 0 );
  if( sched == NULL                     ) RETURN( EPERM   );
  if( PIP_ULP_ISEMPTY( &sched->schedq ) ) RETURN( EDEADLK );

#ifdef PIP_USE_STATIC_CTX
  ctxp = pip_task->context;
#else
  ctxp = &ctx;
#endif
  pip_task->ctx_suspend = ctxp;
  flag_jump = 0;
  (void) pip_save_context( ctxp );
  DBGF( "ctxp:%p", ctxp );
  if( !flag_jump ) {
    flag_jump = 1;
    RETURN( pip_wakeup_and_sched_next( pip_task ) );
  }
  pip_let_prev_task_go( pip_task );
  RETURN( 0 );
}

int pip_dequeue_and_wakeup( pip_locked_queue_t *queue ) {
  pip_task_t 	*task;
  pip_ulp_t	*next;
  int 		err = 0;

  ENTER;
  if( queue    == NULL ) RETURN( EINVAL );
  if( pip_task == NULL ) RETURN( EPERM );

  pip_spin_lock( &queue->lock );
  DBGF( "LOCKED:%p", &queue->lock );
  {
    IF_LIKELY( !PIP_ULP_ISEMPTY( &queue->queue ) ) {
      next = PIP_ULP_NEXT( &queue->queue );
      task = PIP_TASK( next );
      if( task->type & PIP_TYPE_ULP ) {
	PIP_ULP_DEQ( next );
	err = pip_wakeup_( task );
      } else {
	err = EPERM;
      }
    } else {
      err = ENOENT;
    }
  }
  DBGF( "UNLCKD:%p", &queue->lock );
  pip_spin_unlock( &queue->lock );
  RETURN( err );
}

static inline int pip_ulp_yield_( pip_task_t *task ) {
  pip_ulp_t	*queue, *next;
  pip_task_t	*sched = task->task_sched;

  ENTER;
  queue = &sched->schedq;
  PIP_ULP_ENQ_LAST( queue, PIP_ULP( task ) );
  next = PIP_ULP_NEXT( queue );
  PIP_ULP_DEQ( next );

  RETURN( pip_ulp_switch_( task, next ) );
}

int pip_ulp_yield( void ) {
  return( pip_ulp_yield_( pip_task ) );
}

int NOINLINE /* THIS FUNC MUST NOT BE INLINED */
pip_ulp_suspend( void ) {
  pip_task_t 	*sched = pip_task->task_sched;
#ifndef PIP_USE_STATIC_CTX
  pip_ctx_t	ctx;
#endif
  pip_ctx_t	*ctxp;
  volatile int	flag_jump;	/* must be volatile */
  int err = 0;

  ENTER;
  STACK_GUARD;
  if( sched == NULL                     ) RETURN( EPERM   );
  if( PIP_ULP_ISEMPTY( &sched->schedq ) ) RETURN( EDEADLK );

  pip_task->task_resume = sched;
  pip_task->task_sched  = NULL;
#ifdef PIP_USE_STATIC_CTX
  ctxp = pip_task->context;
#else
  ctxp = &ctx;
#endif
  pip_task->ctx_suspend = ctxp;
  flag_jump = 0;
  (void) pip_save_context( ctxp );
  DBGF( "ctxp:%p", ctxp );
  if( !flag_jump ) {
    flag_jump = 1;
    err = pip_ulp_sched_next( sched, NULL );
    RETURN( err );
  }
  pip_let_prev_task_go( pip_task );
  STACK_CHECK;
  RETURN( err );
}

int pip_ulp_resume( pip_ulp_t *ulp, int flags ) {
  pip_task_t	*task, *sched;

  ENTER;
  IF_UNLIKELY( ulp == NULL ) RETURN( EINVAL );
  task = PIP_TASK( ulp );
  /* check if already being scheduled */
  IF_UNLIKELY( task->task_sched != NULL ) RETURN( EBUSY );
  sched = pip_task->task_sched;
  task->task_sched = sched;
  pip_enqueue_with_policy( &sched->schedq, ulp, flags );

  RETURN( 0 );
}

int NOINLINE /* THIS FUNC MUST NOT BE INLINED */
pip_ulp_suspend_and_enqueue( pip_locked_queue_t *queue,
			     pip_enqueuehook_t hook,
			     int flags ) {
  pip_task_t 	*sched = pip_task->task_sched;
#ifndef PIP_USE_STATIC_CTX
  pip_ctx_t 	ctx;
#endif
  pip_ctx_t	*ctxp;
  volatile int	flag_jump;	/* must be volatile */
  int 		err = 0;

  ENTER;
  STACK_GUARD;
  if( queue == NULL )  RETURN( EINVAL );
  if( !pip_isa_ulp() ) RETURN( EPERM  ); /* task cannot be migrated */
  if( PIP_ULP_ISEMPTY( &sched->schedq ) ) RETURN( EDEADLK );

#ifdef PIP_USE_STATIC_CTX
  ctxp = pip_task->context;
#else
  ctxp = &ctx;
#endif
  pip_task->ctx_suspend = ctxp;
  pip_task->task_sched  = NULL;
  pip_task->pid_actual  = 0;

  pip_spin_lock( &queue->lock ); /* LOCK */
  DBGF( "LOCKED:%p", &queue->lock );
  flag_jump = 0;
  (void) pip_save_context( ctxp );
  DBGF( "ctxp:%p", ctxp );
  if( !flag_jump ) {
    flag_jump = 1;
    pip_enqueue_with_policy( &queue->queue, PIP_ULP(pip_task), flags );
    if( hook != NULL ) hook( pip_task->pipid );
    RETURN( pip_ulp_sched_next( sched, &queue->lock ) );
  }
  pip_let_prev_task_go( pip_task );
  STACK_CHECK;
  RETURN( err );
}

int pip_ulp_dequeue_and_involve( pip_locked_queue_t *queue,
				 pip_ulp_t **ulpp,
				 int flags ) {
  pip_task_t	*task, *sched;
  pip_ulp_t	*next;
  int		err = 0;

  ENTER;
  if( queue == NULL) RETURN( EINVAL );

  pip_spin_lock( &queue->lock );
  DBGF( "LOCKED:%p", &queue->lock );
  {
    IF_UNLIKELY( PIP_ULP_ISEMPTY( &queue->queue ) ) ERRJ_ERR( ENOENT );
    next = PIP_ULP_NEXT( &queue->queue );
    task = PIP_TASK( next );
    IF_UNLIKELY( !( task->type & PIP_TYPE_ULP ) ) {
      ERRJ_ERR( EPERM  );	/* must be a ULP. task is not migratable */
    }
    IF_UNLIKELY( task->task_sched != NULL ) ERRJ_ERR( EBUSY );
    /* looks OK, and now dequeue */
    PIP_ULP_DEQ( next );
  }
 error:
  DBGF( "UNLCKD:%p", &queue->lock );
  pip_spin_unlock( &queue->lock );

  if( !err ) {
    sched = pip_task->task_sched;
    task->task_sched = sched;
    task->pid_actual = pip_task->pid;
    DBGF( "enqueue PIPID:%d", task->pipid );
    pip_enqueue_with_policy( &sched->schedq, next, flags );
    if( ulpp != NULL ) *ulpp = next;
  }
  RETURN( err );
}

int pip_ulp_yield_to( pip_ulp_t *ulp ) {
  pip_task_t	*sched, *task;
  pip_ulp_t	*queue;

  IF_UNLIKELY( ulp == NULL ) RETURN( pip_ulp_yield() );
  sched = pip_task->task_sched;
  queue = &sched->schedq;
  task  = PIP_TASK( ulp );
  /* the target ulp must be in the same scheduling domain */
  IF_UNLIKELY( task->task_sched != sched ) RETURN( EPERM );

  PIP_ULP_DEQ( ulp );
  PIP_ULP_ENQ_LAST( queue, PIP_ULP( pip_task ) );

  RETURN( pip_ulp_switch_( pip_task, ulp ) );
}

int pip_ulp_get_pipid( pip_ulp_t *ulp, int *pipidp ) {
  pip_task_t	*task;

  if( pip_root == NULL ) RETURN( EPERM );
  if( ulp == NULL ) {
    task = pip_task;
  } else {
    task = PIP_TASK( ulp );
  }
  *pipidp = task->pipid;
  return 0;
}

int pip_ulp_myself( pip_ulp_t **ulpp ) {
  IF_UNLIKELY( ulpp     == NULL ) RETURN( EINVAL );
  IF_UNLIKELY( pip_root == NULL ) RETURN( EPERM );
  *ulpp = PIP_ULP( pip_task );
  return 0;
}

int pip_ulp_get( int pipid, pip_ulp_t **ulpp ) {
  pip_task_t *task;
  int err;

  IF_UNLIKELY( ulpp == NULL ) RETURN( EINVAL );
  IF_UNLIKELY( ( err = pip_check_pipid( &pipid ) ) != 0 ) RETURN( err );
  task = pip_get_task_( pipid );
  *ulpp = PIP_ULP( task );
  return 0;
}

int pip_ulp_mutex_init( pip_ulp_mutex_t *mutex ) {
  DBG;
  IF_UNLIKELY( mutex == NULL ) RETURN( EINVAL );
  PIP_ULP_INIT( &mutex->waiting );
  mutex->sched  = NULL;
  mutex->holder = NULL;
  return 0;
}

int pip_ulp_mutex_lock( pip_ulp_mutex_t *mutex ) {
  DBG;
  IF_UNLIKELY( mutex == NULL ) {
    RETURN( EINVAL );
  } else IF_UNLIKELY( mutex->sched != NULL &&
		      mutex->sched != pip_task->task_sched ) {
    RETURN( EPERM );
  } else IF_UNLIKELY( mutex->holder == pip_task ) {
    RETURN( EDEADLK );
  } else IF_LIKELY( mutex->holder == NULL ) { /* lock succeeded */
    mutex->holder = pip_task;
    IF_UNLIKELY( mutex->sched == NULL ) {
      mutex->sched = pip_task->task_sched;
    }
  } else {			/* lock failed */
    PIP_ULP_ENQ_LAST( &mutex->waiting, PIP_ULP( pip_task ) );
    RETURN( pip_ulp_suspend() );
  }
  DBG;
  return 0;
}

int pip_ulp_mutex_unlock( pip_ulp_mutex_t *mutex ) {
  pip_ulp_t *ulp;

  DBG;
  IF_UNLIKELY( mutex == NULL ) {
    RETURN( EINVAL );
  } else IF_UNLIKELY( pip_task->task_sched != mutex->sched ) {
    /* different scheduling domain */
    RETURN( EPERM );
  } else IF_UNLIKELY( mutex->holder == NULL ) {
    /* not locked  */
    RETURN( EPERM );
  }
  DBG;
  IF_LIKELY( !PIP_ULP_ISEMPTY( &mutex->waiting ) ) {
    ulp = PIP_ULP_NEXT( &mutex->waiting );
    mutex->holder = ulp;
    PIP_ULP_DEQ( ulp );
    DBG;
    RETURN( pip_ulp_resume( ulp, 0 ) );
  }
  return 0;
}

int pip_ulp_barrier_init( pip_ulp_barrier_t *barrp, int n ) {
  if( barrp == NULL || n < 1 ) RETURN( EINVAL );
  PIP_ULP_INIT( &barrp->waiting );
  barrp->sched      = NULL;
  barrp->count      = n;
  barrp->count_init = n;
  RETURN( 0 );
}

int pip_ulp_barrier_wait( pip_ulp_barrier_t *barrp ) {
  pip_ulp_t 	*ulp, *u;
  int 		err = 0;

  ENTER;
  IF_UNLIKELY( barrp == NULL ) RETURN( EINVAL );
  IF_LIKELY( barrp->count_init > 1 ) {
    IF_UNLIKELY( barrp->sched == NULL ) {
      barrp->sched = (void*) pip_task->task_sched;
    } else IF_UNLIKELY( barrp->sched != (void*) pip_task->task_sched ) {
      /* different scheduling domain */
      RETURN( EPERM );
    }
    IF_UNLIKELY( -- barrp->count == 0 ) {
      /* done ! */
      PIP_ULP_FOREACH_SAFE( &barrp->waiting, ulp, u ) {
	PIP_ULP_DEQ( ulp );
	DBGF( "Resume [%d]", PIP_TASK(ulp)->pipid );
	IF_UNLIKELY( ( err = pip_ulp_resume( ulp, 0 ) ) != 0 ) RETURN( err );
      }
      barrp->count = barrp->count_init;
      barrp->sched = NULL;
    } else {
      /* not yet */
      PIP_ULP_ENQ_LAST( &barrp->waiting, PIP_ULP( pip_task ) );
      err = pip_ulp_suspend();
    }
  } else {
    /* nothing to do */
  }
  RETURN( err );
}

/*** the following malloc/free functions are just for experimenmtal ***/
/*** These will be replaced by the ones by using MPC allocator      ***/

/* long long to align */
#define PIP_ALIGN_TYPE	long long

void *pip_malloc( size_t size ) {
  pip_task_t *task = pip_task;

  pip_spin_lock(   &task->lock_malloc );
  void *p = malloc( size + sizeof(PIP_ALIGN_TYPE) );
  pip_spin_unlock( &task->lock_malloc );

  *(int*) p = pip_get_pipid_();
  p += sizeof(PIP_ALIGN_TYPE);
  return p;
}

void pip_free( void *ptr ) {
  pip_task_t *task;
  free_t free_func;
  int pipid;

  ptr  -= sizeof(PIP_ALIGN_TYPE);
  pipid = *(int*) ptr;
  if( pipid >= 0 || pipid == PIP_PIPID_ROOT ) {
    if( pipid == PIP_PIPID_ROOT ) {
      task = pip_root->task_root;
    } else {
      task = &pip_root->tasks[pipid];
    }
    /* need of sanity check on pipid */
    if( ( free_func = task->symbols.free ) != NULL ) {

      pip_spin_lock(   &task->lock_malloc );
      free_func( ptr );
      pip_spin_unlock( &task->lock_malloc );

    } else {
      pip_warn_mesg( "No free function" );
    }
  } else {
    free( ptr );
  }
}
