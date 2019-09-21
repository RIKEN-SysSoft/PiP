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

#include <pip_internal.h>
#include <pip_blt.h>

#ifdef DEBUG
#define QUEUE_DUMP( queue )				\
  if( PIP_TASKQ_ISEMPTY( queue ) ) {			\
    DBGF( "EMPTY" );					\
  } else {						\
    pip_task_t *task;					\
    char msg[128];					\
    int len = 0, iii = 0;				\
    PIP_TASKQ_FOREACH( queue, task ) len ++;		\
    PIP_TASKQ_FOREACH( queue, task ) {			\
      pip_task_queue_brief( task, msg, sizeof(msg) );	\
      DBGF( "Q-DUMP[%d/%d]: %s", iii++, len, msg );	\
    }							\
  }
#else
#define QUEUE_DUMP( queue )
#endif

static void pip_wakeup( pip_task_internal_t *taski ) {
  if( taski->flag_wakeup ) return;
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
  } else {
    DBGF( "PIPID:%d -- WAKEUP (not sleeping)",
	  taski->pipid );
  }
}

static void pip_change_sched( pip_task_internal_t *taski,
			      pip_task_internal_t *schedi ) {
  if( taski->task_sched != schedi ) {
    DBGF( "taski:%d[%d]-NTC:%d  schedi:%d[%d]-NTC:%d",
	  taski->pipid,
	  taski->task_sched->pipid,
	  (int) taski->task_sched->ntakecare,
	  schedi->pipid,
	  schedi->task_sched->pipid,
	  (int) schedi->ntakecare );
    int ntc = pip_atomic_sub_and_fetch( &taski->task_sched->ntakecare, 1 );
    ASSERTD( ntc < 0 );
    taski->task_sched = schedi;
    (void) pip_atomic_fetch_and_add( &schedi->ntakecare, 1 );
    if( ntc == 0 ) {
      DBG;
      pip_wakeup( taski->task_sched );
    }
  }
}

static void pip_sched_ood_task( pip_task_internal_t *schedi,
				pip_task_internal_t *taski ) {
  pip_task_annex_t	*annex = schedi->annex;
  int			flag;

  DBGF( "sched:PIPID:%d  taski:PIPID:%d", schedi->pipid, taski->pipid );

  pip_change_sched( taski, schedi );
  pip_spin_lock( &annex->lock_oodq );
  {
    flag = ( schedi->oodq_len == 0 );
    PIP_TASKQ_ENQ_LAST( &annex->oodq, PIP_TASKQ(taski) );
    schedi->oodq_len ++;
  }
  pip_spin_unlock( &annex->lock_oodq );
  DBG;
  if( flag ) pip_wakeup( schedi );
}

static void pip_takein_ood_task( pip_task_internal_t *schedi ) {
  pip_task_annex_t	*annex;
  int		 	oodq_len;

  ENTER;
  annex = schedi->annex;
  pip_spin_lock( &annex->lock_oodq );
  {
    //QUEUE_DUMP( &annex->oodq );
    PIP_TASKQ_APPEND( &schedi->schedq, &annex->oodq );
    oodq_len = schedi->oodq_len;
    schedi->oodq_len    = 0;
    schedi->schedq_len += oodq_len;
    //QUEUE_DUMP( &schedi->schedq );
    DBGF( "schedq_len:%d  oodq_len:%d", schedi->schedq_len, oodq_len );
  }
  pip_spin_unlock( &annex->lock_oodq );
}

#ifdef AH
static void pip_stack_protect_myself( pip_task_internal_t *taski ) {
  /* the stack must be protected until its stack is being switched */
  DBGF( "protect PIPID:%d released by myself", taski->pipid );
  ASSERTD( taski->flag_stackpp != NULL );
  taski->flag_stackp  = 1;
  taski->flag_stackpp = &taski->flag_stackp;
  pip_memory_barrier();
}

static void pip_stack_unprotect_myself( pip_task_internal_t *taski ) {
  /* this must be called when the stack is switched */
  DBGF( "UNprotect PIPID:%d by myself", taski->pipid );
#ifdef DEBUG
  if( !taski->flag_stackp ) DBGF( "ALREADY UN-protected" );
#endif
  ASSERTD( taski->flag_stackpp == NULL );
  ASSERTD( taski->flag_stackpp != &taski->flag_stackp );
  taski->flag_stackp  = 0;
  taski->flag_stackpp = NULL;
  pip_memory_barrier();
}
#endif

