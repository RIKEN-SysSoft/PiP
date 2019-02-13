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

/* State Transition of Bi-Level Tasks */
/* ACTIVE task (its PID task is running) */
/* - active (kernel thread is active (schedq is not  empty)) */
/* - passive (kernel thread is blocked (schedq is empty)) */
/* PASSIVE task (its PID task is suspended by OS) */
/* - passive (possibly in a waiting queue) */
/* - running (actually running but with the scheduling task's PID */
/* - runnable (in the schedq, waiting until scheduled) */

//#define _GNU_SOURCE

#include <sys/mman.h>
#include <sched.h>
#include <signal.h>
#include <unistd.h>

#include <pip_internal.h>
#include <pip_util.h>

static int pip_raise_sigchld( void ) {
  extern int pip_raise_signal_( pip_task_internal_t*, int );
  RETURN( pip_raise_signal_( pip_root->task_root, SIGCHLD ) );
}

static void pip_force_exit( pip_task_internal_t *taski ) {
  extern int pip_is_threaded_( void );

  ENTER;
  if( pip_is_threaded_() ) {
    (void) pip_raise_sigchld();
    pthread_exit( NULL );
  } else {			/* process mode */
    exit( taski->annex->extval );
  }
  NEVER_REACH_HERE;
}

static void pip_load_context_( pip_ctx_t *ctxp ) {
  ASSERT( ctxp == NULL );
  DBG;
  ASSERT( pip_load_context( ctxp ) != 0 );
}

static void pip_sched_ood_task( pip_task_internal_t *schedi,
				pip_task_internal_t *taski ) {
  ENTER;
  schedi = schedi->task_sched;	/* to make sure */
  DBGF( "sched:PIPID:%d  taski:PIPID:%d", schedi->pipid, taski->pipid );
  taski->task_sched = schedi;

  pip_spin_lock( &schedi->annex->lock_oodq );
  {
    PIP_TASKQ_ENQ_LAST( &schedi->annex->oodq, PIP_TASKQ( taski ) );
    schedi->oodq_len ++;
  }
  pip_spin_unlock( &schedi->annex->lock_oodq );

  DBGF( "oodq_len:%d  flag_semwait:%d",
	schedi->oodq_len, schedi->flag_semwait );
  if( schedi->flag_semwait ) {
    /* not to call sem_post() more than once per blocking */
    schedi->flag_semwait = 0;
    errno = 0;
    DBGF( "WAKEUP vvvvvvvvvv PIPID:%d", schedi->pipid );
    sem_post( &schedi->annex->sleep );
    ASSERT( errno );
  }
  RETURNV;
}

static int pip_takein_ood_task( pip_task_internal_t *taski ) {
  pip_task_internal_t 	*schedi  = taski->task_sched;
  int 			oodq_len = schedi->oodq_len;

  if( oodq_len == 0 ) {
    DBGF( "oodq is empty" );
  } else {
    ENTER;
    pip_spin_lock( &schedi->annex->lock_oodq );
    {
#ifdef DEBUG
      {
	pip_task_t *task;
	int i = 0;
	PIP_TASKQ_FOREACH( &taski->annex->oodq, task ) {
	  DBGF( "PIPID:%d [%d]", PIP_TASKI(task)->pipid, i++ );
	}
      }
#endif
      PIP_TASKQ_APPEND( &schedi->schedq, &schedi->annex->oodq );
      schedi->oodq_len    = 0;
      schedi->schedq_len += oodq_len;
      DBGF( "PIPID:%d schedq_len:%d", schedi->pipid, schedi->schedq_len );
    }
    pip_spin_unlock( &schedi->annex->lock_oodq );
  }
  return oodq_len;
}

