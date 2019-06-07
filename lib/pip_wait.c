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
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>

//#define PIP_NO_MALLOPT
//#define PIP_USE_STATIC_CTX  /* this is slower, adds 30ns */

//#define DEBUG
//#define PRINT_MAPS
//#define PRINT_FDS

/* the EVAL define symbol is to measure the time for calling dlmopen() */
//#define EVAL

#include <pip.h>
#include <pip_internal.h>
#include <pip_util.h>
#include <pip_gdbif_func.h>

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

void pip_deadlock_inc_( void ) {
  pip_atomic_fetch_and_add( &pip_root_->ntasks_blocking, 1 );
  DBGF( "blocking:%d / ntasks:%d",
	(int)pip_root_->ntasks_blocking, pip_root_->ntasks_count );
  if( pip_root_->ntasks_blocking == pip_root_->ntasks_count ) {
    pip_dump_tasks( stderr );
    pip_err_mesg( "All PiP tasks are blocked and deadlocked !!" );
    fflush( NULL );
  }
}

void pip_deadlock_dec_( void ) {
  pip_atomic_sub_and_fetch( &pip_root_->ntasks_blocking, 1 );
  DBGF( "blocking:%d / ntasks:%d",
	(int) pip_root_->ntasks_blocking, pip_root_->ntasks_count );
}

void pip_do_terminate_RC( pip_task_internal_t *taski, int extval ) {
  /* call fflush() in the target context to flush out std* messages */
  if( taski->annex->symbols.libc_fflush != NULL ) {
    taski->annex->symbols.libc_fflush( NULL );
  }
  if( !taski->flag_exit ) {
    /* mark myself as exited */
    DBGF( "PIPID:%d[%d] extval:%d",
	  taski->pipid, taski->task_sched->pipid, extval );
#ifdef __W_EXITCODE
    taski->annex->extval = __W_EXITCODE( extval, 0 );
#else
    taski->annex->extval = ( extval & 0xFF ) << 8;
#endif
    pip_gdbif_exit_RC( taski, extval );
    pip_memory_barrier();
    taski->flag_exit = PIP_EXITED;
  }
  DBGF( "extval: 0x%x(0x%x)", extval, taski->annex->extval );
}

static int pip_do_wait_( int pipid, int flag_try, int *extvalp ) {
  extern int pip_is_threaded_( void );
  extern void pip_finalize_task_( pip_task_internal_t* );
  pip_task_internal_t *taski;
  int err = 0;

  ENTER;
  taski = pip_get_task_( pipid );
  if( taski->type == PIP_TYPE_NONE ) RETURN( ESRCH );

  DBGF( "pipid=%d  type=0x%x", pipid, taski->type );
  if( pip_is_threaded_() ) {
    /* thread mode */
    DBG;
    if( flag_try ) {
      err = pthread_tryjoin_np( taski->annex->thread, NULL );
      DBGF( "pthread_tryjoin_np(%d)=%d", taski->annex->extval, err );
    } else {
      err = pthread_join( taski->annex->thread, NULL );
      DBGF( "pthread_join(%d)=%d", taski->annex->extval, err );
    }
  } else {
    /* process mode */
    int status  = 0;
    int options = 0;
    pid_t pid = taski->annex->pid;

#ifdef __WALL
    /* __WALL: Wait for all children, regardless of type */
    /* ("clone" or "non-clone") [from man page]          */
    options |= __WALL;
#endif
    if( flag_try ) options |= WNOHANG;

    DBGF( "calling waitpid()  task:%p  pid:%d  pipid:%d",
	  taski, pid, taski->pipid );
    pip_deadlock_inc_();
    while( 1 ) {
      errno = 0;
      pid = waitpid( pid, &status, options );
      if( errno != EINTR ) break;
    }
    pip_deadlock_dec_();
    DBGF( "waitpid(status=0x%x)=%d (err=%d)", status, pid, errno );

    if( pid < 0 ) {
      err = errno;
    } else {
      if( WIFSIGNALED( status ) ) {
  	int sig = WTERMSIG( status );
	pip_warn_mesg( "PiP Task [%d] terminated by '%s' (%d) signal",
		       taski->pipid, strsignal(sig), sig );
      }
      pip_do_terminate_RC( taski, status );
      pip_root_->ntasks_count --;
    }
  }
  DBG;
  if( err == 0 ) {
    if( extvalp != NULL ) *extvalp = taski->annex->extval;
    pip_finalize_task_( taski );
  }
  RETURN( err );
}