static void pip_stack_protect( pip_task_internal_t *taski,
			       pip_task_internal_t *nexti ) {
  /* so that new task can reset the flag */
  DBGF( "protect PIPID:%d released by PIPID:%d",
	taski->pipid, nexti->pipid );
  ASSERTD( nexti->flag_stackpp != NULL );
  taski->flag_stackp  = 1;
  nexti->flag_stackpp = &taski->flag_stackp;
  pip_memory_barrier();
}

static void pip_stack_unprotect( pip_task_internal_t *taski ) {
  /* this function must be called everytime a context is switched */
  if( taski->flag_stackpp != NULL ) {
#ifdef DEBUG
    pip_task_internal_t *prev = (pip_task_internal_t*)
      ( taski->flag_stackpp - offsetof(pip_task_internal_t,flag_stackp) );
    DBGF( "PIPID:%d UN-protect pipid:%d", taski->pipid, prev->pipid );
    if( *taski->flag_stackpp == 0 ) DBGF( "ALREADY UN-protected" );
#endif
    *taski->flag_stackpp = 0;
    taski->flag_stackpp  = NULL;
    pip_memory_barrier();
  } else {
    DBGF( "UNABLE to UN-protect PIPID:%d", taski->pipid );
  }
}

static void pip_stack_wait( pip_task_internal_t *taski ) {
  int i = 0;

  //DBGF( "PIPID:%d wait for unprotect", taski->pipid );
  if( taski->flag_stackp ) {
    for( i=0; i<pip_root_->yield_iters; i++ ) {
      pip_pause();
      if( !taski->flag_stackp ) goto done;
    }
    for( i=0; taski->flag_stackp; i++ ) {
      pip_system_yield();
      DBGF( "WAITING  pipid:%d (count=%d*%d) ...",
	    taski->pipid, i, PIP_BUSYWAIT_COUNT );
#ifdef DEBUG
      pip_task_internal_t *prev = (pip_task_internal_t*)
	( taski->flag_stackpp - offsetof(pip_task_internal_t,flag_stackp) );
      DBGF( "WAITING  pipid:%d (count=%d*%d, unprotect-PIPID:%d) ...",
	    taski->pipid, i, PIP_BUSYWAIT_COUNT,
	    prev->pipid );
      if( i > 100 ) pip_abort();
#endif
    }
  }
 done:
  DBGF( "WAIT-DONE  pipid:%d (count=%d*%d)",
	taski->pipid, i, PIP_BUSYWAIT_COUNT );
}