static void pip_stack_protect( pip_task_internal_t *curr,
			       pip_task_internal_t *new ) {
  DBGF( "PIPID:%d protected, and will be unprotected by PIPID:%d",
	curr->pipid, new->pipid );
#ifdef DEBUG
  if( new->flag_ctxswp ) {
    DBGF( "PIPID:%d is already protected by PIPID:%d", new->pipid,
	  ((pip_task_internal_t*)
	   (new->flag_ctxswp -
	    offsetof(pip_task_internal_t,flag_ctxsw)))->pipid );
  }
#endif
  if( curr != new ) while( new->flag_ctxswp != NULL ) pip_pause();
  curr->flag_ctxsw = 1;			/* protect current until */
  new->flag_ctxswp = &curr->flag_ctxsw; /* next task is scheduled */
}

static void pip_stack_unprotect( pip_task_internal_t *taski ) {
  /* this function musrt be called everytime a context is switched */
  if( taski->flag_ctxswp != NULL ) {
    DBGF( "PIPID:%d unprotecting PIPID:%d", taski->pipid,
	  ((pip_task_internal_t*)
	   (taski->flag_ctxswp -
	    offsetof(pip_task_internal_t,flag_ctxsw)))->pipid );
    *taski->flag_ctxswp = 0;
    taski->flag_ctxswp  = NULL;
  } else {
    DBGF( "pipid:%d  <<<NULL>>>", taski->pipid );
  }
}

void pip_stack_wait_( pip_task_internal_t *taski ) {
  int pip_stack_isbusy( pip_task_internal_t *taski ) {
    return taski->flag_ctxsw || taski->flag_sleep;
  }
  int i;

  DBGF( ">>>> pipid:%d", taski->pipid );
  if( pip_stack_isbusy( taski ) ) {
    DBGF( "PIPID:%d waiting for unprotect", taski->pipid );
    for( i=0; i<PIP_BUSYWAIT_COUNT; i++ ) {
      if( !pip_stack_isbusy( taski ) ) {
	DBGF( "WAIT-COUNT:%d", i );
	goto done;
      }
      pip_pause();
    }
    i = 0;
    while( pip_stack_isbusy( taski ) ) {
      sched_yield();
      i++;
    }
    DBGF( "YILED-COUNT:%d", i );
  }
 done:
  DBGF( "<<<< pipid:%d", taski->pipid );
  return;
}

static pip_task_internal_t *pip_schedq_next( pip_task_internal_t *taski ) {
  pip_task_internal_t 	*schedi = taski->task_sched;
  pip_task_internal_t 	*nexti;
  pip_task_t		*next;

  ASSERT( schedi == NULL );
  DBGF( "sched-PIPID:%d", schedi->pipid );
  (void) pip_takein_ood_task( schedi );
  if( !PIP_TASKQ_ISEMPTY( &schedi->schedq ) ) {
    schedi->schedq_len --;
    DBGF( "PIPID:%d schedq_len:%d", schedi->pipid, schedi->schedq_len );
    next = PIP_TASKQ_NEXT( &schedi->schedq );
    PIP_TASKQ_DEQ( next );
    nexti = PIP_TASKI( next );
    DBGF( "next-PIPID:%d", nexti->pipid );
    pip_stack_protect( taski, nexti );

  } else if( schedi->flag_exit ) {
    /* this is the last one and now sched can terminate itself */
    nexti = schedi;
    DBGF( "next:SCHED (exit)" );

  } else {
    nexti = NULL;
    //pip_stack_protect( taski, taski );
    DBGF( "next:NULL" );
  }
  return nexti;
}

static void pip_task_sched( pip_task_internal_t *taski ) {
  pip_stack_wait_( taski );	/* wait until context is free to use */
  ASSERT( taski->ctx_suspend == NULL );
  void *ctxp = taski->ctx_suspend;
  taski->ctx_suspend = NULL;
  pip_load_context_( ctxp );
  NEVER_REACH_HERE;
}

static void pip_task_sched_with_tls( pip_task_internal_t *taski ) {
#ifdef PIP_LOAD_TLS
  ASSERT( pip_load_tls( taski->tls ) );
#endif
  pip_task_sched( taski );
  NEVER_REACH_HERE;
}