static int pip_do_wait( int pipid, int flag_try, int *extvalp ) {
  int err;

  ENTER;
  if( !pip_isa_root() )                           RETURN( EPERM   );
  if( ( err = pip_check_pipid_( &pipid ) ) != 0 ) RETURN( err     );
  if( pipid == PIP_PIPID_ROOT )                   RETURN( EDEADLK );
  RETURN( pip_do_wait_( pipid, flag_try, extvalp ) );
}

int pip_wait( int pipid, int *extvalp ) {
  ENTER;
  RETURN( pip_do_wait( pipid, 0, extvalp ) );
}

int pip_trywait( int pipid, int *extvalp ) {
  ENTER;
  RETURN( pip_do_wait( pipid, 1, extvalp ) );
}

static int pip_find_terminated( int *pipidp ) {
  static int	sidx = 0;
  int		count, i;

  pip_spin_lock( &pip_root_->lock_tasks );
  /*** start lock region ***/
  {
    count = 0;
    for( i=sidx; i<pip_root_->ntasks; i++ ) {
      if( pip_root_->tasks[i].type != PIP_TYPE_NONE ) count ++;
      if( pip_root_->tasks[i].flag_exit == PIP_EXITED ) {
	DBGF( "%d terminated", pip_root_->tasks[i].pipid );
	/* terminated task found */
	pip_root_->tasks[i].flag_exit = PIP_EXIT_FINALIZE;
	pip_memory_barrier();
	*pipidp = i;
	sidx = i + 1;
	goto found;
      }
    }
    for( i=0; i<sidx; i++ ) {
      if( pip_root_->tasks[i].type != PIP_TYPE_NONE ) count ++;
      if( pip_root_->tasks[i].flag_exit == PIP_EXITED ) {
	DBGF( "%d terminated", pip_root_->tasks[i].pipid );
	/* terminated task found */
	pip_root_->tasks[i].flag_exit = PIP_EXIT_FINALIZE;
	pip_memory_barrier();
	*pipidp = i;
	sidx = i + 1;
	goto found;
      }
    }
    *pipidp = PIP_PIPID_NULL;
  }
 found:
  /*** end lock region ***/
  pip_spin_unlock( &pip_root_->lock_tasks );

  DBGF( "PIPID=%d  count=%d", *pipidp, count );
  return( count );
}

static int pip_set_sigchld_handler( sigset_t *sigset_oldp ) {
  /* must only be called from the root */
  void pip_sighand_sigchld( int sig, siginfo_t *siginfo, void *null ) {
    ENTER;
    if( pip_root_ != NULL ) {
      struct sigaction *sigact = &pip_root_->sigact_chain;
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
  struct sigaction	*sigact_oldp = &pip_root_->sigact_chain;
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
  struct sigaction	*sigact_oldp = &pip_root_->sigact_chain;
  int 	err = 0;

  if( sigaction( SIGCHLD, sigact_oldp, NULL ) != 0 ) {
    err = errno;
  } else {
    err = pthread_sigmask( SIG_SETMASK, sigset_oldp, NULL );
    if( !err ) {
      memset( &pip_root_->sigact_chain, 0, sizeof( struct sigaction ) );
    }
  }
  RETURN( err );
}

static int pip_do_waitany( int flag_try, int *pipidp, int *extvalp ) {
  sigset_t	sigset_old;
  int		pipid, count;
  int		err = 0;

  ENTER;
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
