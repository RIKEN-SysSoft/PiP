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

//#define DEBUG
//#define PRINT_MAPS
//#define PRINT_FDS

//#define PIP_USE_MUTEX
//#define PIP_USE_SIGQUEUE

/* the EVAL env. is to measure the time for calling dlmopen() */
//#define EVAL

#include <pip.h>
#include <pip_ulp.h>
#include <pip_internal.h>
#include <pip_queue.h>
#include <pip_util.h>
#include <pip_gdbif.h>

#ifdef PIP_ULP_NEED_SCHED_LOCK
#define PIP_ULP_SCHED_LOCK(S)	pip_spin_lock(   &(S)->lock_schedq );
#define PIP_ULP_SCHED_UNLOCK(S)	pip_spin_unlock( &(S)->lock_schedq );
#else
#define PIP_ULP_SCHED_LOCK(S)
#define PIP_ULP_SCHED_UNLOCK(S)
#endif

#define IF_LIKELY(C)		if( pip_likely( C ) )
#define IF_UNLIKELY(C)		if( pip_unlikely( C ) )

extern char 		**environ;

/*** note that the following static variables are   ***/
/*** located at each PIP task and the root process. ***/
pip_root_t	*pip_root = NULL;
pip_task_t	*pip_task = NULL;

static pip_clone_t*	pip_cloneinfo = NULL;

int  pip_named_export_init( pip_task_t* );
int  pip_named_export_fin(  pip_task_t* );
void pip_named_export_fin_all( void );

static int (*pip_clone_mostly_pthread_ptr) (
	pthread_t *newthread,
	int clone_flags,
	int core_no,
	size_t stack_size,
	void *(*start_routine) (void *),
	void *arg,
	pid_t *pidp) = NULL;

struct pip_gdbif_root	*pip_gdbif_root;

int pip_is_root( void ) {
  return pip_task != NULL && pip_task->type == PIP_TYPE_ROOT;
}

int pip_is_task( void ) {
  return pip_task != NULL && pip_task->type == PIP_TYPE_TASK;
}

int pip_is_ulp( void ) {
  return pip_task != NULL && pip_task->type == PIP_TYPE_ULP;
}

