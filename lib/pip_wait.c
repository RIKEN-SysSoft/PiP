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

//#define PIP_DEADLOCK_WARN

#include <pip.h>
#include <pip_internal.h>
#include <pip_util.h>
#include <pip_gdbif_func.h>

#ifdef PIP_DUMP_TASKS
static void pip_dump_tasks( FILE *fp ) {
  void pip_dump_task( FILE *fp, pip_task_internal_t *taski ) {
    char st;
    if( taski->flag_exit ) {
      st = 'E';
    } else if( PIP_IS_RUNNING( taski ) ) {
      st = 'R';
    } else {
      st = 'S';
    }
    fprintf( fp,
	     "%d[%d] %c  SchedQL:%d  OODQ:%d  NTC:%d  StkP:%d[%d]  WU:%d  SM:%d\n",
	     taski->pipid,
	     (taski->task_sched!=NULL)?taski->task_sched->pipid:-1,
	     st,
	     (int) taski->schedq_len,
	     (int) taski->oodq_len,
	     (int) taski->ntakecare,
	     (int) taski->flag_stackp,
	     ( taski->flag_stackpp != NULL ) ?
	     ( (pip_task_internal_t*)
	       ( taski->flag_stackpp -
		 offsetof(pip_task_internal_t,flag_stackpp) ) )->pipid : -1,
	     (int) taski->flag_wakeup,
	     (int) taski->flag_semwait );
  }
  int i;

  fflush( NULL );
  if( fp == NULL ) fp = stderr;
  for( i=0; i<pip_root_->ntasks; i++ ) {
    if( pip_root_->tasks[i].pipid != PIP_PIPID_NULL ) {
      pip_dump_task( fp, &pip_root_->tasks[i] );
    }
  }
  pip_dump_task( fp, pip_root_->task_root );
}
#endif

void pip_deadlock_inc_( void ) {
#ifdef PIP_DEADLOCK_WARN
  pip_atomic_fetch_and_add( &pip_root_->ntasks_blocking, 1 );
  DBGF( "nblks:%d ntasks:%d",
	(int) pip_root_->ntasks_blocking,
	(int) pip_root_->ntasks_count );
  if( pip_root_->ntasks_blocking == pip_root_->ntasks_count ) {
    static int flag_reported = 0;
    if( !flag_reported ) {
      flag_reported = 1;
      pip_kill( PIP_PIPID_ROOT, SIGHUP );
    }
  }
#endif
}

void pip_deadlock_dec_( void ) {
#ifdef PIP_DEADLOCK_WARN
  pip_atomic_sub_and_fetch( &pip_root_->ntasks_blocking, 1 );
  DBGF( "nblks:%d ntasks:%d",
	(int) pip_root_->ntasks_blocking,
	(int) pip_root_->ntasks_count );
#endif
}

void pip_set_extval_( pip_task_internal_t *taski, int extval ) {
  ENTER;
  if( !taski->flag_exit ) {
    taski->flag_exit = PIP_EXITED;
    /* mark myself as exited */
    DBGF( "PIPID:%d[%d] extval:%d",
	  taski->pipid, taski->task_sched->pipid, extval );
    if( taski->annex->status == 0 ) {
      taski->annex->status = PIP_W_EXITCODE( extval, 0 );
    }
    pip_gdbif_exit_( taski, extval );
    pip_memory_barrier();
    pip_gdbif_hook_after_( taski );
  }
  DBGF( "extval: 0x%x(0x%x)", extval, taski->annex->status );
  RETURNV;
}

static void pip_set_status_( pip_task_internal_t *taski, int status ) {
  ENTER;
  if( !taski->flag_exit ) {
    taski->flag_exit = PIP_EXITED;
    /* mark myself as exited */
    DBGF( "PIPID:%d[%d] status:0x%x",
	  taski->pipid, taski->task_sched->pipid, status );
    if( taski->annex->status == 0 ) {
      taski->annex->status = status;
    }
    pip_gdbif_exit_( taski, status );
    pip_memory_barrier();
    pip_gdbif_hook_after_( taski );
  }
  DBGF( "status: 0x%x(0x%x)", status, taski->annex->status );
  RETURNV;
}