typedef struct pip_sleep_args {
  pip_task_internal_t	*taski;
  pip_task_queue_t	*queue;
  int 			flag_lock;
  pip_enqueue_callback_t callback;
  void			*cbarg;
} pip_sleep_args_t;

static void pip_sleep( intptr_t task_H, intptr_t task_L ) {
  void pip_block_myself( pip_task_internal_t *taski ) {
    ENTER;
    if( pip_takein_ood_task( taski ) == 0 ) {
      while( 1 ) {
	errno = 0;
	if( sem_wait( &taski->annex->sleep ) == 0 ) break;
	if( errno != EINTR ) ASSERT( errno );
      }
      taski->flag_semwait = 0;
    }
    RETURNV;
  }
  extern void pip_deadlock_inc_( void );
  extern void pip_deadlock_dec_( void );
  pip_sleep_args_t 	*args = (pip_sleep_args_t*)
    ( ( ((intptr_t) task_H) << 32 ) | ( ((intptr_t) task_L) & PIP_MASK32 ) );
  pip_task_internal_t	*taski = args->taski;
  int i;

  ENTER;
  /* now the stack is switched and one may swicth to the context */
  //taski->flag_sleep = 0;	/* unprotect my stack */

  if( args->queue != NULL ) {
    if( args->flag_lock ) pip_task_queue_lock( args->queue );
    pip_task_queue_enqueue( args->queue, PIP_TASKQ( taski ) );
    if( args->flag_lock ) pip_task_queue_unlock( args->queue );
    if( args->callback != NULL ) args->callback( args->cbarg );
  }

  while( 1 ) {
    (void) pip_takein_ood_task( taski ); /* taski -> schedi */
    DBGF( "PIPID:%d schedq_len:%d", taski->pipid, taski->schedq_len );
    if( !PIP_TASKQ_ISEMPTY( &taski->schedq ) ) {
      DBG;
      break;
    }
    pip_deadlock_inc_();
    {
      switch( taski->annex->opts & PIP_SYNC_MASK ) {
      case PIP_SYNC_BLOCKING:
	DBGF( "SLEEPING (blocking) zzzzzzz" );
	taski->flag_semwait = 1;
	pip_block_myself( taski );
	break;
      case PIP_SYNC_BUSYWAIT:
	DBGF( "SLEEPING (busywait) zzzzzzzz" );
	while( !taski->oodq_len ) pip_pause();
	break;
      case PIP_SYNC_YIELD:
	DBGF( "SLEEPING (yield) zzzzzzzz" );
	while( !taski->oodq_len ) sched_yield();
	break;
      case PIP_SYNC_AUTO:
      default:
	for( i=0; i<PIP_BUSYWAIT_COUNT; i++ ) {
	  if( taski->oodq_len ) {
	    DBGF( "SLEEPING (blocking-busywait:%d)", i );
	    goto done;
	  }
	}
	DBGF( "SLEEPING (blocking) zzzzzzz" );
	taski->flag_semwait = 1;
	pip_block_myself( taski );
      }
    }
  done:
    pip_deadlock_dec_();
  }
  DBGF( "WOKEUP (schedq_len:%d) ^^^^^^^^", taski->schedq_len );
  if( taski->flag_exit ) {
    DBGF( "exiting" );
    pip_force_exit( taski );
  } else {
    taski->task_sched = taski;
  }
  {
    pip_task_t *next = PIP_TASKQ_NEXT( &taski->schedq );
    pip_task_internal_t *nexti = PIP_TASKI( next );
    ASSERT( next == NULL );
    PIP_TASKQ_DEQ( next );
    pip_task_sched( nexti );
  }
  NEVER_REACH_HERE;
}

#define STACK_PG_NUM	(16)

