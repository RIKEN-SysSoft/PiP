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
#include <pip_internal.h>
#include <pip_blt.h>

#ifdef DEBUG
#define QUEUE_DUMP( queue, len )			\
  if( PIP_TASKQ_ISEMPTY( queue ) ) {			\
    DBGF( "EMPTY" );					\
  } else {						\
    pip_task_t *task;					\
    char msg[128];					\
    int iii = 0;					\
    PIP_TASKQ_FOREACH( queue, task ) {			\
      pip_task_queue_brief( task, msg, sizeof(msg) );	\
      DBGF( "Q(%d:%d): %s", iii++, len, msg );		\
    }							\
  }

#define TASK_QUEUE_DUMP( queue )			\
  if( pip_task_queue_isempty( queue ) ) {		\
    DBGF( "EMPTY" );					\
  } else {						\
    pip_task_t *task;					\
    char msg[128];					\
    int iii = 0, c;					\
    pip_task_queue_count( queue, &c );			\
    PIP_TASKQ_FOREACH( &queue->queue, task ) {		\
      pip_task_queue_brief( task, msg, sizeof(msg) );	\
      DBGF( "TQ(%d:%d): %s", iii++, c, msg );		\
    }							\
  }
#else
#define QUEUE_DUMP( queue, len )
#define TASK_QUEUE_DUMP( queue )
#endif

static void pip_wakeup( pip_task_internal_t *taski ) {
  pip_memory_barrier();
  taski->flag_wakeup = 1;
  if( taski->flag_semwait ) {
    DBGF( "PIPID:%d -- WAKEUP vvvvvvvvvv\n"
	  "    schedq_len:%d  oodq_len:%d  ntakecare:%d"
	  "  flag_semwait:%d  flag_exit:%d",
	  taski->pipid, taski->schedq_len,
	  (int) taski->oodq_len, (int) taski->ntakecare,
	  taski->flag_semwait, taski->flag_exit );
    taski->flag_semwait = 0;
    /* not to call sem_post() more than once per blocking */
    errno = 0;
    sem_post( &taski->annex->sleep );
    ASSERT( errno );
  }
}

static void pip_change_sched( pip_task_internal_t *taski,
			      pip_task_internal_t *schedi ) {
  ASSERT( schedi == NULL );
  if( taski->task_sched != schedi ) {
    DBGF( "taski:%d[%d]-NTC:%d  schedi:%d[%d]-NTC:%d",
	  taski->pipid,
	  taski->task_sched->pipid,
	  (int) taski->task_sched->ntakecare,
	  schedi->pipid,
	  schedi->task_sched->pipid,
	  (int) schedi->ntakecare );
    int ntc = pip_atomic_sub_and_fetch( &taski->task_sched->ntakecare, 1 );
    ASSERT( ntc < 0 );
    taski->task_sched = schedi;
    (void) pip_atomic_fetch_and_add( &schedi->ntakecare, 1 );
    if( ntc == 0 ) pip_wakeup( taski->task_sched );
  }
}

static void pip_sched_ood_task( pip_task_internal_t *schedi,
				pip_task_internal_t *taski ) {
  pip_task_annex_t	*annex = schedi->annex;

  DBGF( "sched:PIPID:%d  taski:PIPID:%d", schedi->pipid, taski->pipid );
  ASSERT( schedi == NULL );
  ASSERT( schedi->task_sched != schedi );

  pip_change_sched( taski, schedi );

  pip_spin_lock( &annex->lock_oodq );
  {
    PIP_TASKQ_ENQ_LAST( &annex->oodq, PIP_TASKQ( taski ) );
    schedi->oodq_len ++;
  }
  pip_spin_unlock( &annex->lock_oodq );

  pip_wakeup( schedi );
}

static void pip_takein_ood_task( pip_task_internal_t *schedi ) {
  pip_task_annex_t	*annex;
  int		 	oodq_len = schedi->oodq_len;

  ENTER;
  annex = schedi->annex;
  pip_spin_lock( &annex->lock_oodq );
  {
    oodq_len = schedi->oodq_len;
    QUEUE_DUMP( &annex->oodq, oodq_len );
    PIP_TASKQ_APPEND( &schedi->schedq, &annex->oodq );
    schedi->oodq_len    = 0;
    schedi->schedq_len += oodq_len;
    QUEUE_DUMP( &schedi->schedq, schedi->schedq_len );
  }
  pip_spin_unlock( &annex->lock_oodq );
}