static void pip_task_sched( pip_task_internal_t *taski ) {
  void *ctxp = taski->ctx_suspend;

  ASSERTD( ctxp == NULL );
  taski->ctx_suspend = NULL;
  pip_stack_wait( taski );
#ifdef PIP_LOAD_TLS
  ASSERT( pip_load_tls( taski->tls ) );
#endif
  ASSERT( pip_load_context( ctxp ) );
  NEVER_REACH_HERE;
}

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
  pip_task_internal_t	*taski = (pip_task_internal_t*)
    ( ( ((intptr_t) task_H) << 32 ) | ( ((intptr_t) task_L) & PIP_MASK32 ) );
  int	i, j;

  ENTER;
  DBGF( "PIPID:%d", taski->pipid );
  /* now stack is switched and unprotect the stack */
  pip_stack_unprotect( taski );

  while( 1 ) {
    pip_takein_ood_task( taski );
    if( !PIP_TASKQ_ISEMPTY( &taski->schedq ) ) break;

    if( taski->flag_exit && !taski->ntakecare ) {
      void *ctxp = taski->annex->ctx_exit;

      DBGF( "PIPID:%d -- WOKEUP to EXIT", taski->pipid );
      /* wait for the stack is free to use */
      ASSERTD( ctxp == NULL );
      taski->annex->ctx_exit = NULL;
      /* then switch to the exit context */
#ifdef PIP_LOAD_TLS
      ASSERT( pip_load_tls( taski->tls ) );
#endif
      ASSERT( pip_load_context( ctxp ) );
      NEVER_REACH_HERE;
    }

    pip_deadlock_inc_();
    {
      switch( taski->annex->opts & PIP_SYNC_MASK ) {
      case PIP_SYNC_AUTO:
      default:
	for( j=0; j<10; j++ ) {
	  for( i=0; i<pip_root_->yield_iters; i++ ) {
	    if( taski->flag_wakeup ) {
	      DBGF( "PIPID:%d -- SLEEPING (blocking-busywait:%d)", taski->pipid, i );
	      goto done;
	    }
	  }
	  pip_system_yield();
	}
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
      }
      DBGF( "PIPID:%d -- WOKEUP ^^^^^^^^", taski->pipid );
    }
  done:
    pip_deadlock_dec_();
    taski->flag_wakeup = 0;
  }
  //AH//pip_change_sched( taski, taski );

  //QUEUE_DUMP( &taski->schedq );
  ASSERTD( taski->schedq_len == 0 );
  ASSERTD( PIP_TASKQ_ISEMPTY( &taski->schedq ) );
  {
    pip_task_t *next = PIP_TASKQ_NEXT( &taski->schedq );
    pip_task_internal_t *nexti = PIP_TASKI( next );

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
  pip_ctx_t		ctx;
  stack_t		*stk;
  int 			args_H, args_L;

  ENTER;
  DBGF( "PIPID:%d", taski->pipid );
  /* creating new context to switch stack */
  ASSERT( pip_save_context( &ctx ) );
  stk = &(ctx.ctx.uc_stack);
  stk->ss_sp    = taski->annex->sleep_stack;
  stk->ss_size  = pip_root_->stack_size_sleep;
  stk->ss_flags = 0;
  args_H = ( ((intptr_t) taski) >> 32 ) & PIP_MASK32;
  args_L = (  (intptr_t) taski)         & PIP_MASK32;
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

  curr_task->ctx_suspend = &ctx;
  new_ctxp = new_task->ctx_suspend;
  new_task->ctx_suspend = NULL;
  ASSERTD( new_ctxp == NULL );
#ifdef PIP_LOAD_TLS
  ASSERT( pip_load_tls( new_task->tls ) );
#endif
  ASSERT( pip_swap_context( &ctx, new_ctxp ) );
  /* resumed */
  pip_stack_unprotect( curr_task );
  RETURNV;
}

static pip_task_internal_t *pip_schedq_next( pip_task_internal_t *taski ) {
  pip_task_internal_t 	*schedi = taski->task_sched;
  pip_task_internal_t 	*nexti;
  pip_task_t		*next;

  ASSERTD( schedi == NULL );
  DBGF( "sched-PIPID:%d", schedi->pipid );
  pip_takein_ood_task( schedi );
  if( !PIP_TASKQ_ISEMPTY( &schedi->schedq ) ) {
    QUEUE_DUMP( &schedi->schedq );
    next = PIP_TASKQ_NEXT( &schedi->schedq );
    PIP_TASKQ_DEQ( next );
    schedi->schedq_len --;
    ASSERTD( schedi->schedq_len < 0 );
    nexti = PIP_TASKI( next );
    DBGF( "next-PIPID:%d", nexti->pipid );

  } else if( schedi->flag_exit ) {
    /* if the scheduler has no scheduling tasks, then switch bask to */
    /* the scheduler so that scheduling task can terminate itself    */
    DBG;
    nexti = schedi;

  } else {
    nexti = NULL;
    DBGF( "next:NULL" );
  }
  return nexti;
}

void pip_suspend_and_enqueue_generic_( pip_task_internal_t *taski,
				       pip_task_queue_t *queue,
				       int flag_lock,
				       pip_enqueue_callback_t callback,
				       void *cbarg ) {
  void pip_enqueue_task( pip_task_internal_t *taski,
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
    if( callback != NULL ) callback( cbarg );
  }

  pip_task_internal_t	*schedi = taski->task_sched;
  pip_task_internal_t	*nexti  = pip_schedq_next( taski );
  volatile int 	flag_jump = 0;	/* must be volatile */
  pip_ctx_t	ctx;

  ENTER;
  PIP_SUSPEND( taski );
  taski->ctx_suspend = &ctx;
  ASSERT( pip_save_context( &ctx ) );
  if( !flag_jump ) {
    flag_jump = 1;
    if( nexti != NULL ) {
      /* the next scheduled task will unprotect the stack */
      if( queue != NULL ) {
	/* before enqueue, the stack must be protected */
	pip_stack_protect( taski, nexti );
	pip_enqueue_task( taski, queue, flag_lock, callback, cbarg );
      }
      /* context-switch to the next task */
      DBGF( "PIPID:%d(ctxp:%p) -->> PIPID:%d(CTXP:%p)",
	    taski->pipid, taski->ctx_suspend,
	    nexti->pipid, nexti->ctx_suspend );
      pip_task_sched( nexti );

    } else {
      /* if there is no task to schedule next, block scheduling task */
      if( queue != NULL ) {
	/* before enqueue, the stack must be protected */
	pip_stack_protect( taski, schedi );
	pip_enqueue_task( taski, queue, flag_lock, callback, cbarg );
      }
      pip_block_task( schedi );
    }
    NEVER_REACH_HERE;
  }
  /* taski is eventually resumed */
  pip_stack_unprotect( taski );
}

