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

//#define PIP_DEADLOCK_WARN

#include <pip_internal.h>
#include <pip_gdbif_func.h>

#ifdef PIP_DUMP_TASKS
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
	   (int) taski->refcount,
	   (int) taski->flag_stackp,
	   ( taski->flag_stackpp != NULL ) ?
	   ( (pip_task_internal_t*)
	     ( taski->flag_stackpp -
	       offsetof(pip_task_internal_t,flag_stackpp) ) )->pipid : -1,
	   (int) taski->flag_wakeup,
	   (int) taski->flag_semwait );
}

static void pip_dump_all_tasks( FILE *fp ) {
  int i;

  fflush( NULL );
  if( fp == NULL ) fp = stderr;
  for( i=0; i<pip_root->ntasks; i++ ) {
    if( pip_root->tasks[i].pipid != PIP_PIPID_NULL ) {
      pip_dump_task( fp, &pip_root->tasks[i] );
    }
  }
  pip_dump_task( fp, pip_root->task_root );
}
#endif

void pip_deadlock_inc( void ) {
#ifdef PIP_DEADLOCK_WARN
  pip_atomic_fetch_and_add( &pip_root->ntasks_blocking, 1 );
  DBGF( "nblks:%d ntasks:%d",
	(int) pip_root->ntasks_blocking,
	(int) pip_root->ntasks_count );
  if( pip_root->ntasks_blocking == pip_root->ntasks_count ) {
    static int flag_reported = 0;
    if( !flag_reported ) {
      flag_reported = 1;
      pip_kill( PIP_PIPID_ROOT, SIGHUP );
    }
  }
#endif
}

void pip_deadlock_dec( void ) {
#ifdef PIP_DEADLOCK_WARN
  pip_atomic_sub_and_fetch( &pip_root->ntasks_blocking, 1 );
  DBGF( "nblks:%d ntasks:%d",
	(int) pip_root->ntasks_blocking,
	(int) pip_root->ntasks_count );
#endif
}

static void
pip_set_exit_status( pip_task_internal_t *taski, int status ) {
  ENTER;
  if( !taski->flag_exit ) {
    taski->flag_exit = PIP_EXITED;
    /* mark myself as exited */
    DBGF( "PIPID:%d[%d] status:0x%x",
	  taski->pipid, taski->task_sched->pipid, status );
    if( taski->annex->status == 0 ) {
      taski->annex->status = status;
    }
  }
  DBGF( "status: 0x%x(0x%x)", status, taski->annex->status );
  RETURNV;
}

void pip_finalize_task_RC( pip_task_internal_t *taski ) {
  ENTERF( "pipid=%d  status=0x%x", taski->pipid, taski->annex->status );

  pip_gdbif_finalize_task( taski );

  /* dlclose() and free() must be called only from the root process since */
  /* corresponding dlmopen() and malloc() is called by the root process   */
  if( taski->annex->loaded != NULL ) {
    /***** do not call dlclose() *****/
    //pip_dlclose( taski->annex->loaded );
    //taski->annex->loaded = NULL;
  }
#ifdef AH
  PIP_FREE( taski->annex->args.prog );
  taski->annex->args.prog = NULL;
  PIP_FREE( taski->annex->args.prog_full );
  taski->annex->args.prog_full = NULL;
  PIP_FREE( taski->annex->args.funcname );
  taski->annex->args.funcname = NULL;
  PIP_FREE( taski->annex->args.start_arg );
  taski->annex->args.start_arg = NULL;
#endif
#ifdef AH
  PIP_FREE( taski->annex->args.argvec.vec );
  taski->annex->args.argvec.vec  = NULL;
  PIP_FREE( taski->annex->args.argvec.strs );
  taski->annex->args.argvec.strs = NULL;
#endif
  /* since envvec might be free()ed by calling realloc() */
  /* in glibc and envvec cannot be free()ed here         */
  PIP_FREE( taski->annex->args.fd_list );
  taski->annex->args.fd_list = NULL;
  pip_sem_fin( &taski->annex->sleep );

  pip_reset_task_struct( taski );
}