static void pip_system_yield( void ) {
  extern int pip_is_threaded_( void );
  if( pip_is_threaded_() ) {
    (void) pthread_yield();
  } else {
    sched_yield();
  }
}

static void pip_stack_protect( pip_task_internal_t *taski,
			       pip_task_internal_t *nexti ) {
  /* so that new task can reset the flag */
  DBGF( "protect PIPID:%d released by PIPID:%d", taski->pipid, nexti->pipid );
  taski->flag_stackp = 1;
  pip_memory_barrier();
  nexti->flag_stackpp = &taski->flag_stackp;
}

static void pip_stack_unprotect( pip_task_internal_t *taski ) {
  /* this function musrt be called everytime a context is switched */
  if( taski != NULL && taski->flag_stackpp != NULL ) {
    DBGF( "UNprotect PIPID:%d", taski->pipid );
    *taski->flag_stackpp = 0;
    pip_memory_barrier();
    taski->flag_stackpp  = NULL;
  }
}

static void pip_stack_wait( pip_task_internal_t *taski ) {
  if( taski->flag_stackp ) {
    int i, j;

    DBGF( "PIPID:%d wait for unprotect", taski->pipid );
    i = 0;
    while( 1 ) {
      for( j=0; j<PIP_BUSYWAIT_COUNT; j++ ) {
	if( !taski->flag_stackp ) goto done;
      }
      i ++;
      pip_system_yield();
      DBGF( "WAITING  pipid:%d (count=%d*%d) ...",
	    taski->pipid, i, PIP_BUSYWAIT_COUNT );
    }
  done:
    DBGF( "WAIT-DONE  pipid:%d (count=%d*%d)",
	  taski->pipid, i, PIP_BUSYWAIT_COUNT );
  }
  return;
}

static void pip_task_sched( pip_task_internal_t *taski ) {
  void *ctxp = taski->ctx_suspend;
  ASSERT( ctxp == NULL );
  pip_stack_wait( taski );
  taski->ctx_suspend = NULL;
  DBGF( "CTXP:%p", ctxp );
#ifdef PIP_LOAD_TLS
  ASSERT( pip_load_tls( taski->tls ) );
#endif
  ASSERT( pip_load_context( ctxp ) );
  NEVER_REACH_HERE;
}

typedef struct pip_sleep_args {
  pip_task_internal_t	*taski;
} pip_sleep_args_t;