void pip_do_exit( pip_task_internal_t *taski, int extval ) {
  extern void pip_set_extval( pip_task_internal_t*, int );
  pip_task_internal_t	*schedi = taski->task_sched;
  pip_task_t		*queue, *next;
  pip_task_internal_t	*nexti;
  /* this is called when 1) return from main (or func) function */
  /*                     2) pip_exit() is explicitly called     */
  ENTER;
  DBGF( "PIPID:%d", taski->pipid );
  ASSERTD( schedi == NULL );

  pip_set_extval( taski, extval );

  DBGF( "TASKI:  %d[%d]", taski->pipid,  taski->task_sched->pipid  );
  DBGF( "schedi->ntakecare:%d", (int) schedi->ntakecare );

#ifdef DEBUG
  int ntc = pip_atomic_sub_and_fetch( &schedi->ntakecare, 1 );
  ASSERTD( ntc < 0 );
#else
  (void) pip_atomic_sub_and_fetch( &schedi->ntakecare, 1 );
#endif

 again:
  queue = &schedi->schedq;
  pip_takein_ood_task( schedi );
  if( !PIP_TASKQ_ISEMPTY( queue ) ) {
    next = PIP_TASKQ_NEXT( queue );
    PIP_TASKQ_DEQ( next );
    schedi->schedq_len --;
    ASSERTD( schedi->schedq_len < 0 );
    nexti = PIP_TASKI( next );
    DBGF( "PIPID:%d(ctxp:%p) -->> PIPID:%d(CTXP:%p)",
	  taski->pipid, taski->ctx_suspend,
	  nexti->pipid, nexti->ctx_suspend );
    if( taski == schedi ) {
      volatile int flag_jump = 0;	/* must be volatile */
      pip_ctx_t	 ctx;

      PIP_TASKQ_ENQ_LAST( queue, PIP_TASKQ(taski) );
      taski->schedq_len ++;
      taski->ctx_suspend = &ctx;
      ASSERT( pip_save_context( &ctx ) );
      if( !flag_jump ) {
	flag_jump = 1;
	pip_task_sched( nexti );
	NEVER_REACH_HERE;
      }
      goto again;
    } else {
      DBG;
      pip_wakeup( taski );    /* wake up (if sleeping) to terminate */
      pip_task_sched( nexti );
      NEVER_REACH_HERE;
    }
  } else if( taski != schedi ) {
    /* queue is empty and nothing to schedule */
    pip_wakeup( taski );    /* wake up (if sleeping) to terminate */
  }
  /* there might be suspended tasks waiting to be scheduled */
  pip_block_task( schedi );
  NEVER_REACH_HERE;
}