static int pip_count_vec( char **vecsrc ) {
  int n = 0;
  if( vecsrc != NULL ) {
    for( n=0; vecsrc[n]!= NULL; n++ );
  }
  return( n );
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

static int pip_page_alloc( size_t sz, void **allocp ) {
  size_t pgsz;
  if( pip_root == NULL ) {	/* in pip_init () (), no pip_root yet */
    pgsz = sysconf( _SC_PAGESIZE );
  } else {
    pgsz = pip_root->page_size;
  }
  sz = ( ( sz + pgsz - 1 ) / pgsz ) * pgsz;
  RETURN( posix_memalign( allocp, pgsz, sz ) );
}

static void pip_reset_task_struct( pip_task_t *task ) {
  memset( (void*) task, 0, sizeof( pip_task_t ) );
  task->type = PIP_TYPE_NONE;
  task->pid  = -1; /* pip_init_gdbif_task_struct() refers this */
  PIP_ULP_INIT( &task->queue  );
  PIP_ULP_INIT( &task->schedq );
  pip_spin_init( &task->lock_schedq );
  pip_spin_init( &task->lock_malloc );
#ifdef PIP_USE_MUTEX
  (void) pthread_mutex_init( &task->mutex_wait, NULL );
#else
  (void) sem_init( &task->sem_wait, 1, 0 );
#endif
  pip_memory_barrier();
  task->pipid = PIP_PIPID_NONE;
}

static int pipid_to_gdbif( int pipid ) {
  switch( pipid ) {
  case PIP_PIPID_ROOT:
    return( PIP_GDBIF_PIPID_ROOT );
  case PIP_PIPID_ANY:
    return( PIP_GDBIF_PIPID_ANY );
  default:
    return( pipid );
  }
}

static void pip_init_gdbif_task_struct(	struct pip_gdbif_task *gdbif_task,
					pip_task_t *task) {
  /* members from task->args are unavailable if PIP_GDBIF_STATUS_TERMINATED */
  gdbif_task->pathname     = task->args.prog;
  gdbif_task->realpathname = NULL; /* filled by pip_load_gdbif() later */
  if ( task->args.argv == NULL ) {
    gdbif_task->argc = 0;
  } else {
    gdbif_task->argc = pip_count_vec( task->args.argv );
  }
  gdbif_task->argv = task->args.argv;
  gdbif_task->envv = task->args.envv;

  gdbif_task->handle = task->loaded; /* filled by pip_load_gdbif() later */
  gdbif_task->load_address = NULL; /* filled by pip_load_gdbif() later */
  gdbif_task->exit_code    = -1;
  gdbif_task->pid          = task->pid;
  gdbif_task->pipid        = pipid_to_gdbif( task->pipid );
  gdbif_task->exec_mode =
    (pip_root->opts & PIP_MODE_PROCESS) ? PIP_GDBIF_EXMODE_PROCESS :
    (pip_root->opts & PIP_MODE_PTHREAD) ? PIP_GDBIF_EXMODE_THREAD :
    PIP_GDBIF_EXMODE_NULL;
  gdbif_task->status     = PIP_GDBIF_STATUS_NULL;
  gdbif_task->gdb_status = PIP_GDBIF_GDB_DETACHED;
}

static void pip_init_gdbif_root_task_link(struct pip_gdbif_task *gdbif_task) {
  PIP_HCIRCLEQ_INIT(*gdbif_task, task_list);
}

static void pip_link_gdbif_task_struct(	struct pip_gdbif_task *gdbif_task) {
  gdbif_task->root = &pip_gdbif_root->task_root;
  pip_spin_lock( &pip_gdbif_root->lock_root );
  PIP_HCIRCLEQ_INSERT_TAIL(pip_gdbif_root->task_root, gdbif_task, task_list);
  pip_spin_unlock( &pip_gdbif_root->lock_root );
}

/*
 * NOTE: pip_load_gdbif() won't be called for PiP root tasks.
 * thus, load_address and realpathname are both NULL for them.
 * handle is available, though.
 *
 * because this function is only for PIP-gdb,
 * this does not return any error, but warn.
 */
static void pip_load_gdbif( pip_task_t *task ) {
  struct pip_gdbif_task *gdbif_task = task->gdbif_task;
  Dl_info dli;
  char buf[PATH_MAX];

  if( gdbif_task != NULL ) {
    gdbif_task->handle = task->loaded;

    if( !dladdr( task->symbols.main, &dli ) ) {
      pip_warn_mesg( "dladdr(%s) failure"
		     " - PIP-gdb won't work with this PiP task %d",
		     task->args.prog, task->pipid );
      gdbif_task->load_address = NULL;
    } else {
      gdbif_task->load_address = dli.dli_fbase;
    }

    /* dli.dli_fname is same with task->args.prog and may be a relative path */
    DBGF( "[%d] task:%p gdbif_task:%p", task->pipid, task, gdbif_task );
    if( realpath( task->args.prog, buf ) == NULL ) {
      gdbif_task->realpathname = NULL; /* give up */
      pip_warn_mesg( "realpath(%s): %s"
		     " - PIP-gdb won't work with this PiP task %d",
		     task->args.prog, strerror( errno ), task->pipid );
    } else {
      gdbif_task->realpathname = strdup( buf );
      DBGF( "gdbif_task->realpathname:%p", gdbif_task->realpathname );
      if( gdbif_task->realpathname == NULL ) { /* give up */
	pip_warn_mesg( "strdup(%s) failure"
		       " - PIP-gdb won't work with this PiP task %d",
		       task->args.prog, task->pipid );
      }
    }
  }
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
  struct pip_gdbif_root *gdbif_root;

  if( pip_root != NULL ) {
    DBGF( "pip_root:%p", pip_root );
    RETURN( EBUSY ); /* already initialized */
  }
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
    pip_spin_init( &pip_root->lock_stack_flist );
    pip_spin_init( &pip_root->lock_tasks       );
    /* beyond this point, we can call the       */
    /* pip_dlsymc() and pip_dlclose() functions */

    pipid = PIP_PIPID_ROOT;
    pip_set_magic( pip_root );
    pip_root->version   = PIP_VERSION;
    pip_root->ntasks    = ntasks;
    pip_root->cloneinfo = pip_cloneinfo;
    pip_root->opts      = opts;
    pip_root->page_size = sysconf( _SC_PAGESIZE );
    for( i=0; i<ntasks+1; i++ ) {
      pip_reset_task_struct( &pip_root->tasks[i] );
    }
    pip_root->task_root = &pip_root->tasks[ntasks];
    pip_task               = pip_root->task_root;
    pip_task->type         = PIP_TYPE_ROOT;
    pip_task->pipid        = pipid;
    pip_task->type         = PIP_TYPE_ROOT;
    pip_task->symbols.free = (free_t)pip_dlsym(RTLD_DEFAULT,"free");
    pip_task->loaded       = dlopen( NULL, RTLD_NOW );
    pip_task->thread       = pthread_self();
    pip_task->pid          = getpid();
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

    sz = sizeof( *gdbif_root ) + sizeof( gdbif_root->tasks[0] ) * ntasks;
    if( ( err = pip_page_alloc( sz, (void**) &gdbif_root ) ) != 0 ) {
      free( pip_root );
      RETURN( err );
    }
    gdbif_root->hook_before_main = NULL; /* XXX */
    gdbif_root->hook_after_main  = NULL; /* XXX */
    pip_spin_init( &gdbif_root->lock_free );
    PIP_SLIST_INIT(&gdbif_root->task_free);
    pip_spin_init( &gdbif_root->lock_root );
    pip_init_gdbif_task_struct( &gdbif_root->task_root, pip_root->task_root );
    pip_init_gdbif_root_task_link( &gdbif_root->task_root );
    pip_memory_barrier();
    gdbif_root->task_root.status = PIP_GDBIF_STATUS_CREATED;
    pip_gdbif_root = gdbif_root; /* assign after initialization completed */

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
  /* root and child */
  if( pipidp != NULL ) *pipidp = pipid;
  DBGF( "pip_root=%p  pip_task=%p", pip_root, pip_task );
  RETURN( err );
}

static int pip_is_pthread_( void ) {
  return (pip_root->opts & PIP_MODE_PTHREAD) != 0 ? CLONE_THREAD : 0;
}

/* The following functions;
   pip_is_pthread(),
   pip_is_shared_fd_(),
   pip_is_shared_fd(), and
   pip_is_shared_sighand()
   can be called from any context
*/

int pip_is_pthread( int *flagp ) {
  if( pip_root == NULL ) RETURN( EPERM  );
  if( flagp    == NULL ) RETURN( EINVAL );
  *flagp = pip_is_pthread_();
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

int pip_is_shared_sighand( int *flagp ) {
  if( pip_root == NULL ) RETURN( EPERM  );
  if( flagp    == NULL ) RETURN( EINVAL );
  if( pip_root->cloneinfo == NULL ) {
    *flagp = (pip_root->opts & PIP_MODE_PTHREAD) != 0 ? CLONE_SIGHAND : 0;
  } else {
    *flagp = pip_root->cloneinfo->flag_clone & CLONE_SIGHAND;
  }
  RETURN( 0 );
}

int pip_isa_piptask( void ) {
  /* this function might be called before calling pip_init() */
  return getenv( PIP_ROOT_ENV ) != NULL;
}

int pip_check_pipid( int *pipidp ) {
  int pipid = *pipidp;

  if( pipid >= pip_root->ntasks ) RETURN( EINVAL );
  if( pipid != PIP_PIPID_MYSELF &&
      pipid <  PIP_PIPID_ROOT   ) RETURN( EINVAL );
  if( pip_root == NULL          ) RETURN( EPERM  );
  switch( pipid ) {
  case PIP_PIPID_ROOT:
    break;
  case PIP_PIPID_ANY:
    RETURN( EINVAL );
  case PIP_PIPID_MYSELF:
    if( pip_is_root() ) {
      *pipidp = PIP_PIPID_ROOT;
    } else if( pip_is_task() || pip_is_ulp() ) {
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
  if( pip_task == NULL ) return PIP_PIPID_NONE;
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
  if( pip_is_root() ) {
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
    DBGF( "=%p", *addrp );
  } else {
    DBG;
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

static size_t pip_stack_size( void ) {
  char 		*env, *endptr;
  size_t 	sz, scale;
  int 		i;

  if( ( sz = pip_root->stack_size ) == 0 ) {
    if( ( env = getenv( PIP_ENV_STACKSZ ) ) == NULL &&
	( env = getenv( "KMP_STACKSIZE" ) ) == NULL &&
	( env = getenv( "OMP_STACKSIZE" ) ) == NULL ) {
      sz = PIP_STACK_SIZE;	/* default */
    } else if( ( sz = (size_t) strtol( env, &endptr, 10 ) ) <= 0 ) {
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
	scale *= 1024 * 1024;
	sz *= scale;
	break;
      default:
	pip_warn_mesg( "stacksize: '%s' is illegal and 'K' is assumed", env );
      case 'K': case 'k': case '\0':
	scale *= 1024;
      case 'B': case 'b':
	sz *= scale;
	for( i=PIP_STACK_SIZE_MIN; i<sz; i*=2 );
	sz = i;
	break;
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
  symp->glibc_init       = dlsym( handle, "glibc_init"                   );
  symp->mallopt          = dlsym( handle, "mallopt"                      );
  symp->libc_fflush      = dlsym( handle, "fflush"                       );
  symp->free             = dlsym( handle, "free"                         );
  symp->named_export_fin = dlsym( handle, "pip_named_export_fin"         );
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
      pip_load_gdbif( task );
    }
  }
  RETURN( err );
}

static int pip_do_corebind( int coreno, cpu_set_t *oldsetp ) {
  int err = 0;

  if( coreno != PIP_CPUCORE_ASIS ) {
    cpu_set_t cpuset;

    CPU_ZERO( &cpuset );
    CPU_SET( coreno, &cpuset );

    if( pip_is_pthread_() ) {
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

  if( coreno != PIP_CPUCORE_ASIS ) {
    if( pip_is_pthread_() ) {
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

  if( coreno != PIP_CPUCORE_ASIS &&
      coreno >= 0                &&
      coreno <  sizeof(cpuset) * 8 ) {
    DBG;
    CPU_ZERO( &cpuset );
    CPU_SET( coreno, &cpuset );
    if( sched_setaffinity( 0, sizeof(cpuset), &cpuset ) != 0 ) RETURN( errno );
    DBG;
  } else {
    DBGF( "coreno=%d", coreno );
  }
  DBG;
  RETURN( 0 );
}

static int pip_glibc_init( pip_symbols_t *symbols,
			   char *prog,
			   char **argv,
			   char **envv,
			   void *loaded ) {
  int argc = pip_count_vec( argv );

  DBG;
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

static void pip_flush_stdio( pip_task_t *task ) {
  /* call fflush() in the target context to flush out messages */
  if( task != NULL && task->symbols.libc_fflush != NULL ) {
    DBGF( ">> [%d] fflush@%p()", task->pipid, task->symbols.libc_fflush );
    task->symbols.libc_fflush( NULL );
    DBGF( "<< [%d] fflush@%p()", task->pipid, task->symbols.libc_fflush );
  }
}

static void pip_ulp_recycle_stack( void *stack ) {
  pip_spin_lock( &pip_root->lock_stack_flist );
  {
    *((void**)stack) = pip_root->stack_flist;
    pip_root->stack_flist = stack;
  }
  pip_spin_unlock( &pip_root->lock_stack_flist );
}

static void *pip_ulp_reuse_stack( pip_task_t *task ) {
  void *stack;
  pip_spin_lock( &pip_root->lock_stack_flist );
  {
    stack = pip_root->stack_flist;
    if( pip_root->stack_flist != NULL ) {
      pip_root->stack_flist = *((void**)stack);
    }
  }
  pip_spin_unlock( &pip_root->lock_stack_flist );
  return stack;
}

static void *pip_ulp_alloc_stack( void ) {
  void *stack;

  if( ( stack = pip_ulp_reuse_stack( pip_task ) ) == NULL ) {
    /* guard pages, top and bottom, to be idependent from stack direction */
    size_t	pgsz  = pip_root->page_size;
    size_t	stksz = pip_stack_size();
    size_t	sz    = stksz + pgsz + pgsz;
    void       *region;

    if( pip_page_alloc( sz, &region ) != 0 ) return NULL;
    if( mprotect( region,            pgsz, PROT_NONE ) != 0 ||
	mprotect( region+pgsz+stksz, pgsz, PROT_NONE ) != 0 ) {
      free( region );
      return NULL;
    }
    stack = region + pgsz;	/* skip the stack protect page */
  }
  return stack;
}

static void pip_ulp_start_( int pipid, int root_H, int root_L );

static int pip_ulp_newctx( pip_task_t *task, pip_ctx_t *ctxp ) {
  int err = 0;

  DBG;
  if( ( err = pip_save_context( ctxp ) ) == 0 ) {
    stack_t 	*stk = &(ctxp->ctx.uc_stack);
    void 	*stack;
    int		root_H, root_L;

    ctxp->ctx.uc_link = NULL;

    if( ( stack = pip_ulp_alloc_stack() ) == NULL ) {
      err = ENOMEM;

    } else {
      DBGF( "stack=%p", stack );

      task->ulp_stack = stack;
      stk->ss_sp      = stack;
      stk->ss_flags   = 0;
      stk->ss_size    = pip_root->stack_size;

      root_H = ( ((intptr_t) pip_root) >> 32 ) & PIP_MASK32;
      root_L = (  (intptr_t) pip_root)         & PIP_MASK32;
      DBGF( "pip_root=%p  (0x%x:%x)", pip_root, root_H, root_L );
      pip_make_context( ctxp,
			pip_ulp_start_,
			3,
			task->pipid,
			root_H,
			root_L );
    }
  }
  RETURN( err );
}

static int pip_ulp_switch_( pip_task_t *old_task, pip_ulp_t *ulp ) {
  pip_task_t	*new_task = PIP_TASK( ulp );
  pip_ctx_t 	oldctx, newctx;
  pip_ctx_t	*newctxp;
  int		err = 0;

  DBGF( "Next-PIPID:%d", new_task->pipid );
  if( old_task == new_task ) return 0;
  //PIP_ULP_DESCRIBE( ulp );
  if( new_task->ctx_suspend == NULL ) { /* for the first time being scheduled */
    newctxp = &newctx;
    err = pip_ulp_newctx( new_task, newctxp ); /* reset newctx */
  } else {
    newctxp = new_task->ctx_suspend;
  }
  if( err == 0 ) {
    old_task->ctx_suspend = &oldctx;
    new_task->ctx_suspend = newctxp;
    DBGF( "old:%p  new:%p", &oldctx, newctxp );
    err = pip_swap_context( &oldctx, newctxp );
  }
  RETURN( err );
}

static void pip_set_extval( pip_task_t *task, int extval ) {
  if( extval != 0 && task->extval == 0 ) {
    task->extval = extval;
    if( task->gdbif_task != NULL ) {
      task->gdbif_task->exit_code = extval;
    }
  }
}

static void pip_task_terminated( pip_task_t *task ) {
  if( !task->flag_exit ) {
    DBGF( "extval=%d", task->extval );
    task->flag_exit = PIP_EXITED;
    pip_memory_barrier();
    if( task->gdbif_task != NULL ) {
      task->gdbif_task->status = PIP_GDBIF_STATUS_TERMINATED;
    }
  }
}

static int pip_raise_sigchld( void ) {
  int err = 0;

  if( pip_is_pthread_() ) {
    DBG;
#ifndef PIP_USE_SIGQUEUE
    if( pthread_kill( pip_root->task_root->thread, SIGCHLD ) == ESRCH ) {
      DBG;
      if( kill( pip_root->task_root->pid, SIGCHLD ) != 0 ) err = errno;
    }
#else
    if( pthread_sigqueue( pip_root->task_root->thread,
			  SIGCHLD,
			  (const union sigval)0 ) == ESRCH ) {
      DBG;
      if( sigqueue( pip_root->task_root->pid,
		    SIGCHLD,
		    (const union sigval) 0 ) != 0 ) {
	err = errno;
      }
    }
#endif
  }
  RETURN( err );
}

static int pip_ulp_sched_next( pip_task_t *sched ) {
  /* wait for all ULP terminations */
  pip_ulp_t 	*queue, *next;
  pip_task_t 	*nxt_task;
  pip_ctx_t 	nxt_ctx;
  pip_ctx_t	*nxt_ctxp;
  int		err;

  DBGF( "sched:%p", sched );;
  queue = &sched->schedq;
  PIP_ULP_SCHED_LOCK( sched );
  {
    if( PIP_ULP_ISEMPTY( &sched->schedq ) ) {
      PIP_ULP_SCHED_UNLOCK( sched );
      RETURN( ENOENT );
    }
    next = PIP_ULP_NEXT( queue );
    PIP_ULP_DEQ( next );
  }
  PIP_ULP_SCHED_UNLOCK( sched );
  nxt_task = PIP_TASK( next );
  if( nxt_task->ctx_suspend == NULL ) { /* for the first time scheduling */
    nxt_ctxp = &nxt_ctx;
    err = pip_ulp_newctx( nxt_task, nxt_ctxp ); /* reset nxt_ctx */
    if( err ) {
      pip_err_mesg( "Unable to create a new ULP context (%d)", err );
      exit( err );
    }
    nxt_task->ctx_suspend = nxt_ctxp;
  } else {
    nxt_ctxp = nxt_task->ctx_suspend;
  }
  DBGF( "nxt_ctxp:%p  Next-PIPID:%d", nxt_ctxp, nxt_task->pipid );
  err = pip_load_context( nxt_ctxp );
  /* never reach here */
  return err;
}

static void pip_ulp_start_( int pipid, int root_H, int root_L )  {
  char		*prog, **argv, **envv;
  void		*start_arg;
  pip_root_t	*root;
  pip_task_t 	*self, *sched = NULL;
  int 		argc;
  int		extval = 0;
  volatile int	flag_jump;	/* must be volatile */
  pip_ctx_t 	ctx_exit;

  DBGF( "pipid=%d  root=0x%x:%x", pipid, root_H, root_L );
  root = (pip_root_t*)
    ( ( ((intptr_t) root_H)<<32 ) | ( ((intptr_t) root_L) & PIP_MASK32 ) );
  self = &root->tasks[pipid];
  prog      = self->args.prog;
  argv      = self->args.argv;
  envv      = self->args.envv;
  start_arg = self->args.start_arg;

  argc = pip_glibc_init( &self->symbols, prog, argv, envv, self->loaded );

  flag_jump = 0;
  self->ctx_exit = &ctx_exit;
  DBGF( "ctx_exit:%p", &ctx_exit );
  (void) pip_save_context( &ctx_exit );
  if( !flag_jump ) {
    flag_jump = 1;
    if( self->symbols.start == NULL ) {
      DBGF( ">> ULPmain@%p(%d,%s,%s,...)",
	    self->symbols.main, argc, argv[0], argv[1] );
      extval = self->symbols.main( argc, argv, envv );
      DBGF( "<< ULPmain@%p(%d,%s,%s,...)",
	    self->symbols.main, argc, argv[0], argv[1] );
    } else {
      DBGF( ">> %s:%p(%p)",
	    self->args.funcname, self->symbols.start, start_arg );
      extval = self->symbols.start( start_arg );
      DBGF( "<< %s:%p(%p)",
	    self->args.funcname, self->symbols.start, start_arg );
    }
  }
  DBGF( "[%d] task:%p gdbif_task:%p", self->pipid, self, self->gdbif_task );
  /* note: task_sched miht be changed due to migration */
  sched = self->task_sched;
  self->task_sched = NULL;
  pip_flush_stdio( self );
  pip_set_extval( self, extval );
  /* free() must be called in the task's context */
  (void) self->symbols.named_export_fin( self );
#ifdef PIP_USE_MUTEX
  (void) pthread_mutex_unlock( &self->mutex_wait );
#else
  (void) sem_post( &self->sem_wait );
#endif
  pip_task_terminated( self );
  DBGF( "[%d] task:%p gdbif_task:%p", self->pipid, self, self->gdbif_task );
  (void) pip_raise_sigchld();
  DBGF( "[%d] task:%p gdbif_task:%p", self->pipid, self, self->gdbif_task );
  DBG;
  if( sched != NULL && pip_ulp_sched_next( sched ) == ENOENT ) {
    DBGF( "[%d] task:%p gdbif_task:%p", self->pipid, self, self->gdbif_task );
    DBGF( "ctx_exit:%p", sched->ctx_exit );
    (void) pip_load_context( sched->ctx_exit );
  }
  /* never reach here, hopefully */
}

static int pip_jump_into( pip_spawn_args_t *args, pip_task_t *self )
  __attribute__((noinline));
static int pip_jump_into( pip_spawn_args_t *args, pip_task_t *self ) {
  char  	*prog      = args->prog;
  char 		**argv     = args->argv;
  char 		**envv     = args->envv;
  void		*start_arg = args->start_arg;
  int 		argc;
  int		extval = 0;
  pip_ctx_t 	ctx_exit;
  volatile int	flag_jump;	/* must be volatile */

  flag_jump = 0;
  self->ctx_exit = &ctx_exit;
  DBGF( "ctx_exit:%p", &ctx_exit );
  (void) pip_save_context( &ctx_exit );
  DBG;
  if( !flag_jump ) {
    flag_jump = 1;
    DBGF( "<<<<<<<< ctx:%p >>>>>>>>", &ctx_exit );
    argc = pip_glibc_init( &self->symbols, prog, argv, envv, self->loaded );
    if( self->symbols.start == NULL ) {
      DBGF( "[%d] >> main@%p(%d,%s,%s,...)",
	    args->pipid, self->symbols.main, argc, argv[0], argv[1] );
      extval = self->symbols.main( argc, argv, envv );
      DBGF( "[%d] << main@%p(%d,%s,%s,...)",
	    args->pipid, self->symbols.main, argc, argv[0], argv[1] );
    } else {
      DBGF( "[%d] >> %s:%p(%p)",
	    args->pipid, args->funcname, self->symbols.start, start_arg );
      extval = self->symbols.start( start_arg );
      DBGF( "[%d] >> %s:%p(%p)",
	    args->pipid, args->funcname, self->symbols.start, start_arg );
    }
  } else {
    DBGF( "!! main(%s,%s,...)", argv[0], argv[1] );
  }
  if( self->extval ) extval = self->extval;
  return extval;
}

static void pip_sched_ulps( pip_task_t *self ) __attribute__((noinline));
static void pip_sched_ulps( pip_task_t *self ) {
  pip_ctx_t 	ctx_exit;
  volatile int	flag_jump = 0;	/* must be volatile */

  DBGF( "ctx_exit:%p", &ctx_exit );
  self->ctx_exit = &ctx_exit;
  (void) pip_save_context( &ctx_exit );
  if( !flag_jump ) {
    flag_jump = 1;
    DBGF( "PIPID:%d", self->pipid );
    (void) pip_ulp_sched_next( self );
  }
}

static void *pip_do_spawn( void *thargs )  {
  /* The context of this function is of the root task                */
  /* so the global var; pip_task (and pip_root) are of the root task */
  pip_spawn_args_t *args   = (pip_spawn_args_t*) thargs;
  int	 	pipid      = args->pipid;
  char 		**argv     = args->argv;
  int 		coreno     = args->coreno;
  pip_task_t 	*self      = &pip_root->tasks[pipid];
  pip_spawnhook_t before   = self->hook_before;
  void 		*hook_arg  = self->hook_arg;
  int		extval = 0;
  int		err = 0;

  if( ( err = pip_corebind( coreno ) ) != 0 ) {
    pip_warn_mesg( "failed to bund CPU core (%d)", err );
  }
  self->thread = pthread_self();

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

  /* calling hook, if any */
  if( before != NULL && ( err = before( hook_arg ) ) != 0 ) {
    pip_warn_mesg( "[%s] before-hook returns %d", argv[0], before, err );
    pip_flush_stdio( self );
    pip_set_extval( self, err );
    pip_task_terminated( self );
    (void) pip_raise_sigchld();
    extval = err;

  } else {
    if( ( pip_root->opts & PIP_OPT_PGRP ) && !pip_is_pthread_() ) {
      (void) setpgid( 0, 0 );	/* Is this meaningful ? */
    }
    extval = pip_jump_into( args, self );
    /* the after hook is supposed to free the hook_arg being malloc()ed */
    /* and this is the reason to call this from the root process        */
    if( self->hook_after != NULL ) (void) self->hook_after( self->hook_arg );
    DBG;
    if( !PIP_ULP_ISEMPTY( &self->schedq ) ) {
      pip_sched_ulps( self );
    }
    /* no ulps eligible to run and we can terminate this ulp */
    DBGF( "[%d] task:%p gdbif_task:%p", self->pipid, self, self->gdbif_task );
    DBG;
    /* free() must be called in the task's context */
    (void) self->symbols.named_export_fin( self );
    DBGF( "[%d] task:%p gdbif_task:%p", self->pipid, self, self->gdbif_task );
    pip_flush_stdio( self );
    DBGF( "[%d] task:%p gdbif_task:%p", self->pipid, self, self->gdbif_task );
    pip_set_extval( self, extval );
    DBGF( "[%d] task:%p gdbif_task:%p", self->pipid, self, self->gdbif_task );
    pip_task_terminated( self );
    DBGF( "[%d] task:%p gdbif_task:%p", self->pipid, self, self->gdbif_task );
    (void) pip_raise_sigchld();
    DBGF( "[%d] task:%p gdbif_task:%p", self->pipid, self, self->gdbif_task );

    if( pip_root->opts & PIP_OPT_FORCEEXIT ) {
      if( pip_is_pthread_() ) {	/* thread mode */
	pthread_exit( (void*) &extval );
      } else {			/* process mode */
	exit( extval );
      }
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
      if( pip_root->tasks[pipid].pipid != PIP_PIPID_NONE ) {
	err = EAGAIN;
	goto unlock;
      }
    } else {
      for( i=pip_root->pipid_curr; i<pip_root->ntasks; i++ ) {
	if( pip_root->tasks[i].pipid == PIP_PIPID_NONE ) {
	  pipid = i;
	  goto found;
	}
      }
      for( i=0; i<=pip_root->pipid_curr; i++ ) {
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

int pip_task_spawn( pip_spawn_program_t *progp,
		    int coreno,
		    int *pipidp,
		    pip_spawn_hook_t *hookp,
		    pip_ulp_t *sisters ) {
  cpu_set_t 		cpuset;
  pip_spawn_args_t	*args = NULL;
  pip_task_t		*task = NULL;
  struct pip_gdbif_task *gdbif_task = NULL;
  size_t		stack_size = pip_stack_size();
  int 			pipid;
  pid_t			pid = 0;
  int 			err = 0;

  DBGF( ">> pip_spawn_()" );

  if( pip_root        == NULL ) RETURN( EPERM  );
  if( pipidp          == NULL ) RETURN( EINVAL );
  if( !pip_is_root()          ) RETURN( EPERM  );
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
  if( sisters != NULL ) {
    pip_ulp_t 	*daughter;
    pip_task_t	*dtask;

    DBG;
    PIP_ULP_FOREACH( sisters, daughter ) { /* check */
      dtask = PIP_TASK( daughter );
      DBGF( "daughter [%d] (next:%d)  ctx:%p", dtask->pipid,
	    PIP_TASK(daughter->next)->pipid, dtask->ctx_suspend );
      if( dtask->type != PIP_TYPE_ULP ) ERRJ_ERR( EPERM );
      if( dtask->task_sched != NULL   ) ERRJ_ERR( EBUSY );
    }
    PIP_ULP_MOVE_QUEUE( &task->schedq, sisters );
    PIP_ULP_FOREACH( &task->schedq, daughter ) {
      PIP_TASK(daughter)->task_sched = task;
    }
  }
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
  DBG;
  gdbif_task = &pip_gdbif_root->tasks[pipid];
  pip_init_gdbif_task_struct( gdbif_task, task );
  pip_link_gdbif_task_struct( gdbif_task );
  task->gdbif_task = gdbif_task;
  DBG;
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
      DBG;
      do {
	err = pthread_create( &task->thread,
			      &attr,
			      (void*(*)(void*)) pip_do_spawn,
			      (void*) args );
	DBGF( "pthread_create()=%d", errno );
      } while( 0 );
      /* unlock is done in the wrapper function */
      DBG;
      if( pip_root->cloneinfo != NULL ) {
	pid = pip_root->cloneinfo->pid_clone;
	pip_root->cloneinfo->pid_clone = 0;
      }
    }
  }
  DBG;
  if( err == 0 ) {
    task->pid = pid;
    pip_root->ntasks_accum ++;
    pip_root->ntasks_curr  ++;
    gdbif_task->pid = pid;
    pip_memory_barrier();
    gdbif_task->status = PIP_GDBIF_STATUS_CREATED;
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
    if( sisters != NULL ) {
      pip_ulp_t 	*daughter, *next;
      pip_task_t	*dtask;
      PIP_ULP_FOREACH_SAFE( &task->schedq, daughter, next ) {
	dtask = PIP_TASK( daughter );
	dtask->task_sched = NULL;
	PIP_ULP_DEQ( daughter );
      }
    }
  }
  DBGF( "<< pip_spawn_(pipid=%d)", *pipidp );
  RETURN( err );
}

/*
 * The following functions must be called at root process
 */

static void pip_finalize_gdbif_tasks( void ) {
  struct pip_gdbif_task *gdbif_task, **prev, *next;

  if( pip_gdbif_root == NULL ) {
    DBGF( "pip_gdbif_root=NULL, pip_init() hasn't called?" );
    return;
  }
  pip_spin_lock( &pip_gdbif_root->lock_root );
  prev = &PIP_SLIST_FIRST(&pip_gdbif_root->task_free);
  PIP_SLIST_FOREACH_SAFE(gdbif_task, &pip_gdbif_root->task_free, free_list,
			 next) {
    if( gdbif_task->gdb_status != PIP_GDBIF_GDB_DETACHED ) {
      prev = &PIP_SLIST_NEXT(gdbif_task, free_list);
    } else {
      *prev = next;
      PIP_HCIRCLEQ_REMOVE(gdbif_task, task_list);
    }
  }
  pip_spin_unlock( &pip_gdbif_root->lock_root );
}

static void pip_finalize_task( pip_task_t *task, int *extvalp ) {
  struct pip_gdbif_task *gdbif_task = task->gdbif_task;

  DBGF( "pipid=%d  extval=%d", task->pipid, task->extval );

  if( gdbif_task != NULL ) {
    gdbif_task->status   = PIP_GDBIF_STATUS_TERMINATED;
    pip_memory_barrier();
    gdbif_task->pathname = NULL;
    gdbif_task->argc     = 0;
    gdbif_task->argv     = NULL;
    gdbif_task->envv     = NULL;
    DBGF( "[%d] task:%p gdbif_task:%p", task->pipid, task, gdbif_task );
    if( gdbif_task->realpathname  != NULL ) {
      char *p = gdbif_task->realpathname;
      DBGF( "p:%p", p );
      gdbif_task->realpathname = NULL; /* do this before free() for PIP-gdb */
      pip_memory_barrier();
      free( p );
    }
    DBG;
    pip_spin_lock( &pip_gdbif_root->lock_free );
    {
      PIP_SLIST_INSERT_HEAD(&pip_gdbif_root->task_free, gdbif_task, free_list);
      pip_finalize_gdbif_tasks();
    }
    pip_spin_unlock( &pip_gdbif_root->lock_free );
  }
  DBGF( "flag=%d  extval=%d", task->flag_exit, task->extval );
  if( extvalp != NULL ) *extvalp = ( task->extval & 0xFF );

  /* dlclose() and free() must be called only from the root process since */
  /* corresponding dlmopen() and malloc() is called by the root process   */
  if( task->type == PIP_TYPE_ULP ) {
    pip_ulp_recycle_stack( task->ulp_stack );
  }
  if( task->loaded     != NULL ) pip_dlclose( task->loaded );
  if( task->args.prog  != NULL ) free( task->args.prog );
  if( task->args.argv  != NULL ) free( task->args.argv );
  if( task->args.envv  != NULL ) free( task->args.envv );
  if( *task->symbols.progname_full != NULL ) {
    free( *task->symbols.progname_full );
  }
  DBG;
  pip_reset_task_struct( task );
}

int pip_fin( void ) {
  int ntasks, i, err = 0;

  if( pip_root == NULL ) RETURN( EPERM );
  if( pip_is_root() ) {		/* root */
    ntasks = pip_root->ntasks;
    for( i=0; i<ntasks; i++ ) {
      if( !pip_root->tasks[i].pipid == PIP_PIPID_NONE &&
	  !pip_root->tasks[i].flag_exit ) {
	if( pip_is_pthread_() ) {
	  if( pthread_kill( pip_root->tasks[i].thread, 0 ) != 0 &&
	      errno == ESRCH ) {
	    pip_finalize_task( &pip_root->tasks[i], NULL );
	    continue;
	  }
	} else {
	  if( kill( pip_root->tasks[i].pid, 0 ) != 0 &&
	      errno == ESRCH ) {
	    pip_finalize_task( &pip_root->tasks[i], NULL );
	    continue;
	  }
	}
	DBGF( "%d/%d [pipid=%d (type=%d)] -- BUSY",
	      i, ntasks, pip_root->tasks[i].pipid, pip_root->tasks[i].type );
	err = EBUSY;
	//break;
      }
    }
    if( err == 0 ) {
      void *stack, *next;
      DBG;
      /* freeing allocated memory regions */
      for( stack = pip_root->stack_flist; stack != NULL; stack = next ) {
	next = *((void**) stack);
	free( stack - pip_root->page_size );
      }
      pip_named_export_fin_all();
      memset( pip_root, 0, pip_root->size );
      free( pip_root );
      pip_root = NULL;
      pip_task = NULL;
      /* report accumulated timer values, if set */
      PIP_REPORT( time_load_dso  );
      PIP_REPORT( time_load_prog );
      PIP_REPORT( time_dlmopen   );
    }
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
  return pip_task_spawn( &program, coreno, pipidp, &hook,  NULL );
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
  if( pip_is_pthread_() ) {
    /* Do not use gettid(). This is a very Linux-specific function */
    /* The reason of supporintg the thread PiP execution mode is   */
    /* some OSes other than Linux does not support clone()         */
    if( task->type != PIP_TYPE_ULP ) {
      *pidp = (intptr_t) task->thread;
    } else {
      *pidp = (intptr_t) task->task_sched->thread;
    }
  } else {
    if( task->type != PIP_TYPE_ULP ) {
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
  if( task->type == PIP_TYPE_ROOT ||
      task->type == PIP_TYPE_TASK) {
    err = pthread_kill( task->thread, signal );
#ifdef AHAH
    if( pip_is_pthread_() ) {
      err = pthread_kill( task->thread, signal );
      DBGF( "pthread_kill(sig=%d)=%d", signal, err );
    } else {
      if( kill( task->pid, signal ) != 0 ) err = errno;
      DBGF( "kill(sig=%d)=%d", signal, err );
    }
#endif
  } else {			/* signal cannot be sent to ULPs */
    err = EPERM;
  }
  RETURN( err );
}

int pip_exit( int extval ) {
  int err = 0;
  DBG;
  fflush( NULL );
  DBG;
  if( pip_is_task() || pip_is_ulp() ) {
    DBG;
    (void) pip_named_export_fin( pip_task );
    DBG;
    pip_set_extval( pip_task, extval );
    DBGF( "pip_task->ctx_exit:%p", pip_task->ctx_exit );
    err = pip_load_context( pip_task->ctx_exit );
    /* never reach here, hopefully */
  } else {
    if( pip_is_root() ) {
      if( ( err = pip_fin() ) != 0 ) goto error;
    }
    /* must be able to pip_exit() even if it is NOT a PIP environment. */
    exit( extval );
  }
  /* hopefully, never reach here */
 error:
  RETURN( err );
}

static int pip_do_wait_( int pipid, int flag_try, int *extvalp ) {
  pip_task_t *task;
  int err = 0;

  task = pip_get_task_( pipid );
  if( task->type == PIP_TYPE_NONE ) RETURN( ESRCH );

  DBGF( "[%d] task:%p gdbif_task:%p", task->pipid, task, task->gdbif_task );
  DBGF( "pipid=%d  type=%d", pipid, task->type );
  if( task->type == PIP_TYPE_ULP ) {
    /* ULP */
    DBG;
    if( flag_try ) {
#ifdef PIP_USE_MUTEX
      if( ( err = pthread_mutex_trylock( &task->mutex_wait ) ) == 0 &&
	  ( err = pthread_mutex_unlock(  &task->mutex_wait ) ) == 0 &&
	  ( err = pthread_mutex_destroy( &task->mutex_wait ) ) == 0 );
#else
      if( sem_trywait( &task->sem_wait ) != 0 ) {
	if( errno != EINTR ) err = errno;
      }
#endif
    } else {
#ifdef PIP_USE_MUTEX
      if( ( err = pthread_mutex_lock(    &task->mutex_wait ) ) == 0 &&
	  ( err = pthread_mutex_unlock(  &task->mutex_wait ) ) == 0 &&
	  ( err = pthread_mutex_destroy( &task->mutex_wait ) ) == 0 );
#else
      while( 1 ) {
	if( sem_wait( &task->sem_wait ) == 0 ) break;
	if( errno != EINTR ) {
	  err = errno;
	  break;
	}
      }
#endif
    }
    DBGF( "[%d] task:%p gdbif_task:%p", task->pipid, task, task->gdbif_task );
  } else if( pip_is_pthread_() ) {
    /* thread mode */
    if( flag_try ) {
      err = pthread_tryjoin_np( task->thread, NULL );
      DBGF( "pthread_tryjoin_np(%d)=%d", task->extval, err );
    } else {
      err = pthread_join( task->thread, NULL );
      DBGF( "pthread_join(%d)=%d", task->extval, err );
    }
    DBGF( "[%d] task:%p gdbif_task:%p", task->pipid, task, task->gdbif_task );
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
    DBG;
    if( flag_try ) options |= WNOHANG;
    DBGF( "task:%p  task->pid:%d  pipid:%d", task, task->pid, task->pipid );
    DBGF( "pid:%d", getpid() );
    if( ( pid = waitpid( task->pid, &status, options ) ) < 0 ) {
      err = errno;
    } else {
      if( WIFEXITED( status ) ) {
	int extval = WEXITSTATUS( status );
	pip_set_extval( task, extval );
	pip_task_terminated( task );
      } else if( WIFSIGNALED( status ) ) {
  	int sig = WTERMSIG( status );
	pip_warn_mesg( "PiP Task [%d] terminated by %s signal",
		       task->pipid, strsignal(sig) );
	pip_set_extval( task, sig + 128 );
	pip_task_terminated( task );
      }
      DBGF( "wait(status=%x)=%d (errno=%d)", status, pid, err );
    }
  }
  DBGF( "[%d] task:%p gdbif_task:%p", task->pipid, task, task->gdbif_task );
  if( err == 0 ) pip_finalize_task( task, extvalp );
  DBG;
  RETURN( err );
}

static int pip_do_wait( int pipid, int flag_try, int *extvalp ) {
  int err;

  if( !pip_is_root() )                           RETURN( EPERM );
  if( ( err = pip_check_pipid( &pipid ) ) != 0 ) RETURN( err );
  if( pipid == PIP_PIPID_ROOT )                  RETURN( EDEADLK );

  RETURN( pip_do_wait_( pipid, flag_try, extvalp ) );
}

int pip_wait( int pipid, int *extvalp ) {
  DBG;
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
    *pipidp = PIP_PIPID_NONE;
  }
 done:
  /*** end lock region ***/
  pip_spin_unlock( &pip_root->lock_tasks );

  DBGF( "PIPID=%d  count=%d", *pipidp, count );
  return( count );
}

static int pip_do_waitany( int flag_try, int *pipidp, int *extvalp ) {
  void pip_sighand_sigchld( int sig, siginfo_t *siginfo, void *null ) {
    DBG;
    if( pip_root != NULL ) {
      struct sigaction *sigact = &pip_root->sigact_chain;
      if( sigact->sa_sigaction != NULL ) {
	/* if user sets a signal handler, then it must be called */
	if( sigact->sa_sigaction != (void*) SIG_DFL &&  /* SIG_DFL==0 */
	    sigact->sa_sigaction != (void*) SIG_IGN ) { /* SIG_IGN==1 */
	  /* if signal handler is set, then call it */
	  sigact->sa_sigaction( sig, siginfo, null );
	}
      }
    }
  }
  struct sigaction 	sigact;
  struct sigaction	*sigact_old = &pip_root->sigact_chain;
  sigset_t		sigset_old;
  int	pipid, count;
  int	err = 0;

  if( !pip_is_root() ) RETURN( EPERM );

  DBG;
  count = pip_find_terminated( &pipid );
  if( pipid != PIP_PIPID_NONE ) {
    err = pip_do_wait_( pipid, flag_try, extvalp );
    if( err == 0 && pipidp != NULL ) *pipidp = pipid;
    RETURN( err );
  } else if( count == 0 || flag_try ) {
    RETURN( ECHILD );
  }
  /* try again, due to the race condition */
  DBG;
  memset( &sigact, 0, sizeof( sigact ) );
  if( sigemptyset( &sigact.sa_mask )        != 0 ) RETURN( errno );
  if( sigaddset( &sigact.sa_mask, SIGCHLD ) != 0 ) RETURN( errno );
  sigact.sa_flags = SA_SIGINFO;
  if( pthread_sigmask( SIG_SETMASK,
		       &sigact.sa_mask,
		       &sigset_old ) != 0 ) RETURN( errno );
  sigact.sa_sigaction = (void(*)()) pip_sighand_sigchld;
  if( sigaction( SIGCHLD, &sigact, sigact_old ) != 0 ) {
    err = errno;
    goto error;
  } else {
    while( 1 ) {
      DBG;
      count = pip_find_terminated( &pipid );
      if( pipid != PIP_PIPID_NONE ) {
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
	continue;
      }
    }
  }
  /* undo signal setting */
  DBG;
  if( sigaction( SIGCHLD, sigact_old, NULL ) < 0 ) RETURN( errno );
  memset( &pip_root->sigact_chain, 0, sizeof( struct sigaction ) );
 error:
  if( pthread_sigmask( SIG_SETMASK,
		       &sigset_old,
		       NULL ) != 0 ) {
    if( err == 0 ) err = errno;
  }
  RETURN( err );
}

int pip_wait_any( int *pipidp, int *extvalp ) {
  DBG;
  RETURN( pip_do_waitany( 0, pipidp, extvalp ) );
}

int pip_trywait_any( int *pipidp, int *extvalp ) {
  RETURN( pip_do_waitany( 1, pipidp, extvalp ) );
}

int pip_get_pid_( int pipid, pid_t *pidp ) {
  int err = 0;

  if( pidp == NULL ) RETURN( EINVAL );
  if( pip_root->opts && PIP_MODE_PROCESS ) {
    /* only valid with the "process" execution mode */
    if( ( err = pip_check_pipid( &pipid ) ) == 0 ) {
      if( pipid == PIP_PIPID_ROOT ) {
	err = EPERM;
      } else {
	*pidp = pip_root->tasks[pipid].pid;
      }
    }
  } else {
    err = EPERM;
  }
  RETURN( err );
}

int pip_barrier_init( pip_barrier_t *barrp, int n )
  __attribute__ ((deprecated, weak, alias ("pip_task_barrier_init")));
int pip_task_barrier_init( pip_barrier_t *barrp, int n ) {
  if( barrp == NULL ) RETURN( EINVAL );
  if( n < 1         ) RETURN( EINVAL );
  barrp->count      = n;
  barrp->count_init = n;
  barrp->gsense     = 0;
  return 0;
}

int pip_barrier_wait( pip_barrier_t *barrp )
  __attribute__ ((deprecated, weak, alias ("pip_task_barrier_wait")));
int pip_task_barrier_wait( pip_barrier_t *barrp ) {
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
  return 0;
}

/*-----------------------------------------------------*/
/* ULP ULP ULP ULP ULP ULP ULP ULP ULP ULP ULP ULP ULP */
/*-----------------------------------------------------*/

int pip_get_ulp_sched( int *pipidp ) {
  pip_task_t *sched = pip_task->task_sched;

  if( pipidp == NULL ) RETURN( EINVAL );
  if( sched  == NULL ) RETURN( EPERM ); /* i.e., suspended */
  *pipidp = sched->pipid;
  return 0;
}

int pip_ulp_new( pip_spawn_program_t *progp,
		 int  *pipidp,
		 pip_ulp_t *sisters,
		 pip_ulp_t **newp ) {
  //struct pip_gdbif_task *gdbif_task = NULL;
  pip_spawn_args_t	*args = NULL;
  pip_task_t		*task = NULL;
  int			pipid;
  int 			err = 0;


  if( pip_root        == NULL ) RETURN( EPERM  );
  if( pipidp          == NULL ) RETURN( EINVAL );
  if( !pip_is_root()          ) RETURN( EPERM  );
  if( sisters         == NULL &&
      newp            == NULL ) RETURN( EINVAL );
  if( progp           == NULL ) RETURN( EINVAL );
  if( progp->funcname != NULL &&
      progp->argv     != NULL ) RETURN( EINVAL );
  if( progp->funcname == NULL &&
      progp->argv     == NULL ) RETURN( EINVAL );
  if( progp->funcname == NULL &&
      progp->prog     == NULL ) progp->prog = progp->argv[0];
#ifdef AH
  if( sisters != NULL ) {
    if( ( task = PIP_TASK( sisters ) ) != NULL &&
	  task->type                   != PIP_TYPE_ULP ) RETURN( EPERM );
    if( task->flag_exit ) RETURN( ESRCH );
  }
#endif
  DBGF( ">> pip_ulp_new()" );

  if( progp->envv == NULL ) progp->envv = environ;
  pipid = *pipidp;
  if( ( err = pip_find_a_free_task( &pipid ) ) != 0 ) ERRJ;
  task = &pip_root->tasks[pipid];
  pip_reset_task_struct( task );
  task->pipid = pipid;
  task->type  = PIP_TYPE_ULP;
#ifdef PIP_USE_MUTEX
  if( ( err = pthread_mutex_lock( &task->mutex_wait ) ) != 0 ) ERRJ;
#endif
  if( sisters != NULL ) {
    PIP_ULP_ENQ_LAST( sisters, PIP_ULP( task ) );
  }
#ifdef    AHAH
  gdbif_task = &pip_gdbif_root->tasks[pipid];
  pip_init_gdbif_task_struct( gdbif_task, task );
  pip_link_gdbif_task_struct( gdbif_task );
  task->gdbif_task = gdbif_task;
#endif
  args = &task->args;
  args->start_arg = progp->arg;
  args->pipid     = pipid;
  if( ( args->prog = strdup( progp->prog )              ) == NULL ||
      ( args->envv = pip_copy_env( progp->envv, pipid ) ) == NULL ) {
    ERRJ_ERR(ENOMEM);
  }
  if( progp->funcname == NULL ) {
    if( ( args->argv = pip_copy_vec( progp->argv ) ) == NULL ) {
      ERRJ_ERR(ENOMEM);
    }
  } else if( ( args->funcname = strdup( progp->funcname ) ) == NULL ) {
    ERRJ_ERR(ENOMEM);
  }

  pip_spin_lock( &pip_root->lock_ldlinux );
  /*** begin lock region ***/
  do {
    PIP_ACCUM( time_load_prog, ( ( err = pip_load_prog( progp, task ) ) ==0));
  } while( 0 );
  /*** end lock region ***/
  pip_spin_unlock( &pip_root->lock_ldlinux );
  ERRJ_CHK(err);

  if( newp != NULL) *newp = PIP_ULP( task );

 error:
  if( err != 0 ) {
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
  DBGF( "<< pip_ulp_new(pipid:%d, ctx:%p)=%d", pipid, task->ctx_suspend, err );
  RETURN( err );
}

int pip_ulp_yield( void ) {
  pip_task_t	*sched;
  pip_ulp_t	*queue, *next;

  DBGF( "[%d] task:%p gdbif_task:%p",
	pip_task->pipid, pip_task, pip_task->gdbif_task );
  sched = pip_task->task_sched;
  IF_UNLIKELY( sched == NULL ) RETURN( EPERM );
  queue = &sched->schedq;

  PIP_ULP_SCHED_LOCK( sched );
  {
    PIP_ULP_ENQ_LAST( queue, PIP_ULP( pip_task ) );
    next = PIP_ULP_NEXT( queue );
    PIP_ULP_DEQ( next );
  }
  PIP_ULP_SCHED_UNLOCK( sched );

  RETURN( pip_ulp_switch_( pip_task, next ) );
}

int pip_ulp_suspend( void ) {
  pip_task_t 	*sched = pip_task->task_sched;
  pip_ctx_t	ctx;
  volatile int	flag_jump;	/* must be volatile */
  int err = 0;

  if( sched == NULL ) RETURN( EPERM );
  DBG;
  flag_jump = 0;
  pip_task->ctx_suspend = &ctx;
  pip_save_context( &ctx );
  IF_LIKELY( !flag_jump ) {
    flag_jump = 1;
    pip_task->task_resume = sched;
    pip_task->task_sched  = NULL;
    IF_UNLIKELY( ( err = pip_ulp_sched_next( sched ) ) == ENOENT ) {
      /* undo */
      pip_task->task_sched = sched;
      RETURN( EDEADLK );
    }
  }
  RETURN( err );
}

static void
pip_ulp_enqueue_with_policy( pip_ulp_t *queue, pip_ulp_t *ulp, int flags ) {
  if( flags == PIP_ULP_SCHED_LIFO ) {
    PIP_ULP_ENQ_FIRST( queue, ulp );
  } else {
    PIP_ULP_ENQ_LAST(  queue, ulp );
  }
}

int pip_ulp_resume( pip_ulp_t *ulp, int flags ) {
  pip_task_t	*task, *sched;

  DBG;
  IF_UNLIKELY( ulp == NULL ) RETURN( EINVAL );
  task = PIP_TASK( ulp );
  IF_UNLIKELY( task->flag_exit ) RETURN( ESRCH ); /* already terminated */
  /* check if already being scheduled */
  IF_UNLIKELY( task->task_sched != NULL ) RETURN( EBUSY );

  sched = pip_task->task_sched;
  task->task_sched = sched;

  PIP_ULP_SCHED_LOCK( sched );
  pip_ulp_enqueue_with_policy( &sched->schedq, ulp, flags );
  PIP_ULP_SCHED_UNLOCK( sched );

  RETURN( 0 );
}

int pip_ulp_locked_queue_init(  pip_ulp_locked_queue_t *queue ) {
  if( queue == NULL ) RETURN( EINVAL );
  pip_spin_init( &queue->lock );
  PIP_ULP_INIT( &queue->queue );
  return 0;
}

int pip_ulp_suspend_and_enqueue( pip_ulp_locked_queue_t *queue, int flags ) {
  pip_task_t 	*sched = pip_task->task_sched;;
  int 		err = 0;

  if( queue == NULL ) RETURN( EINVAL );
  if( !pip_is_ulp() ) RETURN( EPERM  ); /* task cannot be migrated */

  if( PIP_ULP_ISEMPTY( &sched->schedq ) ) {
    err = EDEADLK;
  } else {
    pip_ctx_t		ctx;
    volatile int	flag_jump = 0;	/* must be volatile */

    pip_spin_lock( &queue->lock );
    {
      pip_ulp_enqueue_with_policy( &queue->queue, PIP_ULP( pip_task ), flags );
      pip_task->ctx_suspend = &ctx;
      pip_save_context( &ctx );
      IF_LIKELY( !flag_jump ) {
	flag_jump = 1;
	pip_task->task_resume = sched;
	pip_task->task_sched  = NULL;

	pip_spin_unlock( &queue->lock );
	(void) pip_ulp_sched_next( sched );
      }
      goto done;
    }
    pip_spin_unlock( &queue->lock );
  }
 done:
  RETURN( err );
}

int pip_ulp_dequeue_and_migrate( pip_ulp_locked_queue_t *queue,
				 pip_ulp_t **ulpp,
				 int flags ) {
  pip_task_t	*task, *sched;
  pip_ulp_t	*next;
  int		err = 0;

  if( queue == NULL) RETURN( EINVAL );

  pip_spin_lock( &queue->lock );

  if( PIP_ULP_ISEMPTY( &queue->queue ) ) ERRJ_ERR( ENOENT );
  next = PIP_ULP_NEXT( &queue->queue );
  PIP_ULP_DEQ( next );

  task = PIP_TASK( next );
  IF_UNLIKELY( task->flag_exit ) ERRJ_ERR( ESRCH ); /* already terminated */
  IF_UNLIKELY( task->type == PIP_TYPE_TASK ) {
    ERRJ_ERR( EPERM );		/* task is not migratable */
  }
  IF_UNLIKELY( task->task_sched != NULL ) ERRJ_ERR( EBUSY );

  sched = pip_task->task_sched;
  task->task_sched  = sched;

  PIP_ULP_SCHED_LOCK( sched );
  pip_ulp_enqueue_with_policy( &sched->schedq, next, flags );
  PIP_ULP_SCHED_UNLOCK( sched );

  if( ulpp != NULL ) *ulpp = next;

 error:
  pip_spin_unlock( &queue->lock );
  RETURN( err );
}

int pip_ulp_enqueue_with_lock( pip_ulp_locked_queue_t *queue,
			       pip_ulp_t *ulp,
			       int flags ) {
  if( queue == NULL || ulp == NULL ) RETURN( EINVAL );
  pip_spin_lock( &queue->lock );
  pip_ulp_enqueue_with_policy( &queue->queue, ulp, flags );
  pip_spin_unlock( &queue->lock );
  return 0;
}

int
pip_ulp_dequeue_with_lock( pip_ulp_locked_queue_t *queue, pip_ulp_t **ulpp ) {
  pip_ulp_t	*ulp;
  int 		err = 0;

  if( queue == NULL ) {
    err = EINVAL;
  } else {
    pip_spin_lock( &queue->lock );
    if( PIP_ULP_ISEMPTY( &queue->queue ) ) {
      err = ENOENT;
    } else {
      ulp = PIP_ULP_NEXT( &queue->queue );
      PIP_ULP_DEQ( ulp );
      if( ulpp != NULL ) *ulpp = ulp;
    }
    pip_spin_unlock( &queue->lock );
  }
  RETURN( err );
}

int pip_ulp_yield_to( pip_ulp_t *ulp ) {
  pip_ulp_t	*queue;
  pip_task_t	*sched, *task;

  IF_UNLIKELY( ulp   == NULL ) RETURN( pip_ulp_yield() );
  sched = pip_task->task_sched;
  IF_UNLIKELY( sched == NULL ) RETURN( EPERM );
  queue = &sched->schedq;
  task  = PIP_TASK( ulp );
  IF_UNLIKELY( task->task_sched != sched ) RETURN( EPERM );

  PIP_ULP_SCHED_LOCK( sched );
  {
    PIP_ULP_DEQ( ulp );
    PIP_ULP_ENQ_LAST( queue, PIP_ULP( pip_task ) );
  }
  PIP_ULP_SCHED_UNLOCK( sched );

  RETURN( pip_ulp_switch_( pip_task, ulp ) );
}

int pip_ulp_get_pipid( pip_ulp_t *ulp, int *pipidp ) {
  pip_task_t	*task;

  IF_UNLIKELY( pipidp == NULL ) RETURN( EINVAL );
  task = PIP_TASK( ulp );
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
  } else IF_UNLIKELY( mutex->holder == PIP_ULP( pip_task ) ) {
    RETURN( EDEADLK );
  } else IF_LIKELY( mutex->holder == NULL ) {
    DBG;
    mutex->holder = PIP_ULP( pip_task );
    IF_UNLIKELY( mutex->sched == NULL ) {
      mutex->sched = pip_task->task_sched;
    }
  } else {
    DBG;
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
    RETURN( EPERM );
  } else IF_UNLIKELY( mutex->holder == NULL ) {
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
  return 0;
}

int pip_ulp_barrier_wait( pip_ulp_barrier_t *barrp ) {
  pip_ulp_t 	*ulp, *u;
  int 		err = 0;

  IF_UNLIKELY( barrp == NULL ) RETURN( EINVAL );
  IF_LIKELY( barrp->count_init > 1 ) {
    IF_UNLIKELY( barrp->sched == NULL ) {
      barrp->sched = (void*) pip_task->task_sched;
    } else IF_UNLIKELY( barrp->sched != (void*) pip_task->task_sched ) {
      RETURN( EPERM );
    }
    IF_UNLIKELY( -- barrp->count == 0 ) {
      DBG;
      PIP_ULP_FOREACH_SAFE( &barrp->waiting, ulp, u ) {
	PIP_ULP_DEQ( ulp );
	IF_UNLIKELY( ( err = pip_ulp_resume( ulp, 0 ) ) != 0 ) RETURN( err );
      }
      barrp->count = barrp->count_init;
      barrp->sched = NULL;
    } else {
      DBG;
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
