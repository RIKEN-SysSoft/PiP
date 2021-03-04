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
 * System Software Development Team, 2016-2021
 * $
 * $PIP_VERSION: Version 3.0.0$
 *
 * $Author: Atsushi Hori (R-CCS)
 * Query:   procinproc-info@googlegroups.com
 * User ML: procinproc-users@googlegroups.com
 * $
 */

//#define PIP_DEADLOCK_WARN

#include <pip/pip_internal.h>
#include <pip/pip_gdbif_func.h>

#ifdef PIP_DUMP_TASKS
void pip_dump_task( FILE *fp, pip_task_internal_t *taski ) {
  char st;
  if( AA(taski)->flag_exit ) {
    st = 'E';
  } else if( PIP_IS_RUNNING( taski ) ) {
    st = 'R';
  } else {
    st = 'S';
  }
  fprintf( fp,
	   "%d[%d] %c  SchedQL:%d  OODQ:%d  RFC:%d  WU:%d  SM:%d\n",
	   TA(taski)->pipid,
	   (TA(taski)->task_sched!=NULL)?TA(TA(taski)->task_sched)->pipid:-1,
	   st,
	   (int) TA(taski)->schedq_len,
	   (int) TA(taski)->oodq_len,
	   (int) TA(taski)->refcount,
	   (int) TA(taski)->flag_wakeup,
	   (int) TA(taski)->flag_semwait );
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
  if( !AA(taski)->flag_exit ) {
    AA(taski)->flag_exit = PIP_EXITED;
    /* mark myself as exited */
    DBGF( "PIPID:%d[%d] status:0x%x",
	  TA(taski)->pipid, TA(TA(taski)->task_sched)->pipid, status );
    if( AA(taski)->status == 0 ) {
      AA(taski)->status = status;
    }
  }
  DBGF( "status: 0x%x(0x%x)", status, AA(taski)->status );
  RETURNV;
}

void pip_finalize_task_RC( pip_task_internal_t *taski ) {
  ENTERF( "pipid=%d  status=0x%x", TA(taski)->pipid, AA(taski)->status );

  pip_gdbif_finalize_task( taski );

  /* dlclose() and free() must be called only from the root process since */
  /* corresponding dlmopen() and malloc() is called by the root process   */
  if( MA(taski)->loaded != NULL ) {
    /***** do not call dlclose() *****/
    //pip_dlclose( AA(taski)->loaded );
    //AA(taski)->loaded = NULL;
  }
#ifdef DONOT_FREE_THEM
  PIP_FREE( MA(taski)->args.funcname );
  MA(taski)->args.funcname = NULL;
  PIP_FREE( MA(taski)->args.start_arg );
  MA(taski)->args.start_arg = NULL;
  PIP_FREE( MA(taski)->args.argvec.vec );
  MA(taski)->args.argvec.vec  = NULL;
  PIP_FREE( MA(taski)->args.argvec.strs );
  MA(taski)->args.argvec.strs = NULL;
#endif
  /* since envvec might be free()ed by calling realloc() */
  /* in glibc and envvec cannot be free()ed here         */
  PIP_FREE( MA(taski)->args.fd_list );
  MA(taski)->args.fd_list = NULL;
  pip_sem_fin( &AA(taski)->sleep );
  //pip_reset_task_struct( taski );
}