static int pip_do_resume( pip_task_internal_t *taski,
			  pip_task_internal_t *resume,
			  pip_task_internal_t *schedi ) {
  ENTER;
  DBGF( "resume:%d[%d](%c)  schedi:%d",
	resume->pipid, resume->task_sched->pipid, resume->state,
	( schedi != NULL )? schedi->pipid : -1 );

  if( taski == resume ) RETURN( 0 );
  if( PIP_IS_RUNNING( resume ) ) RETURN( EPERM );

  if( schedi == NULL ) {
    /* resume in the previous scheduling domain */
    schedi = resume->task_sched;
  } else {
    /* get the right scheduling task (domain) */
    schedi = schedi->task_sched;
  }
  ASSERTD( schedi == NULL );

  PIP_RUN( resume );
  if( taski->task_sched == schedi ) {
    /* same scheduling domain with the current task */
    DBGF( "RUN: %d[%d]", resume->pipid, schedi->pipid );
    PIP_TASKQ_ENQ_LAST( &schedi->schedq, PIP_TASKQ(resume) );
    schedi->schedq_len ++;
  } else {
    /* schedule the task as an OOD (Out Of (scheduling) Domain) task */
    DBGF( "RUN(OOD): %d[%d]", resume->pipid, schedi->pipid );
    pip_sched_ood_task( schedi, resume );
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
  pip_task_internal_t	*taski = pip_task_;
  IF_UNLIKELY( taski  == NULL ) RETURN( EPERM  );
  IF_UNLIKELY( task   == NULL ) RETURN( EINVAL );
  IF_LIKELY(   pipidp != NULL ) *pipidp = PIP_TASKI(task)->pipid;
  RETURN( 0 );
}

int pip_get_sched_domain( pip_task_t **domainp ) {
  pip_task_internal_t	*taski = pip_task_;
  IF_UNLIKELY( taski   == NULL ) RETURN( EPERM  );
  IF_UNLIKELY( domainp == NULL ) RETURN( EINVAL );
  *domainp = PIP_TASKQ( pip_task_->task_sched );
  RETURN( 0 );
}

int pip_count_runnable_tasks( int *countp ) {
  pip_task_internal_t	*taski = pip_task_;
  IF_UNLIKELY( taski   == NULL ) RETURN( EPERM  );
  IF_UNLIKELY( countp == NULL ) RETURN( EINVAL );
  *countp = pip_task_->schedq_len;
  RETURN( 0 );
}

int pip_enqueue_runnable_N( pip_task_queue_t *queue, int *np ) {
  /* runnable tasks in the schedq of the current */
  /* task are enqueued into the queue            */
  pip_task_internal_t	*taski = pip_task_;
  pip_task_internal_t 	*schedi;
  pip_task_t	 	*task, *next, *schedq;
  int			c, n;

  ENTER;
  IF_UNLIKELY( taski == NULL ) RETURN( EPERM  );
  IF_UNLIKELY( np    == NULL ) RETURN( EINVAL );

  schedi = taski->task_sched;
  ASSERTD( schedi == NULL );
  schedq = &schedi->schedq;
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
  schedi->schedq_len -= c;
  ASSERTD( schedi->schedq_len < 0 );
  if( np != NULL ) *np = c;
  return 0;
}

int pip_suspend_and_enqueue_( pip_task_queue_t *queue,
			      pip_enqueue_callback_t callback,
			      void *cbarg ) {
  pip_task_internal_t	*taski = pip_task_;
  IF_UNLIKELY( taski == NULL ) RETURN( EPERM );
  IF_UNLIKELY( queue == NULL ) RETURN( EINVAL );
  pip_suspend_and_enqueue_generic_( taski, queue, 1,
				    callback, cbarg );
  return 0;
}

int pip_suspend_and_enqueue_nolock_( pip_task_queue_t *queue,
				     pip_enqueue_callback_t callback,
				     void *cbarg ) {
  pip_task_internal_t	*taski = pip_task_;
  IF_UNLIKELY( taski == NULL ) RETURN( EPERM );
  IF_UNLIKELY( queue == NULL ) RETURN( EINVAL );
  pip_suspend_and_enqueue_generic_( taski, queue, 0,
				    callback, cbarg );
  return 0;
}

int pip_resume( pip_task_t *resume, pip_task_t *sched ) {
  pip_task_internal_t	*taski = pip_task_;
  IF_UNLIKELY( taski == NULL ) RETURN( EPERM );
  return pip_do_resume( taski, PIP_TASKI(resume), PIP_TASKI(sched) );
}

int pip_dequeue_and_resume_( pip_task_queue_t *queue, pip_task_t *sched ) {
  pip_task_internal_t	*taski = pip_task_;
  pip_task_t *resume;

  ENTER;
  IF_UNLIKELY( taski == NULL ) RETURN( EPERM );
  pip_task_queue_lock( queue );
  resume = pip_task_queue_dequeue( queue );
  pip_task_queue_unlock( queue );
  IF_UNLIKELY( resume == NULL ) RETURN( ENOENT );
  RETURN( pip_do_resume( pip_task_, PIP_TASKI(resume), PIP_TASKI(sched) ) );
}

int pip_dequeue_and_resume_nolock_( pip_task_queue_t *queue,
				    pip_task_t *sched ) {
  ENTER;
  pip_task_t *resume;
  resume = pip_task_queue_dequeue( queue );
  if( resume == NULL ) RETURN( ENOENT );
  RETURN( pip_do_resume( pip_task_, PIP_TASKI(resume), PIP_TASKI(sched) ) );
}

int pip_dequeue_and_resume_N_( pip_task_queue_t *queue,
			       pip_task_t *sched,
			       int *np ) {
  pip_task_internal_t	*taski = pip_task_;
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
    {
      while( ( resume = pip_task_queue_dequeue( queue ) ) != NULL ) {
	PIP_TASKQ_ENQ_LAST( &tmpq, resume );
	c ++;
      }
    }
    pip_task_queue_unlock( queue );
  } else if( n > 0 ) {
    DBGF( "N: %d", n );
    pip_task_queue_lock( queue );
    {
      while( c < n ) {
	resume = pip_task_queue_dequeue( queue );
	if( resume == NULL ) break;
	PIP_TASKQ_ENQ_LAST( &tmpq, resume );
	c ++;
      }
    }
    pip_task_queue_unlock( queue );
  } else if( n < 0 ) {
    RETURN( EINVAL );
  }
  PIP_TASKQ_FOREACH_SAFE( &tmpq, resume, next ) {
    PIP_TASKQ_DEQ( resume );
    err = pip_do_resume( taski, PIP_TASKI(resume), PIP_TASKI(sched) );
    if( err ) break;
  }
  if( !err ) *np = c;
  RETURN( err );
}