static void pip_sleep( intptr_t task_H, intptr_t task_L ) {
  void pip_block_myself( pip_task_internal_t *taski ) {
    ENTER;
    DBGF( "flag_wakeup:%d  flag_exit:%d",
	  taski->flag_wakeup, taski->flag_exit );
    pip_memory_barrier();
    taski->flag_semwait = 1;
    while( !taski->flag_wakeup && !taski->flag_exit ) {
      errno = 0;
      if( sem_wait( &taski->annex->sleep ) == 0 ) continue;
      ASSERT( errno != EINTR );
    }
    pip_memory_barrier();
    taski->flag_semwait = 0;
    RETURNV;
  }
  extern void pip_deadlock_inc_( void );
  extern void pip_deadlock_dec_( void );

  pip_sleep_args_t 	*args = (pip_sleep_args_t*)
    ( ( ((intptr_t) task_H) << 32 ) | ( ((intptr_t) task_L) & PIP_MASK32 ) );
  pip_task_internal_t	*taski = args->taski;
  int			i;

  ENTER;
  DBGF( "PIPID:%d", taski->pipid );
  ASSERT( taski->task_sched == NULL );
  pip_stack_unprotect( taski );

  while( 1 ) {
    /* now stack is switched and unprotect the stack */
    pip_takein_ood_task( taski );
    if( !PIP_TASKQ_ISEMPTY( &taski->schedq ) ) break;

    if( taski->flag_exit && !taski->ntakecare ) {
      DBGF( "PIPID:%d -- WOKEUP to EXIT", taski->pipid );
      /* wait for the stack is free to use */
      pip_stack_wait( taski );
      /* then switch to the exit context */
#ifdef PIP_LOAD_TLS
      ASSERT( pip_load_tls( taski->tls ) );
#endif
      ASSERT( taski->annex->ctx_exit == NULL );
      ASSERT( pip_load_context( taski->annex->ctx_exit ) );
      NEVER_REACH_HERE;
    }

    pip_deadlock_inc_();
    {
      switch( taski->annex->opts & PIP_SYNC_MASK ) {
      case PIP_SYNC_BLOCKING:
	DBGF( "PIPID:%d -- SLEEPING (blocking) zzzzzzz", taski->pipid );
	pip_block_myself( taski );
	break;
      case PIP_SYNC_BUSYWAIT:
	DBGF( "PIPID:%d -- SLEEPING (busywait) zzzzzzzz", taski->pipid );
	while( !taski->flag_wakeup ) pip_pause();
	break;
      case PIP_SYNC_YIELD:
	DBGF( "PIPID:%d -- SLEEPING (yield) zzzzzzzz", taski->pipid );
	while( !taski->flag_wakeup ) pip_system_yield();
	break;
      case PIP_SYNC_AUTO:
      default:
	for( i=0; i<PIP_BUSYWAIT_COUNT; i++ ) {
	  pip_pause();
	  if( taski->flag_wakeup ) {
	    DBGF( "PIPID:%d -- SLEEPING (blocking-busywait:%d)", taski->pipid, i );
	    goto done;
	  }
	}
	DBGF( "PIPID:%d --SLEEPING (blocking) zzzzzzz", taski->pipid );
	pip_block_myself( taski );
      done:
	break;
      }
      DBGF( "PIPID:%d -- WOKEUP ^^^^^^^^", taski->pipid );
    }
    pip_deadlock_dec_();
    taski->flag_wakeup = 0;
  }
  pip_change_sched( taski, taski );

  ASSERT( taski->schedq_len == 0 );
  ASSERT( PIP_TASKQ_ISEMPTY( &taski->schedq ) );
  {
    pip_task_t *next = PIP_TASKQ_NEXT( &taski->schedq );
    pip_task_internal_t *nexti = PIP_TASKI( next );

    ASSERT( next == NULL );
    PIP_TASKQ_DEQ( next );
    taski->schedq_len --;
    DBGF( "PIPID:%d(ctxp:%p) -->> PIPID:%d(CTXP:%p)",
	  taski->pipid, taski->ctx_suspend,
	  nexti->pipid, nexti->ctx_suspend );
    pip_task_sched( nexti );
  }
  NEVER_REACH_HERE;
}

static void pip_block_task( pip_task_internal_t *taski ) {
  pip_sleep_args_t	args;
  pip_ctx_t		ctx;
  stack_t		*stk;
  int 			args_H, args_L;

  ENTER;
  DBGF( "PIPID:%d", taski->pipid );
  args.taski = taski;
  /* creating new context to switch stack */
  ASSERT( pip_save_context( &ctx ) );
  stk = &(ctx.ctx.uc_stack);
  stk->ss_sp    = taski->annex->sleep_stack;
  stk->ss_size  = pip_root_->stack_size_sleep;
  stk->ss_flags = 0;
  args_H = ( ((intptr_t) &args) >> 32 ) & PIP_MASK32;
  args_L = (  (intptr_t) &args)         & PIP_MASK32;
  pip_make_context( &ctx, pip_sleep, 4, args_H, args_L );
  /* no need of saving/restoring TLS */
  ASSERT( pip_load_context( &ctx ) );
  NEVER_REACH_HERE;
}

static void pip_task_switch( pip_task_internal_t *curr_task,
			     pip_task_internal_t *new_task ) {
  pip_ctx_t	ctx, *new_ctxp;

  ENTER;
  DBGF( "PIPID:%d (CTXP:%p) ==>> PIPID:%d (CTXP:%p)",
	curr_task->pipid, &ctx, new_task->pipid, new_task->ctx_suspend );
  ASSERT( curr_task->ctx_suspend != NULL );

  curr_task->ctx_suspend = &ctx;
  new_ctxp = new_task->ctx_suspend;
  new_task->ctx_suspend = NULL;
  ASSERT( pip_swap_context( &ctx, new_ctxp ) );
  /* resumed */
  pip_stack_unprotect( curr_task );
  RETURNV;
}