static int
pip_wait_thread( pip_task_internal_t *taski, int flag_blk ) {
  int err = 0;

  ENTERF( "PIPID:%d", TA(taski)->pipid );
  if( !MA(taski)->thread ) {
    err = ECHILD;
  } else {
    if( flag_blk ) {
      err = pthread_join( MA(taski)->thread, NULL );
      if( err ) {
	DBGF( "pthread_timedjoin_np(): %s", strerror(err) );
      }
    } else {
      err = pthread_tryjoin_np( MA(taski)->thread, NULL );
      DBGF( "pthread_tryjoin_np(): %s", strerror(err) );
    }
    if( err ) err = ECHILD;
  }
  /* workaround (make sure) */
  if( err && AA(taski)->flag_sigchld ) {
    struct timespec ts;
    char path[128];
    struct stat stbuf;

    snprintf( path, 128, "/proc/%d/task/%d",
	      getpid(), AA(taski)->tid );
    while( 1 ) {
      errno = 0;
      (void) stat( path, &stbuf );
      err = errno;
      DBGF( "stat(%s): %s", path, strerror( err ) );
      if( err == ENOENT ) {	/* thread is gone */
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
pip_wait_syscall( pip_task_internal_t *taski, int flag_blk ) {
  int err = 0;

  ENTERF( "PIPID:%d", TA(taski)->pipid );
  if( TA(taski)->type == PIP_TYPE_NULL ) {
    /* already waited */
    RETURN( ECHILD );
  }
  if( pip_is_threaded_() ) {	/* thread mode */
    if( AA(taski)->flag_sigchld ) {
      /* if sigchld is really raised, then blocking wait */
      AA(taski)->flag_sigchld = 0;
      err = pip_wait_thread( taski, 1 );
    } else {
      err = pip_wait_thread( taski, flag_blk );
    }
    if( !err ) pip_set_exit_status( taski, 0 ); /* make sure */
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
	  taski, AA(taski)->tid, TA(taski)->pipid );
    while( 1 ) {
      errno = 0;
      tid = waitpid( AA(taski)->tid, &status, options );
      err = errno;
      if( err == EINTR ) continue;
      if( err == ESRCH ) {
	err = ECHILD;
	break;
      }
      if( err ) break;
      if( tid != AA(taski)->tid ) {
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
	  AA(taski)->tid, status, tid, err );

    if( !err && WIFSIGNALED( status ) ) {
      int sig = WTERMSIG( status );
      pip_warn_mesg( "PiP Task [%d] terminated by '%s' (%d) signal",
		     TA(taski)->pipid, strsignal(sig), sig );
	pip_set_exit_status( taski, status );
    }
  }
  if( !err ) {
    DBGF( "PIPID:%d terminated", TA(taski)->pipid );
    TA(taski)->type   = PIP_TYPE_NULL;
    AA(taski)->tid    = 0;
    MA(taski)->thread = 0;
  }
  RETURN( err );
}

#define NO_CHILDREN	(-2)
#define NOT_ATASK	(-1)
#define DONE		(0)
#define NOT_YET		(1)

static int pip_wait_task_nb( pip_task_internal_t *taski ) {
  int retval = DONE;

  ENTERF( "PIPID:%d", TA(taski)->pipid );
  if( TA(taski)->pipid < 0 || TA(taski)->type == PIP_TYPE_NULL ) {
    retval = NOT_ATASK;
  } else if( pip_wait_syscall( taski, 0 ) == 0 ) {
    retval = DONE;
  } else {
    retval = NOT_YET;
  }
  RETURN_NE( retval );
}

static int pip_nonblocking_waitany( int *pipidp ) {
  pip_task_internal_t 	*taski;
  static int		start = 0;
  int			id, st = NO_CHILDREN;
  int			flag_child = 0;

  ENTER;
  pip_spin_lock( &pip_root->lock_tasks );
  /*** start lock region ***/
  for( id=start; id<pip_root->ntasks; id++ ) {
    taski = &pip_root->tasks[id];
    if( ( st = pip_wait_task_nb( taski ) ) == 0 ) goto found;
    if( st > 0 ) flag_child = 1;
  }
  for( id=0; id<start; id++ ) {
    taski = &pip_root->tasks[id];
    if( ( st = pip_wait_task_nb( taski ) ) == 0 ) goto found;
    if( st > 0 ) flag_child = 1;
  }
  if( flag_child ) {
    st = NOT_YET;
  } else {
    st = NO_CHILDREN;
  }
  goto not_found;
 found:
  DBGF( "%d", id );
  if( !st ) *pipidp = id;
  id ++;
  start = ( id < pip_root->ntasks ) ? id : 0;
 not_found:
  /*** end lock region ***/
  pip_spin_unlock( &pip_root->lock_tasks );
  return st;
}

int pip_wait( int pipid, int *statusp ) {
  pip_task_internal_t	*taski;
  int 			err = 0;

  ENTERF( "PIPID:%d", pipid );;
  if( ( err = pip_check_pipid( &pipid ) ) != 0 ) RETURN( err     );
  if( !pip_isa_root() )                          RETURN( EPERM   );
  if( pipid == PIP_PIPID_ROOT )                  RETURN( EDEADLK );

  taski = pip_get_task( pipid );
  if( TA(taski)->type == PIP_TYPE_NULL ) {
    err = ECHILD;
  } else {
    while( 1 ) {
      if( pip_wait_task_nb( taski ) == 0 ) {
	if( statusp != NULL ) {
	  *statusp = AA(taski)->status;
	}
	pip_finalize_task_RC( taski );
	break;
      }
      ASSERT( pip_signal_wait( SIGCHLD ) == 0 );
    }
  }
  RETURN( err );
}

int pip_trywait( int pipid, int *statusp ) {
  pip_task_internal_t 	*taski;
  int err;

  ENTERF( "PIPID:%d", pipid );;
  if( ( err = pip_check_pipid( &pipid ) ) != 0 ) RETURN( err     );
  if( !pip_isa_root() )                          RETURN( EPERM   );
  if( pipid == PIP_PIPID_ROOT )                  RETURN( EDEADLK );

  taski = pip_get_task( pipid );
  if( ( err = pip_wait_syscall( taski, 0 ) ) == 0 ) {
    if( statusp != NULL ) {
      *statusp = AA(taski)->status;
    }
    pip_finalize_task_RC( taski );
  }
  RETURN( err );
}

static int
pip_do_waitany( int *pipidp, int *statusp, int flag_blk ) {
  int pipid, st, err = 0;

  ENTER;
  if( !pip_is_initialized() ) RETURN( EPERM  );
  if( !pip_isa_root() )       RETURN( EPERM  );

  if( flag_blk ) {
    while( ( st = pip_nonblocking_waitany( &pipid ) ) == NOT_YET ) {
      ASSERT( pip_signal_wait( SIGCHLD ) == 0 );
    }
  } else {
    st = pip_nonblocking_waitany( &pipid );
  }
  if( st ) {
    err = ECHILD;
  } else {
    pip_task_internal_t *taski = pip_get_task( pipid );
    if( pipidp != NULL ) {
      *pipidp = pipid;
    }
    if( statusp != NULL ) {
      *statusp = AA(taski)->status;
    }
    pip_finalize_task_RC( taski );
  }
  RETURN( err );
}

int pip_wait_any( int *pipidp, int *statusp ) {
  return pip_do_waitany( pipidp, statusp, 1 );
}

int pip_trywait_any( int *pipidp, int *statusp ) {
  return pip_do_waitany( pipidp, statusp, 0 );
}