int pip_dequeue_and_resume_N_nolock_( pip_task_queue_t *queue,
				      pip_task_t *sched,
				      int *np ) {
  pip_task_internal_t	*taski = pip_task_;
  pip_task_t 		*resume;
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
    err = pip_do_resume( taski, PIP_TASKI(resume), PIP_TASKI(sched) );
    if( err ) break;
  }
  if( !err ) *np = c;
  RETURN( err );
}

pip_task_t *pip_task_self( void ) { return PIP_TASKQ(pip_task_); }

int pip_yield( int flag ) {
  pip_task_t		*queue, *next;
  pip_task_internal_t	*taski = pip_task_;
  pip_task_internal_t	*nexti, *schedi = taski->task_sched;

  ENTER;
  IF_UNLIKELY( taski == NULL ) RETURN( EPERM );

  if( flag == PIP_YIELD_DEFAULT ||
      flag &  PIP_YIELD_SYSTEM ) {
    pip_system_yield();
  }

  IF_UNLIKELY( schedi->oodq_len > 0 ) {	/* fast check */
    pip_takein_ood_task( schedi );
  }
  queue = &schedi->schedq;
  IF_UNLIKELY( PIP_TASKQ_ISEMPTY( queue ) ) RETURN( 0 );

  PIP_TASKQ_ENQ_LAST( queue, PIP_TASKQ(taski) );
  next = PIP_TASKQ_NEXT( queue );
  PIP_TASKQ_DEQ( next );
  nexti = PIP_TASKI( next );
  ASSERTD( taski == nexti );
  DBGF( "next-PIPID: %d", nexti->pipid );
  pip_task_switch( taski, nexti );
  RETURN( EINTR );
}

int pip_yield_to( pip_task_t *target ) {
  pip_task_internal_t	*targeti = PIP_TASKI( target );
  pip_task_internal_t	*schedi, *taski = pip_task_;
  pip_task_t		*queue;

  IF_UNLIKELY( targeti == NULL  ) {
    RETURN( pip_yield( PIP_YIELD_DEFAULT ) );
  }
  IF_UNLIKELY( taski   == NULL  ) RETURN( EPERM );
  IF_UNLIKELY( targeti == taski ) RETURN( 0 );
  schedi = taski->task_sched;
  /* the target task must be in the same scheduling domain */
  IF_UNLIKELY( targeti->task_sched != schedi ) RETURN( EPERM );

  IF_UNLIKELY( schedi->oodq_len > 0 ) {	/* fast check */
    pip_takein_ood_task( schedi );
  }
  queue = &schedi->schedq;
  PIP_TASKQ_DEQ( target );
  PIP_TASKQ_ENQ_LAST( queue, PIP_TASKQ(taski) );
  pip_task_switch( taski, targeti );
  RETURN( EINTR );
}

int pip_set_aux( pip_task_t *task, void *aux ) {
  pip_task_internal_t 	*taski;

  IF_UNLIKELY( pip_task_ == NULL ) RETURN( EPERM  );
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

  IF_UNLIKELY( pip_task_ == NULL ) RETURN( EPERM  );
  IF_UNLIKELY( auxp      == NULL ) RETURN( EINVAL );
  if( task == NULL ) {
    taski = pip_task_;
  } else {
    taski = PIP_TASKI( task );
  }
  *auxp = taski->annex->aux;
  RETURN( 0 );
}