void pip_finalize_task_RC_( pip_task_internal_t *taski ) {
  void pip_blocking_fin( pip_blocking_t *blocking ) {
    //(void) sem_destroy( &blocking->semaphore );
    (void) sem_destroy( blocking );
  }
  DBGF( "pipid=%d  status=0x%x", taski->pipid, taski->annex->status );

  pip_gdbif_finalize_task_( taski );
  /* dlclose() and free() must be called only from the root process since */
  /* corresponding dlmopen() and malloc() is called by the root process   */
  if( taski->annex->loaded != NULL ) {
    pip_dlclose( taski->annex->loaded );
    taski->annex->loaded = NULL;
  }
  if( taski->annex->pip_root_p != NULL ) {
    *taski->annex->pip_root_p = NULL;
  }
  if( taski->annex->pip_task_p != NULL ) {
    *taski->annex->pip_task_p = NULL;
  }
  PIP_FREE( taski->annex->args.prog );
  taski->annex->args.prog = NULL;
  PIP_FREE( taski->annex->args.prog_full );
  taski->annex->args.prog_full = NULL;
  PIP_FREE( taski->annex->args.funcname );
  taski->annex->args.funcname = NULL;
  PIP_FREE( taski->annex->args.argv );
  taski->annex->args.argv = NULL;
  PIP_FREE( taski->annex->args.envv );
  taski->annex->args.envv = NULL;
  PIP_FREE( taski->annex->args.fd_list );
  taski->annex->args.fd_list = NULL;
  pip_blocking_fin( &taski->annex->sleep );
  pip_reset_task_struct_( taski );
}

static int 
pip_wait_syscall( pip_task_internal_t *taski, int flag_blk ) {
  int err = 0;

  ENTERF( "PIPID:%d", taski->pipid );
  if( taski->flag_exit  == PIP_EXIT_WAITED ||
      taski->annex->tid <= 0 ) {
    /* already waited */
    RETURN( ESRCH );
  }
  if( pip_is_threaded_() ) {	/* thread mode */
    if( flag_blk ) {
      err = pthread_join( taski->annex->thread, NULL );
    } else {
      err = pthread_tryjoin_np( taski->annex->thread, NULL );
      DBGF( "err:%d", err );
      if( err == EBUSY ) err = ECHILD;
    }
    if( !err ) {
      pip_set_status_( taski, 0 );

    } else if( err == ESRCH &&
	       taski->annex->symbols.pip_set_tid == NULL ) {
      /* workaround */
      if( pip_raise_signal_( taski, 0 ) == ESRCH ) {
	/* to make sure if the task is running or not */
	err = 0;
      } else if( flag_blk ) {
	while( pip_raise_signal_( taski, 18 ) != ESRCH ) {
	  struct timespec ts;
	  ts.tv_sec  = 0;
	  ts.tv_nsec = 10*1000*1000; /* 10ms */
	  nanosleep( &ts, NULL );
	}
	err = 0;
      }
      DBGF( "err:%d", err );
    }
    
  } else {			/* process mode */
    pid_t tid;
    int   status  = 0;
    int   options = 0;
    
#ifdef __WALL
    /* __WALL: Wait for all children, regardless of type */
    /* ("clone" or "non-clone") [from the man page]      */
    options |= __WALL;
#endif
#ifdef __WCLONE
    /* __WCLONE: Wait for all clone()ed processes */
    options |= __WCLONE;
#endif
    if( !flag_blk ) options |= WNOHANG;
    
    DBGF( "calling waitpid()  task:%p  tid:%d  pipid:%d",
	  taski, taski->annex->tid, taski->pipid );
    while( 1 ) {
      errno = 0;
      tid = waitpid( taski->annex->tid, &status, options );
      err = errno;
      if( err == EINTR ) continue;
      if( err ) break;
      if( tid != taski->annex->tid ) {
	err = ECHILD;
      } else if( tid > 0 ) {
	if( WIFSTOPPED( status ) || WIFCONTINUED( status ) ) {
	  if( flag_blk ) continue;
	  err = ECHILD;
	}
      }
      break;
    }
    DBGF( "waitpid(tid=%d,status=0x%x)=%d (err=%d)", 
	  taski->annex->tid, status, tid, err );
    
    if( !err && WIFSIGNALED( status ) ) {
      int sig = WTERMSIG( status );
      pip_warn_mesg( "PiP Task [%d] terminated by '%s' (%d) signal",
		     taski->pipid, strsignal(sig), sig );
	pip_set_status_( taski, status );
    }
  }
  if( !err ) {
    DBGF( "PIPID:%d terminated", taski->pipid );
    taski->flag_exit     = PIP_EXIT_WAITED;
    taski->annex->thread = 0;
    taski->annex->tid    = 0;
  }
  RETURN( err );
}

static int pip_check_task( pip_task_internal_t *taski ) {
  ENTER;
  if( taski->flag_exit == PIP_EXIT_WAITED ) {
    RETURN( 1 );
  } else if( taski->annex->tid    == 0 ||
	     taski->annex->thread == 0 ) {
    /* not yet (empty slot) or terminated already */
    RETURN( 0 );
  } else if( taski->type != PIP_TYPE_NONE ) {
    if( pip_is_threaded_() ) {
      if( taski->annex->flag_sigchld ) {
	/* if sigchld is really raised, then blocking wait */
	taski->annex->flag_sigchld = 0;
	if( pip_wait_syscall( taski, 1 ) == 0 ) {
	  RETURN( 1 );
	}
      } else if( pip_wait_syscall( taski, 0 ) == 0 ) {
	RETURN( 1 );
      }
    } else {			/* process mode */
      if( pip_wait_syscall( taski, 0 ) == 0 ) {
	RETURN( 1 );
      }
    }
  }
  RETURN( 0 );
}