static pip_task_internal_t *pip_schedq_next( pip_task_internal_t *taski ) {
  pip_task_internal_t 	*schedi = taski->task_sched;
  pip_task_internal_t 	*nexti;
  pip_task_t		*next;

  ASSERT( schedi == NULL );
  DBGF( "sched-PIPID:%d", schedi->pipid );
  pip_takein_ood_task( schedi );
  if( !PIP_TASKQ_ISEMPTY( &schedi->schedq ) ) {
    QUEUE_DUMP( &schedi->schedq, schedi->schedq_len );
    next = PIP_TASKQ_NEXT( &schedi->schedq );
    PIP_TASKQ_DEQ( next );
    schedi->schedq_len --;
    ASSERT( schedi->schedq_len < 0 );
    nexti = PIP_TASKI( next );
    DBGF( "next-PIPID:%d", nexti->pipid );

  } else if( schedi->flag_exit ) {
    /* if the scheduler has no scheduling tasks, then switch bask to */
    /* the scheduler so that scheduling task can terminate itself    */
    nexti = schedi;

  } else {
    nexti = NULL;
    DBGF( "next:NULL" );
  }
  return nexti;
}

static void pip_sched_next( pip_task_internal_t *taski,
			    pip_queue_info_t *qip ) {
  void pip_enqueue_qi( pip_task_internal_t *taski,
		       pip_queue_info_t *qip ) {
    pip_task_t		*task = PIP_TASKQ( taski );
    pip_task_queue_t	*queue = qip->queue;

    if( qip->flag_lock ) {
      pip_task_queue_lock( queue );
      pip_task_queue_enqueue( queue, task );
      pip_task_queue_unlock( queue );
    } else {
      pip_task_queue_enqueue( queue, task );
    }
    if( qip->callback != NULL ) qip->callback( qip->cbarg );
  }

  pip_task_internal_t	*schedi = taski->task_sched;
  pip_task_internal_t	*nexti  = pip_schedq_next( taski );
  volatile int 	flag_jump = 0;	/* must be volatile */
  pip_ctx_t	ctx;

  ENTER;
  PIP_SUSPEND( taski );
  if( qip != NULL && qip->queue != NULL ) {
    if( nexti != NULL ) {
      pip_stack_protect( taski, nexti );
    } else {
      pip_stack_protect( taski, schedi );
    }
    /* before enqueue, the stack must be protected */
    pip_enqueue_qi( taski, qip );
  }
  DBGF( "CTXP:%p", &ctx );
  taski->ctx_suspend = &ctx;
  ASSERT( pip_save_context( &ctx ) );
  if( !flag_jump ) {
    flag_jump = 1;
    if( nexti != NULL ) {
      /* context-switch to the next task */
      DBGF( "PIPID:%d(ctxp:%p) -->> PIPID:%d(CTXP:%p)",
	    taski->pipid, taski->ctx_suspend,
	    nexti->pipid, nexti->ctx_suspend );
      pip_task_sched( nexti );

    } else {
      /* no task to be scheduled next and block the scheduling task */
      pip_block_task( schedi );
    }
    NEVER_REACH_HERE;
  }
  DBG;
  /* resumed */
  pip_stack_unprotect( taski );
}