static void pip_switch_stack_and_sleep( pip_task_internal_t *taski,
					pip_task_queue_t *queue,
					int flag_lock,
					pip_enqueue_callback_t callback,
					void *cbarg ) {
  extern void pip_page_alloc_( size_t, void** );
  pip_sleep_args_t	args;
  size_t	pgsz  = pip_root->page_size;
  size_t	stksz = STACK_PG_NUM * pgsz;
  pip_ctx_t	ctx;
  stack_t	*stk = &(ctx.ctx.uc_stack);
  void		*stack, *redzone;
  int 		args_H, args_L;

  ENTER;
  if( ( stack = taski->annex->sleep_stack ) == NULL ) {
    pip_page_alloc_( stksz + pgsz, &stack );
    redzone = stack + stksz;
    ASSERT( mprotect( redzone, pgsz, PROT_NONE ) );
    taski->annex->sleep_stack = stack;
  }
  args.taski     = taski;
  args.queue     = queue;
  args.flag_lock = flag_lock;
  args.callback  = callback;
  args.cbarg     = cbarg;

  //taski->flag_sleep = 1; /* set the another busy flag to protect stack */

  /* creating new context to switch stack */
  pip_save_context( &ctx );
  ctx.ctx.uc_link = NULL;
  stk->ss_sp      = stack;
  stk->ss_flags   = 0;
  stk->ss_size    = stksz;
  args_H = ( ((intptr_t) &args) >> 32 ) & PIP_MASK32;
  args_L = (  (intptr_t) &args)         & PIP_MASK32;
  pip_make_context( &ctx, pip_sleep, 4, args_H, args_L );
  /* no need of saving/restoring TLS */
  pip_load_context_( &ctx );
}

static void pip_task_switch( pip_task_internal_t *old_task,
			     pip_task_internal_t *new_task ) {
  ENTER;
  DBGF( "pipid:%d -> pipid:%d", old_task->pipid, new_task->pipid );

  if( old_task != new_task ) {
#ifdef PIP_USE_STATIC_CTX
    old_task->ctx_suspend = old_task->ctx_static;
    new_task->ctx_suspend = new_task->ctx_static;
#else  /* context on stack */
    pip_ctx_t 	oldctx;
    old_task->ctx_suspend = &oldctx;
#endif

    pip_stack_wait_( new_task );
    DBG;
    pip_stack_protect( old_task, new_task );
#ifdef PIP_LOAD_TLS
    ASSERT( pip_load_tls( new_task->tls ) );
#endif
    ASSERT( pip_swap_context( old_task->ctx_suspend,
			      new_task->ctx_suspend ) );
    pip_stack_unprotect( old_task );
    DBG;
  }
}

static void pip_sched_next( pip_task_internal_t *taski,
			    pip_task_internal_t *nexti,
			    pip_task_queue_t *queue,
			    int flag_lock,
			    pip_enqueue_callback_t callback,
			    void *cbarg ) {
  ENTER;
  if( nexti == NULL ) {
    /* block the scheduing task since it has no tasks to be scheduled */
    DBG;
    pip_switch_stack_and_sleep( taski->task_sched,
				queue, flag_lock, callback, cbarg );
    NEVER_REACH_HERE;
  }
  DBGF( "next-PIPID:%d", nexti->pipid );
  /* OK, context-switch to the next task */
  if( queue != NULL ) {
    if( flag_lock ) pip_task_queue_lock( queue );
    pip_task_queue_enqueue( queue, PIP_TASKQ( taski ) );
    if( flag_lock ) pip_task_queue_unlock( queue );
    if( callback != NULL ) callback( cbarg );
  }
  DBGF( "PIPID:%d(ctxp:%p) -->> PIPID:%d(CTXP:%p)",
	taski->pipid, taski->ctx_suspend, nexti->pipid, nexti->ctx_suspend );
  taski->task_sched->schedq_len --;
  DBGF( "PIPID:%d schedq_len:%d", taski->task_sched->pipid,
	taski->task_sched->schedq_len );
  pip_task_sched_with_tls( nexti );
  NEVER_REACH_HERE;
}