static int
pip_wait_thread( pip_task_internal_t *taski, int flag_blk ) {
  int err = 0;

  ENTERF( "PIPID:%d", taski->pipid );
  if( flag_blk ) {
    err = pthread_join( taski->annex->thread, NULL );
    if( err ) {
      DBGF( "pthread_timedjoin_np(): %s", strerror(err) );
    }
  } else {
    err = pthread_tryjoin_np( taski->annex->thread, NULL );
    DBGF( "pthread_tryjoin_np(): %s", strerror(err) );
  }
  if( err ) err = ECHILD;
  /* workaround (make sure) */
  if( err != 0 &&
      taski->annex->flag_sigchld ) {
    struct timespec ts;
    char path[128];
    struct stat stbuf;

    snprintf( path, 128, "/proc/%d/task/%d",
	      getpid(), taski->annex->tid );
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
  if( !err ) pip_glibc_unlock();
  RETURN( err );
}

static int
pip_wait_syscall( pip_task_internal_t *taski, int flag_blk ) {
  int err = 0;

  ENTERF( "PIPID:%d", taski->pipid );
  if( taski->flag_exit  == PIP_EXIT_WAITED ||
      taski->annex->tid <= 0 ) {
    /* already waited */
    RETURN( ECHILD );
  }
  if( pip_is_threaded_() ) {	/* thread mode */
    err = pip_wait_thread( taski, flag_blk );
    if( !err ) pip_set_exit_status( taski, 0 );

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
      if( err == ESRCH ) {
	err = ECHILD;
	break;
      }
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
	pip_set_exit_status( taski, status );
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

static int pip_wait_task( pip_task_internal_t *taski ) {
  ENTERF( "PIPID:%d", taski->pipid );
  if( taski->flag_exit == PIP_EXIT_WAITED ) {
    goto found;
  } else if( taski->type          != PIP_TYPE_NONE &&
	     taski->annex->tid    != 0             &&
	     taski->annex->thread != 0 ) {
    if( pip_is_threaded_() ) {
      if( taski->annex->flag_sigchld ) {
	/* if sigchld is really raised, then blocking wait */
	taski->annex->flag_sigchld = 0;
	if( pip_wait_syscall( taski, 1 ) == 0 ) {
	  goto found;
	}
      } else if( pip_wait_syscall( taski, 0 ) == 0 ) {
	goto found;
      }
    } else {			/* process mode */
      if( pip_wait_syscall( taski, 0 ) == 0 ) {
	goto found;
      }
    }
  }
  RETURN( 0 );			/* not yet */
 found:
  RETURN( 1 );
}

static int pip_nonblocking_waitany( void ) {
  pip_task_internal_t 	*taski;
  static int		start = 0;
  int			id, pipid = PIP_PIPID_NULL;

  pip_spin_lock( &pip_root->lock_tasks );
  /*** start lock region ***/
  for( id=start; id<pip_root->ntasks; id++ ) {
    taski = &pip_root->tasks[id];
    if( pip_wait_task( taski ) ) goto found;
  }
  for( id=0; id<start; id++ ) {
    taski = &pip_root->tasks[id];
    if( pip_wait_task( taski ) ) goto found;
  }
  goto unlock;
 found:
  pipid = id ++;
  start = ( id < pip_root->ntasks ) ? id : 0;
 unlock:
  /*** end lock region ***/
  pip_spin_unlock( &pip_root->lock_tasks );

  if( pipid == PIP_PIPID_NULL ) {
    for( id=0; id<pip_root->ntasks; id++ ) {
      taski = &pip_root->tasks[id];
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

  while( 1 ) {
    pipid = pip_nonblocking_waitany();
    DBGF( "pip_nonblocking_waitany() = %d", pipid );
    if( pipid != PIP_PIPID_NULL ) break;
    ASSERT( pip_signal_wait( SIGCHLD ) );
  }
  return( pipid );
}

int pip_wait( int pipid, int *statusp ) {
  pip_task_internal_t	*taski;
  int 			err = 0;

  ENTER;
  if( !pip_is_initialized() )                    RETURN( EPERM   );
  if( !pip_isa_root() )                          RETURN( EPERM   );
  if( ( err = pip_check_pipid( &pipid ) ) != 0 ) RETURN( err     );
  if( pipid == PIP_PIPID_ROOT )                  RETURN( EDEADLK );
  DBGF( "PIPID:%d", pipid );

  taski = pip_get_task( pipid );
  if( taski->type == PIP_TYPE_NONE ) {
    /* already waited */
    err = ECHILD;
  } else {
    while( 1 ) {
      if( pip_wait_task( taski ) ) {
	if( statusp != NULL ) {
	  *statusp = taski->annex->status;
	}
	pip_finalize_task_RC( taski );
	break;
      }
      ASSERT( pip_signal_wait( SIGCHLD ) );
    }
  }
  RETURN( err );
}

int pip_trywait( int pipid, int *statusp ) {
  pip_task_internal_t 	*taski;
  int err;

  ENTER;
  if( !pip_is_initialized() )                    RETURN( EPERM   );
  if( !pip_isa_root() )                          RETURN( EPERM   );
  if( ( err = pip_check_pipid( &pipid ) ) != 0 ) RETURN( err     );
  if( pipid == PIP_PIPID_ROOT )                  RETURN( EDEADLK );
  DBGF( "PIPID:%d", pipid );

  taski = pip_get_task( pipid );
  if( ( err = pip_wait_syscall( taski, 0 ) ) == 0 ) {
    if( statusp != NULL ) {
      *statusp = taski->annex->status;
    }
    pip_finalize_task_RC( taski );
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
    pip_task_internal_t *taski = pip_get_task( pipid );
    if( pipidp != NULL ) {
      *pipidp = pipid;
    }
    if( statusp != NULL ) {
      *statusp = taski->annex->status;
    }
    pip_finalize_task_RC( taski );
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
    pip_task_internal_t *taski = pip_get_task( pipid );
    if( pipidp != NULL ) {
      *pipidp = pipid;
    }
    if( statusp != NULL ) {
      *statusp = taski->annex->status;
    }
    pip_finalize_task_RC( taski );
  }
  RETURN( err );
}