void pip_do_exit( pip_task_internal_t *taski, int extval ) {
  extern void pip_set_extval_RC( pip_task_internal_t*, int );
  pip_task_internal_t	*schedi = taski->task_sched;
  pip_task_t		*queue, *next;
  pip_task_internal_t	*nexti;
  int 			ntc;
  /* this is called when 1) return from main (or func) function */
  /*                     2) pip_exit() is explicitly called     */
  ENTER;
  DBGF( "PIPID:%d", taski->pipid );
  ASSERT( schedi == NULL );

  pip_set_extval_RC( taski, extval );

  ntc = pip_atomic_sub_and_fetch( &schedi->ntakecare, 1 );
  ASSERT( ntc < 0 );

  DBGF( "TASKI:  %d[%d]", taski->pipid,  taski->task_sched->pipid  );
  DBGF( "SCHEDI: %d[%d]", schedi->pipid, schedi->task_sched->pipid );
  DBGF( "schedi->ntakecare:%d", (int) schedi->ntakecare );

  DBG;
  pip_takein_ood_task( schedi );
  queue = &schedi->schedq;
  if( !PIP_TASKQ_ISEMPTY( queue ) ) {
    next = PIP_TASKQ_NEXT( queue );
    PIP_TASKQ_DEQ( next );
    schedi->schedq_len --;
    ASSERT( schedi->schedq_len < 0 );
    nexti = PIP_TASKI( next );
    pip_stack_protect( taski, nexti );
    if( taski != schedi ) {
      /* wake up (if sleeping) to terminate */
      pip_wakeup( taski );
    }
    pip_task_switch( taski, nexti );
  } else if( taski != schedi ) {
    pip_stack_protect( taski, schedi );
    /* wake up (if sleeping) to terminate */
    pip_wakeup( taski );
  }

  DBG;
  /* there might be suspended tasks waiting to be resumed */
  pip_block_task( schedi );
  NEVER_REACH_HERE;
}

static int pip_do_resume( pip_task_internal_t *taski,
			  pip_task_internal_t *schedi ) {
  ENTER;
  DBGF( "taski:%d[%d](%c)  schedi:%d",
	taski->pipid, taski->task_sched->pipid, taski->state,
	( schedi != NULL )? schedi->pipid : -1 );

  if( PIP_IS_RUNNING( taski ) ) RETURN( EPERM );

  if( schedi == NULL ) {
    /* resume in the previous scheduling domain */
    schedi = taski->task_sched;
  } else {
    /* get the right scheduling task (domain) */
    schedi = schedi->task_sched;
  }
  ASSERT( schedi == NULL );
  ASSERT( taski->task_sched == NULL );
  DBGF( "Sched PIPID:%d (PID:%d) takes in PIPID:%d (PID:%d)",
	schedi->pipid, schedi->annex->tid, taski->pipid, taski->annex->tid );

  PIP_RUN( taski );
  DBGF( "RUN: %d[%d]", taski->pipid, taski->task_sched->pipid );
  if( schedi == pip_task_->task_sched ) {
    /* same scheduling domain with the current task */
    pip_change_sched( taski, schedi );
    PIP_TASKQ_ENQ_LAST( &schedi->schedq, PIP_TASKQ(taski) );
    schedi->schedq_len ++;
  } else {			/* otherwise */
    pip_sched_ood_task( schedi, taski );
  }
  RETURN( 0 );
}

/* API */

void pip_task_queue_brief( pip_task_t *task, char *msg, size_t len ) {
  pip_task_internal_t	*taski = PIP_TASKI(task);
  snprintf( msg, len, "%d[%d](%c)",
	    taski->pipid, taski->task_sched->pipid, taski->state );
}

int pip_get_task_pipid( pip_task_t *task, int *pipidp ) {
  if( task      == NULL ) RETURN( EINVAL );
  if( pip_task_ == NULL ) RETURN( EPERM  );
  if( pipidp != NULL ) *pipidp = PIP_TASKI(task)->pipid;
  RETURN( 0 );
}

int pip_get_sched_domain( pip_task_t **domainp ) {
  if( domainp    == NULL ) RETURN( EINVAL );
  if( pip_task_  == NULL ) RETURN( EPERM  );
  *domainp = PIP_TASKQ( pip_task_->task_sched );
  RETURN( 0 );
}

int pip_count_runnable_tasks( int *countp ) {
  if( countp == NULL ) RETURN( EINVAL );
  *countp = pip_task_->schedq_len;
  RETURN( 0 );
}

int pip_enqueue_runnable_N( pip_task_queue_t *queue, int *np ) {
  /* runnable tasks in the schdq of the pip_task are enqueued into the queue */
  pip_task_internal_t 	*schedi;
  pip_task_t	 	*task, *next, *schedq;
  int			c = 0, n;

  ENTER;
  if( np == NULL ) RETURN( EINVAL );
  schedi = pip_task_->task_sched;
  ASSERT( schedi == NULL );
  schedq = &schedi->schedq;
  n = *np;
  if( n == PIP_TASK_ALL ) {
    pip_task_queue_lock( queue );
    PIP_TASKQ_FOREACH_SAFE( schedq, task, next ) {
      PIP_TASKQ_DEQ( task );
      pip_task_queue_enqueue( queue, task );
      c ++;
    }
    pip_task_queue_unlock( queue );
  } else if( n > 0 ) {
    pip_task_queue_lock( queue );
    PIP_TASKQ_FOREACH_SAFE( schedq, task, next ) {
      PIP_TASKQ_DEQ( task );
      pip_task_queue_enqueue( queue, task );
      if( ++c == n ) break;
    }
    pip_task_queue_unlock( queue );
  } else if( n < 0 ) {
    RETURN( EINVAL );
  }
  schedi->schedq_len -= c;
  ASSERT( schedi->schedq_len < 0 );
  *np = c;
  return 0;
}

