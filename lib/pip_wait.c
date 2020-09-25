/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019$
 * $PIP_VERSION: Version 2.0.0$
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

//#define PIP_DEADLOCK_WARN

#include <pip_internal.h>

static void
pip_set_exit_status( pip_task_t *task, int status ) {
  ENTER;
  if( !task->flag_exit ) {
    task->flag_exit = PIP_EXITED;
    /* mark myself as exited */
    DBGF( "PIPID:%d status:0x%x", task->pipid, status );
    if( task->status == 0 ) task->status = status;
  }
  DBGF( "status: 0x%x(0x%x)", status, task->status );
  RETURNV;
}

void pip_finalize_task( pip_task_t *task ) {
  ENTERF( "pipid=%d  status=0x%x", task->pipid, task->status );

  pip_gdbif_finalize_task( task );

  /* dlclose() and free() must be called only from the root process since */
  /* corresponding dlmopen() and malloc() is called by the root process   */
  if( task->loaded != NULL ) {
    /***** do not call dlclose() *****/
    //pip_dlclose( taski->loaded );
    //taski->loaded = NULL;
  }
  //pip_reset_task_struct( task );
}

static int
pip_wait_thread( pip_task_t *task, int flag_blk ) {
  int err = 0;

  ENTERF( "PIPID:%d", task->pipid );
  if( flag_blk ) {
    err = pthread_join( task->thread, NULL );
    if( err ) {
      DBGF( "pthread_timedjoin_np(): %s", strerror(err) );
    }
  } else {
    err = pthread_tryjoin_np( task->thread, NULL );
    DBGF( "pthread_tryjoin_np(): %s", strerror(err) );
  }
  if( err ) err = ECHILD;
  /* workaround (make sure) */
  if( err != 0 && task->flag_sigchld ) {
    struct timespec ts;
    char path[128];
    struct stat stbuf;

    snprintf( path, 128, "/proc/%d/task/%d",
	      getpid(), task->tid );
    while( 1 ) {
      errno = 0;
      (void) stat( path, &stbuf );
      err = errno;
      DBGF( "stat(%s): %s", path, strerror( err ) );
      if( err == ENOENT ) {
	err = 0;
	break;
      }
      if( !flag_blk ) break;
      ts.tv_sec  = 0;
      ts.tv_nsec = 100*1000;
      nanosleep( &ts, NULL );
    }
  }
  RETURN( err );
}

static int
pip_wait_syscall( pip_task_t *task, int flag_blk ) {
  int err = 0;

  ENTERF( "PIPID:%d", task->pipid );
  if( task->flag_exit  == PIP_EXIT_WAITED ||
      task->tid <= 0 ) {
    /* already waited */
    RETURN( ECHILD );
  }
  if( pip_is_threaded_() ) {	/* thread mode */
    err = pip_wait_thread( task, flag_blk );
    if( !err ) pip_set_exit_status( task, 0 );

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
	  task, task->tid, task->pipid );
    while( 1 ) {
      errno = 0;
      tid = waitpid( task->tid, &status, options );
      err = errno;
      if( err == EINTR ) continue;
      if( err == ESRCH ) {
	err = ECHILD;
	break;
      }
      if( err ) break;
      if( tid != task->tid ) {
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
	  task->tid, status, tid, err );

    if( !err && WIFSIGNALED( status ) ) {
      int sig = WTERMSIG( status );
      pip_warn_mesg( "PiP Task [%d] terminated by '%s' (%d) signal",
		     task->pipid, strsignal(sig), sig );
	pip_set_exit_status( task, status );
    }
  }
  if( !err ) {
    DBGF( "PIPID:%d terminated", task->pipid );
    task->type      = PIP_TYPE_NULL;
    task->thread    = 0;
    task->tid       = 0;
  }
  RETURN( err );
}

static int pip_wait_task( pip_task_t *task ) {
  ENTERF( "PIPID:%d", task->pipid );
  if( task->flag_exit == PIP_EXIT_WAITED ) {
    goto found;
  } else if( task->type   != PIP_TYPE_NULL &&
	     task->tid    != 0            &&
	     task->thread != 0 ) {
    if( pip_is_threaded_() ) {
      if( task->flag_sigchld ) {
	/* if sigchld is really raised, then blocking wait */
	task->flag_sigchld = 0;
	if( pip_wait_syscall( task, 1 ) == 0 ) {
	  goto found;
	}
      } else if( pip_wait_syscall( task, 0 ) == 0 ) {
	goto found;
      }
    } else {			/* process mode */
      if( pip_wait_syscall( task, 0 ) == 0 ) {
	goto found;
      }
    }
  }
  RETURN( 0 );			/* not yet */
 found:
  RETURN( 1 );
}