void pip_do_exit( pip_task_internal_t *taski, int extval ) {
  extern void pip_task_terminated_( pip_task_internal_t*, int );
  pip_task_internal_t	*nexti, *schedi;
  pip_task_t 		*task, *next;
  /* this is called when 1) return from main (or func) function */
  /*                     2) pip_exit() is explicitly called     */
  ENTER;
  DBGF( "PIPID:%d", taski->pipid );

  pip_task_terminated_( taski, extval );

  schedi = taski->task_sched;
  ASSERT( schedi == NULL );
  if( schedi != taski ) {
    /* this is not the scheduling task */
    DBGF( "Exit is Defered PIPID:%d", taski->pipid );
    /* enqueue me into the exitq of the scheduling task. */
    /* when the scheduling taks terminates, all tasks    */
    /* in the exitq will be activated to terminate       */
    /* see the code at the end of this function          */
    PIP_TASKQ_ENQ_LAST( &schedi->annex->exitq, PIP_TASKQ(taski) );
    /* then schedule the next one if any when schedq becomes empty */
    /* the scheduling task will be scheduled to terminate          */
    if( PIP_TASKQ_ISEMPTY( &schedi->schedq ) ) {
      nexti = schedi;
    } else {
      schedi->schedq_len --;
      DBGF( "PIPID:%d schedq_len:%d", schedi->pipid, schedi->schedq_len );
      next = PIP_TASKQ_NEXT( &schedi->schedq );
      PIP_TASKQ_DEQ( next );
      nexti = PIP_TASKI( next );
    }
    //pip_task_sched_with_tls( nexti );
    pip_task_switch( taski, nexti );

  } else {
    /* this is the scheduling task */
    if( ( nexti = pip_schedq_next( taski ) ) != taski ) {
      /* schedule the tasks in the schedq */
      pip_ctx_t		*ctxp;
      volatile int 	flag_jump = 0;	/* must be volatile */
#ifdef PIP_USE_STATIC_CTX
      ctxp = taski->ctx_static;
#else
      pip_ctx_t		ctx;
      ctxp = &ctx;
#endif
      taski->ctx_suspend = ctxp;
      pip_save_context( ctxp );
      if( !flag_jump ) {
	flag_jump = 1;
	pip_sched_next( taski, nexti, NULL, 0, NULL, NULL );
      }
      pip_stack_unprotect( taski );
    }
    /* there is no other tasks eligible to run and so terminate itself */
    PIP_TASKQ_FOREACH_SAFE( &taski->annex->exitq, task, next ) {
      PIP_TASKQ_DEQ( task );
      /* wakeup to exit */
      DBGF( "Defered Exit PIPID:%d", PIP_TASKI(task)->pipid );
      PIP_TASKI(task)->task_sched = PIP_TASKI(task);
      pip_sched_ood_task( PIP_TASKI(task), PIP_TASKI(task) );
    }
  }
  /* and termintae myself */
  pip_force_exit( taski );
  NEVER_REACH_HERE;
}

static void pip_do_suspend( pip_task_internal_t *taski,
			    pip_task_internal_t *nexti,
			    pip_task_queue_t *queue,
			    int flag_lock,
			    pip_enqueue_callback_t callback,
			    void *cbarg ) {
  volatile int	flag_jump = 0;	/* must be volatile */
  pip_ctx_t	*ctxp;
#ifdef PIP_USE_STATIC_CTX
  ctxp = pip_task->ctx_static;
#else
  pip_ctx_t 	ctx;
  ctxp = &ctx;
#endif
  taski->ctx_suspend = ctxp;

  pip_save_context( ctxp );
  if( !flag_jump ) {
    flag_jump = 1;
    pip_sched_next( taski, nexti, queue, flag_lock, callback, cbarg );
  }
  pip_stack_unprotect( taski );
}