static void pip_cb_unlock_after_enqueue( void *q ) {
  pip_task_queue_t *queue = (pip_task_queue_t*) q;
  pip_task_queue_unlock( queue );
}

int pip_suspend_and_enqueue_generic_( pip_task_internal_t *taski,
				      pip_task_queue_t *queue,
				      int flag_lock,
				      pip_enqueue_callback_t callback,
				      void *cbarg ) {
  pip_queue_info_t qi;

  ENTER;
  ASSERT( taski->task_sched == NULL );
  qi.queue     = queue;
  qi.flag_lock = flag_lock;
  if( callback == PIP_CB_UNLOCK_AFTER_ENQUEUE ) {
    qi.callback  = pip_cb_unlock_after_enqueue;
    qi.cbarg     = (void*) queue;
  } else {
    qi.callback  = callback;
    qi.cbarg     = cbarg;
  }
  /* pip_scheq_next() protected current stack. and now we can enqueue myself */
  pip_sched_next( taski, &qi );
  RETURN( 0 );
}

int pip_suspend_and_enqueue_( pip_task_queue_t *queue,
			     pip_enqueue_callback_t callback,
			     void *cbarg ) {
  if( queue == NULL && callback != NULL ) RETURN( EINVAL );
  RETURN( pip_suspend_and_enqueue_generic_( pip_task_, queue, 1,
					    callback, cbarg ) );
}

int pip_suspend_and_enqueue_nolock_( pip_task_queue_t *queue,
				     pip_enqueue_callback_t callback,
				     void *cbarg ) {
  if( queue == NULL && callback != NULL ) RETURN( EINVAL );
  RETURN( pip_suspend_and_enqueue_generic_( pip_task_, queue, 0,
					    callback, cbarg ) );
}

int pip_resume( pip_task_t *task, pip_task_t *sched ) {
  return pip_do_resume( PIP_TASKI(task), PIP_TASKI(sched) );
}

int pip_dequeue_and_resume_( pip_task_queue_t *queue, pip_task_t *sched ) {
  ENTER;
  pip_task_t *task;
  pip_task_queue_lock( queue );
  task = pip_task_queue_dequeue( queue );
  pip_task_queue_unlock( queue );
  if( task == NULL ) RETURN( ENOENT );
  RETURN( pip_do_resume( PIP_TASKI(task), PIP_TASKI(sched) ) );
}

int pip_dequeue_and_resume_nolock_( pip_task_queue_t *queue,
				    pip_task_t *sched ) {
  ENTER;
  pip_task_t *task;
  task = pip_task_queue_dequeue( queue );
  if( task == NULL ) RETURN( ENOENT );
  RETURN( pip_do_resume( PIP_TASKI(task), PIP_TASKI(sched) ) );
}

int pip_dequeue_and_resume_N_( pip_task_queue_t *queue,
			       pip_task_t *sched,
			       int *np ) {
  pip_task_t 	*task;
  int		n, c = 0, err = 0;

  ENTER;
  if( np == NULL ) RETURN( EINVAL );
  n = *np;
  if( n == PIP_TASK_ALL ) {
    DBGF( "dequeue_and_resume_N: ALL" );
    pip_task_queue_lock( queue );
    TASK_QUEUE_DUMP( queue );
    while( !pip_task_queue_isempty( queue ) ) {
      task = pip_task_queue_dequeue( queue );
      err = pip_do_resume( PIP_TASKI(task), PIP_TASKI(sched) );
      if( err ) break;
      c ++;
    }
    TASK_QUEUE_DUMP( queue );
    pip_task_queue_unlock( queue );

  } else if( n > 0 ) {
    DBGF( "dequeue_and_resume_N: %d", n );
    pip_task_queue_lock( queue );
    TASK_QUEUE_DUMP( queue );
    while( !pip_task_queue_isempty( queue ) && c < n ) {
      task = pip_task_queue_dequeue( queue );
      err = pip_do_resume( PIP_TASKI(task), PIP_TASKI(sched) );
      if( err ) break;
      c ++;
    }
    TASK_QUEUE_DUMP( queue );
    pip_task_queue_unlock( queue );

  } else if( n < 0 ) {
    err = EINVAL;
  }
  *np = c;
  RETURN( err );
}

