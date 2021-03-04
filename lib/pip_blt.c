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

#include <pip/pip_internal.h>
#include <pip/pip_gdbif_func.h>

#ifdef DEBUG
#define QUEUE_DUMP(taski,queue)			\
  do {						\
    char msg[256];				\
    char *p = msg;				\
    int l = sizeof(msg), n;			\
    if( taski != NULL ) {			\
      n = pip_taski_str( p, l, taski );		\
      p += n; l -= n;				\
    }						\
    n = snprintf( p, l, ":" );			\
    p += n; l -= n;				\
    if( PIP_TASKQ_ISEMPTY( queue ) ) {		\
      snprintf( p, l, "  EMPTY" );		\
    } else {					\
      pip_task_t *task;				\
      int iii = 0;				\
      PIP_TASKQ_FOREACH( queue, task ) {	\
	n = snprintf( p, l, " [%d]:", iii++ );	\
	p += n; l -= n;				\
	n = pip_task_str( p, l, task );		\
	p += n; l -= n;				\
      }						\
    }						\
    DBGF( "QDUMP %s: %s", #queue, msg );	\
  } while( 0 )

#else  /* DEBUG */
#define QUEUE_DUMP( taski, queue )
#endif	/* DEBUG */

void pip_sched_ood_task( pip_task_internal_t *schedi,
			 pip_task_internal_t *taski,
			 pip_task_internal_t *wakeup ) {
  int			flag;

  ENTERF( "taski:PIPID:%d  sched:PIPID:%d", TA(taski)->pipid, TA(schedi)->pipid );

  TA(taski)->task_sched = schedi;
  pip_spin_lock( &TA(schedi)->lock_oodq );
  flag = ( TA(schedi)->oodq_len == 0 );
  TA(schedi)->oodq_len ++;
  PIP_TASKQ_ENQ_LAST( &TA(schedi)->oodq, PIP_TASKQ(taski) );
  /* wakeup() must be called inside this lock region */
  if( flag && wakeup != NULL ) pip_wakeup( wakeup );
  pip_spin_unlock( &TA(schedi)->lock_oodq );
}

INLINE int pip_takein_ood_task( pip_task_internal_t *schedi ) {
  ENTER;
  pip_spin_lock( &TA(schedi)->lock_oodq );
  if( !PIP_TASKQ_ISEMPTY( &TA(schedi)->oodq ) ) {
    QUEUE_DUMP( schedi, &TA(schedi)->oodq );
    PIP_TASKQ_APPEND( &TA(schedi)->schedq, &TA(schedi)->oodq );
    TA(schedi)->schedq_len += TA(schedi)->oodq_len;
    TA(schedi)->oodq_len    = 0;
    QUEUE_DUMP( schedi, &TA(schedi)->schedq );
  }
  pip_spin_unlock( &TA(schedi)->lock_oodq );
  RETURN_NE( !PIP_TASKQ_ISEMPTY( &TA(schedi)->schedq ) );
}

void pip_terminate_task( pip_task_internal_t *self ) {
  ENTERF( "PIPID:%d  tid:%d", TA(self)->pipid, AA(self)->tid );

  ASSERT( (pid_t) AA(self)->tid == pip_gettid() );
  ASSERT( TA(self)->schedq_len == 0 );

  /* call fflush() in the target context to flush out std* messages */
  if( MA(self)->symbols.libc_fflush != NULL ) {
    MA(self)->symbols.libc_fflush( NULL );
  }
  if( MA(self)->symbols.named_export_fin != NULL ) {
    MA(self)->symbols.named_export_fin( self );
  }
  pip_gdbif_exit( self, WEXITSTATUS(AA(self)->status) );
  pip_gdbif_hook_after( self );

  DBGF( "PIPID:%d -- FORCE EXIT", TA(self)->pipid );
  if( pip_is_threaded_() ) {	/* thread mode */
    AA(self)->flag_sigchld = 1;
    /* simulate process behavior */
    ASSERT( pip_raise_signal( pip_root->task_root, SIGCHLD ) == 0 );
    DBGF( "calling pthread_exit()" );
    if( MA(self)->symbols.pthread_exit != NULL ) {
      MA(self)->symbols.pthread_exit( NULL );
    } else {
      pthread_exit( NULL );
    }
  } else {			/* process mode */
    if( MA(self)->symbols.exit != NULL ) {
      MA(self)->symbols.exit( WEXITSTATUS(AA(self)->status) );
    } else {
      exit( WEXITSTATUS(AA(self)->status) );
    }
  }
  NEVER_REACH_HERE;
}

void pip_wakeup( pip_task_internal_t *taski ) {
  pip_task_annex_t	*annex = AA(taski);
  uint32_t	 	opts = annex->opts_sync;

  ENTERF( "-- WAKEUP PIPID:%d (PID:%d) vvvvvvvvvv",
	  TA(taski)->pipid, annex->tid );
  DBGF( "schedq_len:%d  oodq_len:%d  RFC:%d  flag_exit:%d",
	TA(taski)->schedq_len, (int) TA(taski)->oodq_len,
	(int) TA(taski)->refcount, annex->flag_exit );

  pip_memory_barrier();
  annex->flag_wakeup = 1;
  DBGF( "sync_opts: 0x%x", opts );
  switch( opts ) {
  case PIP_SYNC_BUSYWAIT:
  case PIP_SYNC_YIELD:
    DBGF( "PIPID:%d -- WAKEUP (busywait or yield)", TA(taski)->pipid );
    break;
  case PIP_SYNC_BLOCKING:
  case PIP_SYNC_AUTO:
  default:
    pip_sem_post( &annex->sleep );
    break;
  }
  RETURNV;
}

static void pip_do_sleep( pip_task_internal_t *taski ) {
  pip_task_annex_t	*annex = AA(taski);
  uint32_t 		opts = annex->opts & PIP_SYNC_MASK;

  ENTERF( "sync_opts: 0x%x", opts );
  annex->opts_sync = opts;
  pip_deadlock_inc();
  {
    int	i, j;

    switch( opts & PIP_SYNC_MASK ) {
    case PIP_SYNC_BUSYWAIT:
      DBGF( "PIPID:%d -- SLEEPING (busywait) zzzzzzzz", TA(taski)->pipid );
      while( !annex->flag_wakeup ) pip_pause();
      break;
    case PIP_SYNC_YIELD:
      DBGF( "PIPID:%d -- SLEEPING (yield) zzzzzzzz", TA(taski)->pipid );
      while( 1 ) {
	for( i=0; i<pip_root->yield_iters; i++ ) {
	  pip_pause();
	  if( annex->flag_wakeup ) goto done;
	}
	pip_system_yield();
      }
      break;
    case 0:
    case PIP_SYNC_AUTO:
    default:
      DBGF( "PIPID:%d -- SLEEPING (AUTO)", TA(taski)->pipid );
      for( j=0; j<100; j++ ) {
	pip_system_yield();
	for( i=0; i<pip_root->yield_iters; i++ ) {
	  if( annex->flag_wakeup ) goto done;
	}
      }
      /* fall through */
    case PIP_SYNC_BLOCKING:
      DBGF( "PIPID:%d -- SLEEPING (blocking) zzzzzzz", TA(taski)->pipid );
      while( !annex->flag_wakeup ) {
	pip_sem_wait( &annex->sleep );
      }
      break;
    }
  }
 done:
  pip_deadlock_dec();
  annex->flag_wakeup = 0;
  pip_memory_barrier();

  DBGF( "PIPID:%d -- WOKEUP ^^^^^^^^", TA(taski)->pipid );
  RETURNV;
}

void pip_sleep( pip_task_internal_t* ) NORETURN;
void pip_sleep( pip_task_internal_t *schedi ) {
  pip_task_internal_t 	*nexti;
  pip_task_t 		*next;

  ENTERF( "PIPID:%d", TA(schedi)->pipid );
  while( 1 ) {
    while( 1 ) {
      if( pip_takein_ood_task( schedi ) ) break;
      if( pip_able_to_terminate_now( schedi ) ) {
	DBGF( "PIPID:%d -- WOKEUP to EXIT", TA(schedi)->pipid );
	pip_terminate_task( schedi );
	NEVER_REACH_HERE;
      }
      pip_do_sleep( schedi );
    }
    next = PIP_TASKQ_NEXT( &TA(schedi)->schedq );
    PIP_TASKQ_DEQ( next );
    TA(schedi)->schedq_len --;

    nexti = PIP_TASKI( next );
    DBGF( "PIPID:%d ==>> PIPID:%d", TA(schedi)->pipid, TA(nexti)->pipid );
    SETCURR( schedi, nexti );
    pip_couple_context( schedi, nexti );
  }
  NEVER_REACH_HERE;
}

static pip_task_internal_t *pip_schedq_next( pip_task_internal_t *schedi ) {
  pip_task_internal_t 	*nexti;
  pip_task_t		*next;

  ASSERTD( schedi != NULL );
  DBGF( "sched-PIPID:%d", TA(schedi)->pipid );
  if( pip_takein_ood_task( schedi ) ) {
    next = PIP_TASKQ_NEXT( &TA(schedi)->schedq );
    PIP_TASKQ_DEQ( next );
    TA(schedi)->schedq_len --;
    ASSERTD( TA(schedi)->schedq_len >= 0 );
    nexti = PIP_TASKI( next );
    DBGF( "next-PIPID:%d", TA(nexti)->pipid );
  } else {
    nexti = NULL;
    DBGF( "next:NULL" );
  }
  return nexti;
}

static void pip_enqueue_task( pip_task_internal_t *taski,
			      pip_task_queue_t *queue,
			      int flag_lock,
			      pip_enqueue_callback_t callback,
			      void *cbarg ) {
  pip_task_t	*task  = PIP_TASKQ( taski );

  if( flag_lock ) {
    pip_task_queue_lock( queue );
    pip_task_queue_enqueue( queue, task );
    pip_task_queue_unlock( queue );
  } else {
    pip_task_queue_enqueue( queue, task );
  }
  if( callback != NULL ) {
    if( callback == PIP_CB_UNLOCK_AFTER_ENQUEUE ) {
      pip_task_queue_unlock( queue );
    } else {
      callback( cbarg );
    }
  }
}

struct deffered_enqueue {
  pip_task_internal_t	*taski;
  pip_task_queue_t 	*queue;
  int 			flag_lock;
  pip_enqueue_callback_t callback;
  void 			*cbarg;
};

static void pip_deffered_enqueue( void *def_arg ) {
  struct deffered_enqueue *def_enq =
    (struct deffered_enqueue*) def_arg;
  pip_enqueue_task( def_enq->taski,
		    def_enq->queue,
		    def_enq->flag_lock,
		    def_enq->callback,
		    def_enq->cbarg );
}

void pip_suspend_and_enqueue_generic( pip_task_internal_t *taski,
				      pip_task_queue_t *queue,
				      int flag_lock,
				      pip_enqueue_callback_t callback,
				      void *cbarg ) {
  pip_task_internal_t	*schedi = TA(taski)->task_sched;
  pip_task_internal_t	*nexti  = pip_schedq_next( schedi );
  struct deffered_enqueue def_enqueue;

  ASSERTD( schedi != NULL );
  PIP_SUSPEND( taski );
  pip_atomic_fetch_and_add( &TA(schedi)->refcount, 1 );
  DBGF( "Sched %d  RFC:%d", TA(schedi)->pipid, (int)TA(schedi)->refcount );
  def_enqueue.taski     = taski;
  def_enqueue.queue     = queue;
  def_enqueue.flag_lock = flag_lock;
  def_enqueue.callback  = callback;
  def_enqueue.cbarg     = cbarg;
  if( nexti != NULL ) {
    TA(nexti)->deffered_proc = pip_deffered_enqueue;
    TA(nexti)->deffered_arg  = &def_enqueue;
    DBGF( "PIPID:%d -->> PIPID:%d", TA(taski)->pipid, TA(nexti)->pipid );
    SETCURR( schedi, nexti );
    pip_swap_context( taski, nexti );

  } else {
    TA(schedi)->deffered_proc = pip_deffered_enqueue;
    TA(schedi)->deffered_arg  = &def_enqueue;
    pip_decouple_context( taski, schedi );
  }
  /* taski is resumed eventually */
  RETURNV;
}

static void pip_deffered_wakeup( void *def_arg ) {
  pip_task_internal_t	*taski =
    (pip_task_internal_t*) def_arg;
  DBGF( "deffered-wakeup PIPID:%d", TA(taski)->pipid );
  pip_wakeup( taski );
}

void pip_do_exit( pip_task_internal_t *taski ) {
  pip_task_internal_t	*schedi, *nexti;
  pip_task_t		*queue, *next;
  /* this is called when 1) return from main (or func) function */
  /*                     2) pip_exit() is explicitly called     */
  ENTERF( "PIPID:%d", TA(taski)->pipid );

 try_again:
  ASSERT( ( schedi = TA(taski)->task_sched ) != NULL );
  queue = &TA(schedi)->schedq;
  if( pip_takein_ood_task( schedi ) ) {
    /* taski cannot terminate itself        */
    /* since it has other tasks to schedule */
    next = PIP_TASKQ_NEXT( queue );
    PIP_TASKQ_DEQ( next );
    nexti = PIP_TASKI( next );
    DBGF( "PIPID:%d ==>> PIPID:%d", TA(taski)->pipid, TA(nexti)->pipid );
    if( taski != schedi ) {
      /* if taski is NOT a scheduling task,  */
      /* then wakeup taski to terminate and  */
      ASSERT( --TA(schedi)->schedq_len >= 0 );
      TA(nexti)->deffered_proc = pip_deffered_wakeup;
      TA(nexti)->deffered_arg  = (void*) taski;
    } else {
      /* if this is a scheduling task, then */
      /* repeat this until the queue becomes emoty */
      /* and RFC becomes zero */
      PIP_TASKQ_ENQ_LAST( queue, PIP_TASKQ(taski) );
    }
    /* schedi schedules the rests */
    SETCURR( schedi, nexti );
    pip_swap_context( taski, nexti );
    DBGF( "TRY-AGAIN (1)" );

  } else {			/* queue is empty */
    DBGF( "task PIPID:%d   sched PIPID:%d", TA(taski)->pipid, TA(schedi)->pipid );
    if( taski == schedi ) {
      if( pip_able_to_terminate_now( taski ) ) {
	pip_terminate_task( taski );
	NEVER_REACH_HERE;
      }
      DBGF( "TRY-AGAIN (2)" );
    } else {
      /* wakeup taski to terminate */
      TA(schedi)->deffered_proc = pip_deffered_wakeup;
      TA(schedi)->deffered_arg  = (void*) taski;
      DBGF( "TRY-AGAIN (3)" );
    }
    pip_decouple_context( taski, schedi );
  }
  goto try_again;
  NEVER_REACH_HERE;
}

static int pip_do_resume( pip_task_internal_t *resume,
			  pip_task_internal_t *schedi ) {
  pip_task_internal_t	*taski = pip_task;
  pip_task_internal_t	*sched_curr = TA(resume)->task_sched;
  pip_task_internal_t	*sched_new;

  IF_UNLIKELY( taski == NULL            ) RETURN( EPERM );
  IF_UNLIKELY( PIP_IS_RUNNING( resume ) ) RETURN( EPERM );
  if( taski == resume ) RETURN( 0 );

  if( schedi == NULL ) {
    /* resume in the previous scheduling domain */
    ENTERF( "resume(PIPID:%d)  resume->schedi(PIPID:%d)",
	    TA(resume)->pipid, TA(TA(resume)->task_sched)->pipid );
    sched_new = TA(resume)->task_sched;
    ASSERTD( sched_new != NULL );
  } else {
    ENTERF( "resume(PIPID:%d)  schedi(PIPID:%d)",
	    TA(resume)->pipid, TA(schedi)->pipid );
    if( AA(schedi)->flag_exit ) RETURN( EBUSY );
    sched_new = schedi;
  }
  PIP_RUN( resume );

  if( sched_new == TA(taski)->task_sched ) {
    /* same scheduling domain with the current task */
    DBGF( "RUN(self): %d/%d", TA(resume)->pipid, TA(sched_curr)->pipid );
    TA(sched_new)->schedq_len ++;
    PIP_TASKQ_ENQ_LAST( &TA(sched_new)->schedq, PIP_TASKQ(resume) );
    TA(resume)->task_sched = sched_new;
  } else {
    /* schedule the task as an OOD (Out Of (scheduling) Domain) task */
    DBGF( "RUN(OOD): %d/%d", TA(resume)->pipid, TA(sched_new)->pipid );
    if( sched_new != taski ) {
      pip_sched_ood_task( sched_new, resume, sched_new );
    } else {
      pip_sched_ood_task( sched_new, resume, NULL );
    }
  }

  ASSERT( pip_atomic_sub_and_fetch( &TA(sched_curr)->refcount, 1 ) >= 0 );
  DBGF( "Sched %d  RFC:%d", TA(sched_curr)->pipid, (int)TA(sched_curr)->refcount );
  if( pip_able_to_terminate_now( sched_curr ) ) {
    pip_wakeup( sched_curr );
  }
  RETURN( 0 );
}

/* API */

void pip_task_queue_brief( pip_task_t *task, char *msg, size_t len ) {
  pip_task_internal_t	*taski = PIP_TASKI(task);
  snprintf( msg, len, "%d/%d(%c)",
	    TA(taski)->pipid, TA(TA(taski)->task_sched)->pipid, TA(taski)->state );
}

int pip_get_task_pipid( pip_task_t *task, int *pipidp ) {
  pip_task_internal_t	*taski = pip_task;
  IF_UNLIKELY( taski  == NULL ) RETURN( EPERM  );
  IF_UNLIKELY( task   == NULL ) RETURN( EINVAL );
  IF_LIKELY(   pipidp != NULL ) *pipidp = TA(PIP_TASKI(task))->pipid;
  RETURN( 0 );
}

int pip_get_task_by_pipid( int pipid, pip_task_t **taskp ) {
  int	err;

  IF_LIKELY( ( err = pip_check_pipid( &pipid ) ) == 0 ) {
    if( taskp != NULL ) {
      if( pipid == PIP_PIPID_ROOT ) {
	*taskp = PIP_TASKQ( pip_root->task_root );
      } else {
	*taskp = PIP_TASKQ( &pip_root->tasks[pipid] );
      }
    }
  }
  RETURN( err );
}

int pip_get_sched_domain( pip_task_t **domainp ) {
  pip_task_internal_t	*taski = pip_task;
  IF_UNLIKELY( taski   == NULL ) RETURN( EPERM  );
  IF_UNLIKELY( domainp != NULL ) {
    *domainp = PIP_TASKQ( TA(taski)->task_sched );
  }
  RETURN( 0 );
}

int pip_count_runnable_tasks( int *countp ) {
  pip_task_internal_t	*taski = pip_task;
  IF_UNLIKELY( taski  == NULL ) RETURN( EPERM  );
  IF_UNLIKELY( countp == NULL ) RETURN( EINVAL );
  *countp = TA(taski)->schedq_len;
  RETURN( 0 );
}

int pip_enqueue_runnable_N( pip_task_queue_t *queue, int *np ) {
  /* runnable tasks in the schedq of the current */
  /* task are enqueued into the queue            */
  pip_task_internal_t	*taski = pip_task;
  pip_task_internal_t 	*schedi;
  pip_task_t	 	*task, *next, *schedq;
  int			c, n;

  ENTER;
  IF_UNLIKELY( taski == NULL ) RETURN( EPERM  );
  IF_UNLIKELY( np    == NULL ) RETURN( EINVAL );

  schedi = TA(taski)->task_sched;
  ASSERTD( schedi != NULL );
  schedq = &TA(schedi)->schedq;
  n = *np;
  c = 0;
  if( n == PIP_TASK_ALL ) {
    pip_task_queue_lock( queue );
    PIP_TASKQ_FOREACH_SAFE( schedq, task, next ) {
      PIP_TASKQ_DEQ( task );
      PIP_SUSPEND( PIP_TASKI(task) );
      pip_task_queue_enqueue( queue, task );
      c ++;
    }
    pip_task_queue_unlock( queue );
  } else if( n > 0 ) {
    pip_task_queue_lock( queue );
    PIP_TASKQ_FOREACH_SAFE( schedq, task, next ) {
      PIP_TASKQ_DEQ( task );
      PIP_SUSPEND( PIP_TASKI(task) );
      pip_task_queue_enqueue( queue, task );
      if( ++c == n ) break;
    }
    pip_task_queue_unlock( queue );
  } else if( n < 0 ) {
    RETURN( EINVAL );
  }
  TA(schedi)->schedq_len -= c;
  ASSERTD( TA(schedi)->schedq_len >= 0 );
  *np = c;
  return 0;
}

int pip_suspend_and_enqueue_( pip_task_queue_t *queue,
			      pip_enqueue_callback_t callback,
			      void *cbarg ) {
  pip_task_internal_t	*taski  = pip_task;

  ENTER;
  IF_UNLIKELY( taski == NULL ) RETURN( EPERM );
  IF_UNLIKELY( queue == NULL ) RETURN( EINVAL );
  IF_UNLIKELY( PIP_IS_SUSPENDED( taski ) ) RETURN( EPERM );
  pip_suspend_and_enqueue_generic( taski, queue, 1, callback, cbarg );
  RETURN( 0 );
}

int pip_suspend_and_enqueue_nolock_( pip_task_queue_t *queue,
				     pip_enqueue_callback_t callback,
				     void *cbarg ) {
  pip_task_internal_t	*taski  = pip_task;

  ENTER;
  IF_UNLIKELY( taski == NULL ) RETURN( EPERM );
  IF_UNLIKELY( queue == NULL ) RETURN( EINVAL );
  IF_UNLIKELY( PIP_IS_SUSPENDED( taski ) ) RETURN( EPERM );
  pip_suspend_and_enqueue_generic( taski, queue, 0, callback, cbarg );
  RETURN( 0 );
}

int pip_resume( pip_task_t *resume, pip_task_t *sched ) {
  return pip_do_resume( PIP_TASKI(resume), PIP_TASKI(sched) );
}

int pip_dequeue_and_resume_( pip_task_queue_t *queue, pip_task_t *sched ) {
  pip_task_t		 *resume;

  ENTER;
  IF_UNLIKELY( pip_task == NULL ) RETURN( EPERM );
  pip_task_queue_lock( queue );
  resume = pip_task_queue_dequeue( queue );
  pip_task_queue_unlock( queue );
  IF_UNLIKELY( resume == NULL ) RETURN( ENOENT );
  RETURN( pip_do_resume( PIP_TASKI(resume), PIP_TASKI(sched) ) );
}

int pip_dequeue_and_resume_nolock_( pip_task_queue_t *queue,
				    pip_task_t *sched ) {
  pip_task_t *resume;

  ENTER;
  IF_UNLIKELY( pip_task == NULL ) RETURN( EPERM );
  resume = pip_task_queue_dequeue( queue );
  if( resume == NULL ) RETURN( ENOENT );
  RETURN( pip_do_resume( PIP_TASKI(resume), PIP_TASKI(sched) ) );
}

int pip_dequeue_and_resume_multiple( pip_task_queue_t *queue,
				     pip_task_internal_t *sched,
				     int *np ) {
  pip_task_internal_t	*taski = pip_task;
  pip_task_t	 	tmpq, *resume, *next;
  int			n, c = 0, err = 0;

  ENTER;
  IF_UNLIKELY( taski == NULL ) RETURN( EPERM  );
  IF_UNLIKELY( queue == NULL ) RETURN( EINVAL );
  IF_UNLIKELY( np    == NULL ) RETURN( EINVAL );
  n = *np;

  PIP_TASKQ_INIT( &tmpq );
  if( n == PIP_TASK_ALL ) {
    DBGF( "dequeue_and_resume_N: ALL" );
    pip_task_queue_lock( queue );
    while( ( resume = pip_task_queue_dequeue( queue ) ) != NULL ) {
      PIP_TASKQ_ENQ_LAST( &tmpq, resume );
      c ++;
    }
    pip_task_queue_unlock( queue );

  } else if( n > 0 ) {
    DBGF( "N: %d", n );
    pip_task_queue_lock( queue );
    while( c < n ) {
      resume = pip_task_queue_dequeue( queue );
      if( resume == NULL ) break;
      PIP_TASKQ_ENQ_LAST( &tmpq, resume );
      c ++;
    }
    pip_task_queue_unlock( queue );

  } else if( n < 0 ) {
    RETURN( EINVAL );
  }
  PIP_TASKQ_FOREACH_SAFE( &tmpq, resume, next ) {
    PIP_TASKQ_DEQ( resume );
    err = pip_do_resume( PIP_TASKI(resume), sched );
    if( err ) break;
  }
  if( !err ) *np = c;
  RETURN( err );
}

int pip_dequeue_and_resume_N_( pip_task_queue_t *queue,
			       pip_task_t *sched,
			       int *np ) {
  return pip_dequeue_and_resume_multiple( queue,
					  PIP_TASKI(sched),
					  np );
}

int pip_dequeue_and_resume_N_nolock_( pip_task_queue_t *queue,
				      pip_task_t *sched,
				      int *np ) {
  pip_task_internal_t	*taski = pip_task;
  pip_task_t 		tmpq, *resume, *next;
  int			n, c = 0, err = 0;

  ENTER;
  IF_UNLIKELY( taski == NULL ) RETURN( EPERM  );
  IF_UNLIKELY( queue == NULL ) RETURN( EINVAL );
  IF_UNLIKELY( np    == NULL ) RETURN( EINVAL );
  n = *np;

  PIP_TASKQ_INIT( &tmpq );
  if( n == PIP_TASK_ALL ) {
    DBGF( "dequeue_and_resume_N: ALL" );
    while( ( resume = pip_task_queue_dequeue( queue ) ) != NULL ) {
      PIP_TASKQ_ENQ_LAST( &tmpq, resume );
      c ++;
    }
  } else if( n > 0 ) {
    DBGF( "N: %d", n );
    while( c < n ) {
      resume = pip_task_queue_dequeue( queue );
      if( resume == NULL ) break;
      PIP_TASKQ_ENQ_LAST( &tmpq, resume );
      c ++;
    }
  } else if( n < 0 ) {
    RETURN( EINVAL );
  }
  PIP_TASKQ_FOREACH_SAFE( &tmpq, resume, next ) {
    PIP_TASKQ_DEQ( resume );
    err = pip_do_resume( PIP_TASKI(resume), PIP_TASKI(sched) );
    if( err ) break;
  }
  if( !err ) *np = c;
  RETURN( err );
}

pip_task_t *pip_task_self( void ) { return PIP_TASKQ(pip_task); }

int pip_yield( int flag ) {
  pip_task_t		*queue, *next;
  pip_task_internal_t	*taski = pip_task;
  pip_task_internal_t	*nexti, *schedi = TA(taski)->task_sched;
  int			err = 0;

  ENTER;
  IF_UNLIKELY( taski == NULL ) RETURN( EPERM );

  if( flag == 0 ||
      ( flag & PIP_YIELD_SYSTEM ) == PIP_YIELD_SYSTEM ) {
    pip_system_yield();
  }
  if( flag == 0 ||
      ( flag & PIP_YIELD_USER ) == PIP_YIELD_USER ) {
    IF_UNLIKELY( TA(schedi)->oodq_len > 0 ) {	/* fast check */
      (void) pip_takein_ood_task( schedi );
    }
    queue = &TA(schedi)->schedq;
    IF_UNLIKELY( PIP_TASKQ_ISEMPTY( queue ) ) RETURN( 0 );
    err = EINTR;
    PIP_TASKQ_ENQ_LAST( queue, PIP_TASKQ(taski) );
    next = PIP_TASKQ_NEXT( queue );
    PIP_TASKQ_DEQ( next );
    nexti = PIP_TASKI( next );
    ASSERTD( taski != nexti );
    DBGF( "next-PIPID: %d", TA(nexti)->pipid );
    SETCURR( schedi, nexti );
    pip_swap_context( taski, nexti );
  }
  RETURN( err );
}

int pip_yield_to( pip_task_t *target ) {
  pip_task_internal_t	*targeti = PIP_TASKI( target );
  pip_task_internal_t	*schedi, *taski = pip_task;
  pip_task_t		*queue;

  IF_UNLIKELY( targeti == NULL  ) {
    RETURN( pip_yield( PIP_YIELD_DEFAULT ) );
  }
  IF_UNLIKELY( taski   == NULL  ) RETURN( EPERM );
  IF_UNLIKELY( targeti == taski ) RETURN( 0 );
  schedi = TA(taski)->task_sched;
  /* the target task must be in the same scheduling domain */
  IF_UNLIKELY( TA(targeti)->task_sched != schedi ) RETURN( EPERM );

  IF_UNLIKELY( TA(schedi)->oodq_len > 0 ) {	/* fast check */
    (void) pip_takein_ood_task( schedi );
  }
  queue = &TA(schedi)->schedq;
  PIP_TASKQ_DEQ( target );
  PIP_TASKQ_ENQ_LAST( queue, PIP_TASKQ(taski) );
  SETCURR( schedi, targeti );
  pip_swap_context( taski, targeti );
  RETURN( EINTR );
}

int pip_set_aux( void *aux ) {
  pip_task_internal_t 	*taski = pip_task;

  if( !pip_is_initialized() ) RETURN( EPERM );
  AA(taski)->aux = aux;
  RETURN( 0 );
}

int pip_get_aux( void **auxp ) {
  pip_task_internal_t 	*taski = pip_task;

  if( !pip_is_initialized() ) RETURN( EPERM );
  IF_LIKELY( auxp != NULL ) {
    *auxp = AA(taski)->aux;
  }
  RETURN( 0 );
}

struct deffered_ood_and_wakeup {
  pip_task_internal_t 	*ood_schedi;
  pip_task_internal_t 	*ood_taski;
  pip_task_internal_t 	*ood_wakeup;
  pip_task_internal_t 	*wakeup;
};

static void pip_deffered_ood_and_wakeup( void *def_arg ) {
  struct deffered_ood_and_wakeup *def_ood =
    (struct deffered_ood_and_wakeup*) def_arg;
  pip_task_internal_t	*ood_schedi = def_ood->ood_schedi;
  pip_task_internal_t	*ood_taski  = def_ood->ood_taski;
  pip_task_internal_t	*ood_wakeup = def_ood->ood_wakeup;
  pip_task_internal_t	*wakeup     = def_ood->wakeup;

  ENTER;
  if( ood_schedi != NULL ) {
    ASSERTD( ood_taski != NULL );
    DBGF( "deffered-OOD PIPID:%d/%d",
	  TA(ood_taski)->pipid, TA(ood_schedi)->pipid );
    pip_sched_ood_task( ood_schedi, ood_taski, ood_wakeup );
  }
  if( wakeup ) {
    DBGF( "deffered-wakeup PIPID:%d", TA(wakeup)->pipid );
    pip_wakeup( wakeup );
  }
}

int pip_couple( void ) {
  struct deffered_ood_and_wakeup def_ood;
  pip_task_internal_t 	*taski = pip_task;
  pip_task_internal_t 	*schedi = TA(taski)->task_sched;
  pip_task_internal_t 	*nexti;
  int			err = 0;

  ENTER;
  IF_UNLIKELY( taski                    == NULL  ) RETURN( EPERM );
  IF_UNLIKELY( schedi                   == taski ) RETURN( EBUSY );
  IF_UNLIKELY( AA(taski)->coupled_sched != NULL  ) RETURN( EBUSY );

  AA(taski)->coupled_sched = schedi;
  def_ood.ood_schedi = taski;
  def_ood.ood_taski  = taski;
  def_ood.ood_wakeup = NULL;
  def_ood.wakeup     = taski;

  if( ( nexti = pip_schedq_next( schedi ) ) != NULL ) {
    SETCURR( schedi, nexti );
    TA(nexti)->deffered_proc = pip_deffered_ood_and_wakeup;
    TA(nexti)->deffered_arg  = &def_ood;
    pip_swap_context( taski, nexti );
  } else {
    TA(schedi)->deffered_proc = pip_deffered_ood_and_wakeup;
    TA(schedi)->deffered_arg  = &def_ood;
    pip_decouple_context( taski, schedi );
  }
  RETURN( err );
}

int pip_decouple( pip_task_t *sched ) {
  struct deffered_ood_and_wakeup def_ood;
  pip_task_internal_t 	*taski   = pip_task;
  pip_task_internal_t  	*schedi  = PIP_TASKI( sched );
  pip_task_internal_t  	*coupled = AA(taski)->coupled_sched;
  int			err = 0;

  ENTER;
  IF_UNLIKELY( taski                  == NULL  ) RETURN( EPERM );
  IF_UNLIKELY( TA(taski)->task_sched != taski ) RETURN( EPERM );
  if( schedi == NULL ) schedi = coupled;
  IF_UNLIKELY( schedi                 == NULL  ) RETURN( EPERM );
  IF_UNLIKELY( schedi                 == taski ) RETURN( EPERM );
  IF_UNLIKELY( AA(schedi)->flag_exit           ) RETURN( EBUSY );

  AA(taski)->coupled_sched = NULL;
  def_ood.ood_schedi = schedi;
  def_ood.ood_taski  = taski;
  def_ood.ood_wakeup = schedi;
  def_ood.wakeup     = NULL;
  TA(taski)->deffered_proc = pip_deffered_ood_and_wakeup;
  TA(taski)->deffered_arg  = &def_ood;

  pip_decouple_context( taski, taski );
  RETURN( err );
}

int pip_set_syncflag( uint32_t flags ) {
  int err = 0;
  if( pip_check_sync_flag( &flags ) != 0 ) {
    err = EINVAL;
  } else {
    AA(pip_task)->opts =
      ( AA(pip_task)->opts & ~PIP_SYNC_MASK ) | flags;
  }
  return err;
}