static int pip_do_resume( pip_task_internal_t *taski,
			  pip_task_internal_t *schedi ) {
  ENTER;
  DBGF( "taski:%d (schedby:%d)  schedi:%d",
	taski->pipid, taski->task_sched->pipid,
	( schedi != NULL )? schedi->pipid : -1 );

  if( PIP_IS_RUNNING( taski ) ) RETURN( EPERM );

  if( schedi == NULL ) {
    /* resume into the previous scheduling domain */
    schedi = taski->task_sched;
  } else {
    /* get the right task (i.e., scheduling domain) */
    schedi = schedi->task_sched;
    taski->task_sched = schedi;
  }
  DBGF( "Sched PIPID:%d (PID:%d) takes in PIPID:%d (PID:%d)",
	schedi->pipid, schedi->annex->pid, taski->pipid, taski->annex->pid );

  PIP_RUN( taski );
  if( schedi == pip_task->task_sched ) {
    /* same scheduling domain with the current task */
    PIP_TASKQ_ENQ_LAST( &schedi->schedq, PIP_TASKQ(taski) );
    schedi->schedq_len ++;
    DBGF( "PIPID:%d schedq_len:%d", schedi->pipid, schedi->schedq_len );
  } else {			/* otherwise */
    pip_sched_ood_task( schedi, taski );
  }
  RETURN( 0 );
}

/* API */

int pip_get_sched_task_id( int *pipidp ) {
  if( pipidp               == NULL ) RETURN( EINVAL );
  if( pip_task             == NULL ) RETURN( EPERM  );
  if( pip_task->task_sched == NULL ) RETURN( ENOENT );
  *pipidp = pip_task->task_sched->pipid;
  RETURN( 0 );
}

int pip_get_sched_task( pip_task_t **taskp ) {
  if( taskp                == NULL ) RETURN( EINVAL );
  if( pip_task             == NULL ) RETURN( EPERM  );
  if( pip_task->task_sched == NULL ) RETURN( ENOENT );
  *taskp = PIP_TASKQ( pip_task->task_sched );
  RETURN( 0 );
}

int pip_count_runnable_tasks( int *countp ) {
  if( countp == NULL ) RETURN( EINVAL );
  *countp = pip_task->schedq_len;
  RETURN( 0 );
}

int pip_enqueue_runnable_N( pip_task_queue_t *queue, int *np ) {
  /* runnable tasks in the schdq of the pip_task are enqueued into the queue */
  pip_task_internal_t 	*schedi;
  pip_task_t	 	*task, *next, *schedq;
  int			c = 0, n;

  ENTER;
  if( np == NULL ) RETURN( EINVAL );
  schedi = pip_task->task_sched;
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
  DBGF( "PIPID:%d schedq_len:%d", schedi->pipid, schedi->schedq_len );
  *np = c;
  return 0;
}

int pip_suspend_and_enqueue( pip_task_queue_t *queue,
			     pip_enqueue_callback_t callback,
			     void *cbarg ) {
  pip_task_internal_t	*nexti;

  ENTER;
  PIP_SUSPEND( pip_task );
  nexti = pip_schedq_next( pip_task );
  /* pip_scheq_next() protected current stack. and now we can enqueue myself */
  pip_do_suspend( pip_task, nexti, queue, 1, callback, cbarg );
  RETURN( 0 );
}

int pip_suspend_and_enqueue_nolock( pip_task_queue_t *queue,
				    pip_enqueue_callback_t callback,
				    void *cbarg ) {
  pip_task_internal_t	*nexti;

  ENTER;
#ifdef DEBUG
  ASSERT( pip_task_queue_trylock( queue ) );
  pip_task_queue_unlock( queue );
#endif

  PIP_SUSPEND( pip_task );
  nexti = pip_schedq_next( pip_task );
  /* pip_scheq_next() protected current stack. and now we can enqueue myself */
  pip_task_queue_enqueue( queue, PIP_TASKQ( pip_task ) );
  if( callback != NULL ) callback( cbarg );
  pip_do_suspend( pip_task, nexti, queue, 0, callback, cbarg );
  RETURN( 0 );
}

int pip_resume( pip_task_t *task, pip_task_t *sched ) {
  return pip_do_resume( PIP_TASKI(task), PIP_TASKI(sched) );
}

int pip_dequeue_and_resume( pip_task_queue_t *queue, pip_task_t *sched ) {
  ENTER;
  pip_task_t *task;
  pip_task_queue_lock( queue );
  task = pip_task_queue_dequeue( queue );
  pip_task_queue_unlock( queue );
  if( task == NULL ) RETURN( ENOENT );
  RETURN( pip_do_resume( PIP_TASKI(task), PIP_TASKI(sched) ) );
}