int pip_dequeue_and_resume_N_nolock_( pip_task_queue_t *queue,
				      pip_task_t *sched,
				      int *np ) {
  pip_task_t 	*task;
  int		n, c = 0, err = 0;

  ENTER;
  if( np == NULL ) RETURN( EINVAL );
  n = *np;
  if( n == PIP_TASK_ALL ) {
    while( !pip_task_queue_isempty( queue ) ) {
      task = pip_task_queue_dequeue( queue );
      if( ( err = pip_resume( task, sched ) ) != 0 ) break;
      c ++;
    }
  } else if( n > 0 ) {
    while( !pip_task_queue_isempty( queue ) && c < n ) {
      task = pip_task_queue_dequeue( queue );
      if( ( err = pip_resume( task, sched ) ) != 0 ) break;
      c ++;
    }
  } else if( n < 0 ) {
    err = EINVAL;
  }
  *np = c;
  RETURN( err );
}

pip_task_t *pip_task_self( void ) { return PIP_TASKQ(pip_task_); }

int pip_yield( void ) {
  pip_task_t		*queue, *next;
  pip_task_internal_t	*nexti, *schedi = pip_task_->task_sched;

  ENTER;
  if( pip_task_ == NULL ) RETURN( EPERM );
  if( schedi->oodq_len > 0 ) {	/* fast check */
    pip_takein_ood_task( schedi );
  }
  queue = &schedi->schedq;
  PIP_TASKQ_ENQ_LAST( queue, PIP_TASKQ( pip_task_ ) );
  next = PIP_TASKQ_NEXT( queue );
  PIP_TASKQ_DEQ( next );
  nexti = PIP_TASKI( next );
  if( pip_task_ != nexti ) pip_task_switch( pip_task_, nexti );
  RETURN( 0 );
}

int pip_yield_to( pip_task_t *task ) {
  pip_task_internal_t	*taski = PIP_TASKI( task );
  pip_task_internal_t	*schedi;
  pip_task_t		*queue;

  IF_UNLIKELY( task == NULL ) RETURN( pip_yield() );
  if( pip_task_ == NULL ) RETURN( EPERM );
  schedi = pip_task_->task_sched;
  /* the target task must be in the same scheduling domain */
  IF_UNLIKELY( taski->task_sched != schedi ) RETURN( EPERM );

  if( schedi->oodq_len > 0 ) {	/* fast check */
    pip_takein_ood_task( schedi );
  }
  queue = &schedi->schedq;
  PIP_TASKQ_DEQ( PIP_TASKQ(task) );
  PIP_TASKQ_ENQ_LAST( queue, PIP_TASKQ(pip_task_) );

  if( pip_task_ != taski ) pip_task_switch( pip_task_, taski );
  RETURN( 0 );
}

int pip_set_aux( pip_task_t *task, void *aux ) {
  pip_task_internal_t 	*taski;

  if( pip_task_ == NULL ) RETURN( EPERM  );
  if( task == NULL ) {
    taski = pip_task_;
  } else {
    taski = PIP_TASKI( task );
  }
  taski->annex->aux = aux;
  RETURN( 0 );
}

int pip_get_aux( pip_task_t *task, void **auxp ) {
  pip_task_internal_t 	*taski;

  if( pip_task_ == NULL ) RETURN( EPERM  );
  if( auxp      == NULL ) RETURN( EINVAL );
  if( task == NULL ) {
    taski = pip_task_;
  } else {
    taski = PIP_TASKI( task );
  }
  *auxp = taski->annex->aux;
  RETURN( 0 );
}