static int pip_nonblocking_waitany( void ) {
  pip_task_t 	*task;
  static int	start = 0;
  int		id, pipid = PIP_PIPID_NULL;

  pip_spin_lock( &pip_root->lock_tasks );
  /*** start lock region ***/
  for( id=start; id<pip_root->ntasks; id++ ) {
    task = &pip_root->tasks[id];
    if( pip_wait_task( task ) ) goto found;
  }
  for( id=0; id<start; id++ ) {
    task = &pip_root->tasks[id];
    if( pip_wait_task( task ) ) goto found;
  }
  goto unlock;
 found:
  pipid = id ++;
  start = ( id < pip_root->ntasks ) ? id : 0;
 unlock:
  /*** end lock region ***/
  pip_spin_unlock( &pip_root->lock_tasks );

  /* chekc if there is a live task or not */
  if( pipid == PIP_PIPID_NULL ) {
    for( id=0; id<pip_root->ntasks; id++ ) {
      task = &pip_root->tasks[id];
      if( task->type != PIP_TYPE_NULL ) goto success;
    }
    /* no live tasks */
    pipid = PIP_PIPID_ANY;
  }
 success:
  return( pipid );
}

static int pip_blocking_waitany( void ) {
  int	pipid;

  while( 1 ) {
    pipid = pip_nonblocking_waitany();
    DBGF( "pip_nonblocking_waitany() = %d", pipid );
    if( pipid != PIP_PIPID_NULL ) break;
    ASSERT( pip_signal_wait( SIGCHLD ) );
  }
  return( pipid );
}

int pip_wait( int pipid, int *statusp ) {
  pip_task_t	*task;
  int 		err = 0;

  ENTER;
  if( !pip_is_initialized() )                    RETURN( EPERM   );
  if( !pip_isa_root() )                          RETURN( EPERM   );
  if( ( err = pip_check_pipid( &pipid ) ) != 0 ) RETURN( err     );
  if( pipid == PIP_PIPID_ROOT )                  RETURN( EDEADLK );
  DBGF( "PIPID:%d", pipid );

  task = pip_get_task_( pipid );
  if( task->type == PIP_TYPE_NULL ) {
    /* already waited */
    err = ECHILD;
  } else {
    while( 1 ) {
      if( pip_wait_task( task ) ) {
	if( statusp != NULL ) {
	  *statusp = task->status;
	}
	pip_finalize_task( task );
	break;
      }
      ASSERT( pip_signal_wait( SIGCHLD ) );
    }
  }
  RETURN( err );
}

int pip_trywait( int pipid, int *statusp ) {
  pip_task_t 	*task;
  int 		err;

  ENTER;
  if( !pip_is_initialized() )                    RETURN( EPERM   );
  if( !pip_isa_root() )                          RETURN( EPERM   );
  if( ( err = pip_check_pipid( &pipid ) ) != 0 ) RETURN( err     );
  if( pipid == PIP_PIPID_ROOT )                  RETURN( EDEADLK );
  DBGF( "PIPID:%d", pipid );

  task = pip_get_task_( pipid );
  if( ( err = pip_wait_syscall( task, 0 ) ) == 0 ) {
    if( statusp != NULL ) {
      *statusp = task->status;
    }
    pip_finalize_task( task );
  }
  RETURN( err );
}

int pip_wait_any( int *pipidp, int *statusp ) {
  int pipid, err = 0;

  ENTER;
  if( !pip_is_initialized() ) RETURN( EPERM );
  if( !pip_isa_root() )       RETURN( EPERM );

  pipid = pip_blocking_waitany();
  if( pipid == PIP_PIPID_ANY ) {
    err = ECHILD;
  } else {
    pip_task_t *task = pip_get_task_( pipid );
    if( pipidp != NULL ) {
      *pipidp = pipid;
    }
    if( statusp != NULL ) {
      *statusp = task->status;
    }
    pip_finalize_task( task );
  }
  RETURN( err );
}

int pip_trywait_any( int *pipidp, int *statusp ) {
  int pipid, err = 0;

  ENTER;
  if( !pip_is_initialized() ) RETURN( EPERM );
  if( !pip_isa_root() )       RETURN( EPERM );

  pipid = pip_nonblocking_waitany();
  if( pipid == PIP_PIPID_NULL ) {
    err = ECHILD;
  } else if( pipid == PIP_PIPID_ANY ) {
    err = ECHILD;
  } else {
    pip_task_t *task = pip_get_task_( pipid );
    if( pipidp != NULL ) {
      *pipidp = pipid;
    }
    if( statusp != NULL ) {
      *statusp = task->status;
    }
    pip_finalize_task( task );
  }
  RETURN( err );
}