int pip_dequeue_and_resume_nolock( pip_task_queue_t *queue,
				   pip_task_t *sched ) {
  ENTER;
#ifdef DEBUG
  ASSERT( pip_task_queue_trylock( queue ) );
  pip_task_queue_unlock( queue );
#endif
  pip_task_t *task;
  task = pip_task_queue_dequeue( queue );
  if( task == NULL ) RETURN( ENOENT );
  RETURN( pip_do_resume( PIP_TASKI(task), PIP_TASKI(sched) ) );
}

int pip_dequeue_and_resume_N( pip_task_queue_t *queue,
			      pip_task_t *sched,
			      int *np ) {
  pip_task_t 	*task;
  int		n, c = 0, err = 0;

  ENTER;
  if( np == NULL ) RETURN( EINVAL );
  n = *np;
  if( n == PIP_TASK_ALL ) {
    pip_task_queue_lock( queue );
    while( !pip_task_queue_isempty( queue ) ) {
      task = pip_task_queue_dequeue( queue );
      err = pip_do_resume( PIP_TASKI(task), PIP_TASKI(sched) );
      if( err ) break;
      c ++;
    }
    pip_task_queue_unlock( queue );

  } else if( n > 0 ) {
    pip_task_queue_lock( queue );
    while( !pip_task_queue_isempty( queue ) && c < n ) {
      task = pip_task_queue_dequeue( queue );
      err = pip_do_resume( PIP_TASKI(task), PIP_TASKI(sched) );
      if( err ) break;
      c ++;
    }
    pip_task_queue_unlock( queue );

  } else if( n < 0 ) {
    err = EINVAL;
  }
  *np = c;
  RETURN( err );
}

int pip_dequeue_and_resume_N_nolock( pip_task_queue_t *queue,
				     pip_task_t *sched,
				     int *np ) {
  pip_task_t 	*task;
  int		n, c = 0, err = 0;

  ENTER;
#ifdef DEBUG
  ASSERT( pip_task_queue_trylock( queue ) );
  pip_task_queue_unlock( queue );
#endif

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

pip_task_t *pip_task_self( void ) { return PIP_TASKQ(pip_task); }

int pip_yield( void ) {
  pip_task_t		*queue, *next;
  pip_task_internal_t	*sched = pip_task->task_sched;

  ENTER;
  queue = &sched->schedq;
  PIP_TASKQ_ENQ_LAST( queue, PIP_TASKQ( pip_task ) );
  next = PIP_TASKQ_NEXT( queue );
  PIP_TASKQ_DEQ( next );
  pip_task_switch( pip_task, PIP_TASKI( next ) );
  RETURN( 0 );
}

int pip_yield_to( pip_task_t *task ) {
  pip_task_internal_t	*taski = PIP_TASKI( task );
  pip_task_internal_t	*sched;
  pip_task_t		*queue;

  IF_UNLIKELY( task  == NULL ) RETURN( pip_yield() );
  sched = pip_task->task_sched;
  /* the target task must be in the same scheduling domain */
  IF_UNLIKELY( taski->task_sched != sched ) RETURN( EPERM );

  queue = &sched->schedq;
  PIP_TASKQ_DEQ( PIP_TASKQ(task) );
  PIP_TASKQ_ENQ_LAST( queue, PIP_TASKQ(pip_task) );

  pip_task_switch( pip_task, taski );
  RETURN( 0 );
}

void pip_set_aux( pip_task_t *task, void *aux ) {
  pip_task_internal_t 	*taski= PIP_TASKI( task );
  if( task == NULL ) taski = pip_task;
  taski->annex->aux = aux;
}

void pip_get_aux( pip_task_t *task, void **auxp ) {
  pip_task_internal_t 	*taski= PIP_TASKI( task );
  if( task == NULL ) taski = pip_task;
  *auxp = taski->annex->aux;
}