static int pip_nonblocking_waitany( void ) {
  pip_task_internal_t 	*taski;
  static int		start = 0;
  int			id, pipid = PIP_PIPID_NULL;

  DBG;
  pip_spin_lock( &pip_root_->lock_tasks );
  /*** start lock region ***/
  for( id=start; id<pip_root_->ntasks; id++ ) {
    taski = &pip_root_->tasks[id];
    if( pip_check_task( taski ) ) goto found;
  }
  for( id=0; id<start; id++ ) {
    taski = &pip_root_->tasks[id];
    if( pip_check_task( taski ) ) goto found;
  }
  goto unlock;
 found:
  pipid = id ++;
  start = ( id < pip_root_->ntasks ) ? id : 0;
 unlock:
  /*** end lock region ***/
  pip_spin_unlock( &pip_root_->lock_tasks );

  if( pipid == PIP_PIPID_NULL ) {
    for( id=0; id<pip_root_->ntasks; id++ ) {
      taski = &pip_root_->tasks[id];
      if( taski->type != PIP_TYPE_NONE ) goto success;
    }
    /* no live tasks */
    pipid = PIP_PIPID_ANY;
  }
 success:
  return( pipid );
}

static int pip_blocking_waitany( void ) {
  int	pipid;
  DBG;
  while( 1 ) {
    sigset_t	sigset;
    pipid = pip_nonblocking_waitany();
    DBGF( "pip_nonblocking_waitany() = %d", pipid );
    if( pipid != PIP_PIPID_NULL ) break;

    ASSERT( sigemptyset( &sigset ) );
    (void) sigsuspend( &sigset ); /* always returns EINTR */
  }
  return( pipid );
}

int pip_wait( int pipid, int *statusp ) {
  pip_task_internal_t	*taski;
  int 			err = 0;

  ENTER;
  if( !pip_isa_root() )                           RETURN( EPERM   );
  if( ( err = pip_check_pipid_( &pipid ) ) != 0 ) RETURN( err     );
  if( pipid == PIP_PIPID_ROOT )                   RETURN( EDEADLK );
  DBGF( "PIPID:%d", pipid );

  taski = pip_get_task_( pipid );
  if( taski->type == PIP_TYPE_NONE ) {
    /* already waited */
    err = ESRCH;
  } else {
    while( 1 ) {
      sigset_t	sigset;
      if( pip_check_task( taski ) ) {
	if( statusp != NULL ) {
	  *statusp = taski->annex->status;
	}
	pip_finalize_task_RC_( taski );
	break;
      }
      ASSERT( sigemptyset( &sigset ) );
      (void) sigsuspend( &sigset ); /* always returns EINTR */
    }    
  } 
  RETURN( err );
}

int pip_trywait( int pipid, int *statusp ) {
  pip_task_internal_t 	*taski;
  int err;

  ENTER;
  if( !pip_isa_root() )                           RETURN( EPERM   );
  if( ( err = pip_check_pipid_( &pipid ) ) != 0 ) RETURN( err     );
  if( pipid == PIP_PIPID_ROOT )                   RETURN( EDEADLK );
  DBGF( "PIPID:%d", pipid );

  taski = pip_get_task_( pipid );
  if( ( err = pip_wait_syscall( taski, 1 ) ) == 0 ) {
    if( statusp != NULL ) {
      *statusp = taski->annex->status;
    }
    pip_finalize_task_RC_( taski );
  }
  RETURN( err );
}

int pip_wait_any( int *pipidp, int *statusp ) {
  int pipid, err = 0;

  ENTER;
  if( !pip_isa_root() ) RETURN( EPERM   );
  
  pipid = pip_blocking_waitany();
  if( pipid == PIP_PIPID_ANY ) {
    err = ECHILD;
  } else {
    pip_task_internal_t *taski = pip_get_task_( pipid );
    if( pipidp != NULL ) {
      *pipidp = pipid;
    }
    if( statusp != NULL ) {
      *statusp = taski->annex->status;
    }
    pip_finalize_task_RC_( taski );
  }
  RETURN( err );
}

int pip_trywait_any( int *pipidp, int *statusp ) {
  int pipid, err = 0;

  ENTER;
  if( !pip_isa_root() ) RETURN( EPERM   );
  
  pipid = pip_nonblocking_waitany();
  if( pipid == PIP_PIPID_NULL ) {
    err = ESRCH;
  } else if( pipid == PIP_PIPID_ANY ) {
    err = ECHILD;
  } else {
    pip_task_internal_t *taski = pip_get_task_( pipid );
    if( pipidp != NULL ) {
      *pipidp = pipid;
    }
    if( statusp != NULL ) {
      *statusp = taski->annex->status;
    }
    pip_finalize_task_RC_( taski );
  }
  RETURN( err );
}
