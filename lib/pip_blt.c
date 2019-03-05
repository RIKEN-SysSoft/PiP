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

#include <sys/mman.h>
#include <sched.h>
#include <signal.h>
#include <unistd.h>

#include <pip_internal.h>
#include <pip_blt.h>

extern void pip_deadlock_inc_( void );
extern void pip_deadlock_dec_( void );

#ifdef DEBUG
#define QUEUE_DUMP( queue, len )			\
  if( PIP_TASKQ_ISEMPTY( queue ) ) {			\
    DBGF( "EMPTY" );					\
  } else {						\
    pip_task_t *task;					\
    char msg[128];					\
    int i = 0;						\
    PIP_TASKQ_FOREACH( queue, task ) {			\
      pip_task_queue_brief( task, msg, sizeof(msg) );	\
      DBGF( "Q(%d:%d): %s", i++, len, msg );		\
    }							\
  }


#define TASK_QUEUE_DUMP( queue )			\
  if( pip_task_queue_isempty( queue ) ) {		\
    DBGF( "EMPTY" );					\
  } else {						\
    pip_task_t *task;					\
    char msg[128];					\
    int i = 0, c;					\
    pip_task_queue_count( queue, &c );			\
    PIP_TASKQ_FOREACH( &queue->queue, task ) {		\
      pip_task_queue_brief( task, msg, sizeof(msg) );	\
      DBGF( "TQ(%d:%d): %s", i++, c, msg );		\
    }							\
  }

#else
#define QUEUE_DUMP( queue, len )
#define TASK_QUEUE_DUMP( queue )
#endif

static int pip_raise_sigchld( void ) {
  extern int pip_raise_signal_( pip_task_internal_t*, int );
  RETURN( pip_raise_signal_( pip_root_->task_root, SIGCHLD ) );
}

static void pip_force_exit( pip_task_internal_t *taski ) {
  extern int pip_is_threaded_( void );

  DBGF( "PIPID:%d  pid:%d(%d)",
	taski->pipid, taski->annex->pid, pip_gettid() );
  if( 0 ) {
    int i, sv = 0;
    (void) sem_getvalue( &taski->annex->sleep, &sv );
    DBGF( "SV: %d", sv );
    for( i=0; i<sv; i++ ) (void) sem_post( &taski->annex->sleep );
    (void) sem_destroy( &taski->annex->sleep );
  }
  if( pip_is_threaded_() ) {
    (void) pip_raise_sigchld();
    pthread_exit( NULL );
  } else {			/* process mode */
    ASSERT( taski->annex->pid != pip_gettid() );
    exit( taski->annex->extval );
  }
  NEVER_REACH_HERE;
}

static void pip_load_context_( pip_ctx_t *ctxp ) {
  ASSERT( ctxp == NULL );
  ASSERT( pip_load_context( ctxp ) != 0 );
}

static void pip_wakeup( pip_task_internal_t *taski ) {
  DBGF( "%d[%d]: schedq_len:%d  oodq_len:%d  ntakecare:%d  "
	"flag_semwait:%d  flag_exit:%d",
	taski->pipid, taski->task_sched->pipid,
	taski->schedq_len, (int) taski->oodq_len,
	(int) taski->ntakecare, taski->flag_semwait,
	taski->flag_exit );
  taski->flag_wakeup = 1;
  if( taski->flag_semwait ) {
    taski->flag_semwait = 0;
    /* not to call sem_post() more than once per blocking */
    DBGF( "WAKEUP vvvvvvvvvv PIPID:%d", taski->pipid );
    errno = 0;
    sem_post( &taski->annex->sleep );
    ASSERT( errno );
  }
}

static void pip_change_sched( pip_task_internal_t *taski,
			      pip_task_internal_t *schedi ) {

  DBGF( "taski:%d[%d]-NTC:%d  schedi:%d[%d]-NTC:%d",
	taski->pipid,
	taski->task_sched->pipid,
	(int) taski->task_sched->ntakecare,
	schedi->pipid,
	schedi->task_sched->pipid,
	(int) schedi->ntakecare );
  if( taski->task_sched != schedi ) {
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

static int pip_takein_ood_task( pip_task_internal_t *schedi ) {
  pip_task_annex_t	*annex;
  int		 	oodq_len = schedi->oodq_len;

  if( oodq_len == 0 ) {
    DBGF( "oodq is empty" );
    return 0;
  }
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
  return oodq_len;
}

static void pip_stack_set_unprotect( pip_task_internal_t *curr,
				     pip_task_internal_t *new ) {
  DBGF( "PIPID:%d protected, and will be unprotected by PIPID:%d",
	curr->pipid, new->pipid );
#ifdef DEBUG
  if( new->flag_stackpp ) {
    DBGF( "PIPID:%d is already protected by PIPID:%d "
	  "and wait until unprotected",
	  new->pipid,
	  ((pip_task_internal_t*)
	   (new->flag_stackpp -
	    offsetof(pip_task_internal_t,flag_stackpp)))->pipid );
  }
#endif
  if( curr != new ) {
    while( new->flag_stackpp != NULL ) pip_pause();
  }
  new->flag_stackpp = &curr->flag_stackp; /* next task is scheduled */
}

static void pip_stack_unprotect_prevt( pip_task_internal_t *taski ) {
  /* this function musrt be called everytime a context is switched */
  if( taski->flag_stackpp != NULL ) {
    DBGF( "PIPID:%d unprotecting PIPID:%d", taski->pipid,
	  ((pip_task_internal_t*)
	   (taski->flag_stackpp -
	    offsetof(pip_task_internal_t,flag_stackp)))->pipid );
    *taski->flag_stackpp = 0;
    taski->flag_stackpp  = NULL;
  } else {
    DBGF( "pipid:%d  <<<NULL>>>", taski->pipid );
  }
}

static void pip_stack_wait( pip_task_internal_t *taski ) {
  int pip_stack_isbusy( pip_task_internal_t *taski ) {
    //return taski->flag_stackp || taski->flag_sleep;
    return taski->flag_stackp;
  }

  if( pip_stack_isbusy( taski ) ) {
    int i;
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
#ifdef DEBUG
      int j;
      for( j=0; j<PIP_BUSYWAIT_COUNT; j++ ) {
	if( !pip_stack_isbusy( taski ) ) goto done;
	pip_pause();
      }
#endif
      DBGF( "YILED-COUNT:%d", i );
    }
  }
 done:
  DBGF( "WAIT-DONE  pipid:%d", taski->pipid );
  return;
}

static void pip_task_sched( pip_task_internal_t *taski ) {
  pip_stack_wait( taski );	/* wait until context is free to use */
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
  pip_task_internal_t	*schedi;
  pip_task_internal_t	*taski;	/* task's stack will be unprotected */
} pip_sleep_args_t;

static void pip_sleep( intptr_t task_H, intptr_t task_L ) {
  void pip_block_myself( pip_task_internal_t *taski ) {
    ENTER;
    DBGF( "flag_wakeup:%d  flag_exit:%d",
	  taski->flag_wakeup, taski->flag_exit );
    taski->flag_semwait = 1;
    while( !taski->flag_wakeup && !taski->flag_exit ) {
      errno = 0;
      if( sem_wait( &taski->annex->sleep ) == 0 ) continue;
      if( errno != EINTR ) ASSERT( errno );
    }
    taski->flag_semwait = 0;
    RETURNV;
  }

  pip_sleep_args_t 	*args = (pip_sleep_args_t*)
    ( ( ((intptr_t) task_H) << 32 ) | ( ((intptr_t) task_L) & PIP_MASK32 ) );
  pip_task_internal_t	*taski  = args->taski;
  pip_task_internal_t	*schedi = args->schedi;
  int			i;

  ENTER;
  //ASSERT( taski != schedi );
  DBGF( "taski:%d  schedi:%d",
	(taski!=NULL)?taski->pipid:-1, schedi->pipid );
  /* now the stack has been switched and one may swicth to the prev. stack */
  if( taski != NULL ) taski->flag_stackp = 0;	/* unprotect stack */

  while( 1 ) {
    (void) pip_takein_ood_task( schedi );
#ifdef DEBUG
    if( !schedi->flag_exit ) {
      DBGF( "PIPID:%d schedq_len:%d", schedi->pipid, schedi->schedq_len );
    } else {
      DBGF( "PIPID:%d schedq_len:%d  ntakecare:%d -- EXIT",
	    schedi->pipid, schedi->schedq_len, (int) schedi->ntakecare );
    }
#endif
    if( !PIP_TASKQ_ISEMPTY( &schedi->schedq ) ) break;

    if( schedi->flag_exit && !schedi->ntakecare ) {
      DBGF( "WOKEUP and EXIT" );
      pip_force_exit( schedi );
      NEVER_REACH_HERE;
    }

    pip_deadlock_inc_();
    {
      switch( schedi->annex->opts & PIP_SYNC_MASK ) {
      case PIP_SYNC_BLOCKING:
	DBGF( "SLEEPING (blocking) zzzzzzz" );
	pip_block_myself( schedi );
	break;
      case PIP_SYNC_BUSYWAIT:
	DBGF( "SLEEPING (busywait) zzzzzzzz" );
	while( !schedi->flag_wakeup ) pip_pause();
	break;
      case PIP_SYNC_YIELD:
	DBGF( "SLEEPING (yield) zzzzzzzz" );
	while( !schedi->flag_wakeup ) sched_yield();
	break;
      case PIP_SYNC_AUTO:
      default:
	for( i=0; i<PIP_BUSYWAIT_COUNT; i++ ) {
	  pip_pause();
	  if( schedi->flag_wakeup ) {
	    DBGF( "SLEEPING (blocking-busywait:%d)", i );
	    goto done;
	  }
	}
	DBGF( "SLEEPING (blocking) zzzzzzz" );
	pip_block_myself( schedi );
      done:
	break;
      }
    }
    pip_deadlock_dec_();
    schedi->flag_wakeup = 0;
  }
  pip_change_sched( schedi, schedi );

  DBGF( "WOKEUP ^^^^^^^^" );
  ASSERT( schedi->schedq_len == 0 );
  ASSERT( PIP_TASKQ_ISEMPTY( &schedi->schedq ) );
  {
    pip_task_t *next = PIP_TASKQ_NEXT( &schedi->schedq );
    pip_task_internal_t *nexti = PIP_TASKI( next );
    ASSERT( next == NULL );
    PIP_TASKQ_DEQ( next );
    schedi->schedq_len --;
    DBGF( "PIPID:%d(ctxp:%p) -->> PIPID:%d(CTXP:%p)",
	  schedi->pipid, schedi->ctx_suspend,
	  nexti->pipid,  nexti->ctx_suspend );
    pip_task_sched( nexti );
  }
  NEVER_REACH_HERE;
}

#define STACK_PG_NUM	(16)

static void pip_switch_stack_and_sleep( pip_task_internal_t *schedi,
					pip_task_internal_t *taski ) {
  extern void pip_page_alloc_( size_t, void** );
  pip_sleep_args_t	args;
  size_t	pgsz  = pip_root_->page_size;
  size_t	stksz = STACK_PG_NUM * pgsz;
  pip_ctx_t	ctx;
  stack_t	*stk;
  void		*stack, *redzone;
  int 		args_H, args_L;

  ENTER;
  DBGF( "sched-PIPID:%d  taski-PIPID:%d",
	schedi->pipid, (taski!=NULL)?taski->pipid:-1 );
  if( ( stack = schedi->annex->sleep_stack ) == NULL ) {
    pip_page_alloc_( stksz + pgsz, &stack );
    redzone = stack + stksz;
    ASSERT( mprotect( redzone, pgsz, PROT_NONE ) );
    schedi->annex->sleep_stack = stack;
  }
  /* pass taski, not schedi, so that stack can be unprotected in pip_sleep */
  args.schedi = schedi;
  args.taski  = taski;
  /* creating new context to switch stack */
  pip_save_context( &ctx );

  ctx.ctx.uc_link = NULL;
  stk = &(ctx.ctx.uc_stack);
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
    DBGF( "PIPID:%d(ctxp:%p) <<==>> PIPID:%d(CTXP:%p)",
	  old_task->pipid, old_task->ctx_suspend,
	  new_task->pipid, new_task->ctx_suspend );

    pip_stack_wait( new_task );
    old_task->flag_stackp = 1;			/* protect current */
    pip_stack_set_unprotect( old_task, new_task );
#ifdef PIP_LOAD_TLS
    ASSERT( pip_load_tls( new_task->tls ) );
#endif
    ASSERT( pip_swap_context( old_task->ctx_suspend,
			      new_task->ctx_suspend ) );
    pip_stack_unprotect_prevt( old_task );
  }
}

static pip_task_internal_t *pip_schedq_next( pip_task_internal_t *taski ) {
  pip_task_internal_t 	*schedi = taski->task_sched;
  pip_task_internal_t 	*nexti;
  pip_task_t		*next;

  ASSERT( schedi == NULL );
  DBGF( "sched-PIPID:%d", schedi->pipid );
  (void) pip_takein_ood_task( schedi );
  if( !PIP_TASKQ_ISEMPTY( &schedi->schedq ) ) {
    QUEUE_DUMP( &schedi->schedq, schedi->schedq_len );
    next = PIP_TASKQ_NEXT( &schedi->schedq );
    PIP_TASKQ_DEQ( next );
    schedi->schedq_len --;
    ASSERT( schedi->schedq_len < 0 );
    nexti = PIP_TASKI( next );
    DBGF( "next-PIPID:%d", nexti->pipid );

  } else {
    nexti = NULL;
    DBGF( "next:NULL" );
  }
  return nexti;
}

typedef struct pip_queue_info {
  pip_task_queue_t	*queue;
  pip_enqueue_callback_t callback;
  void			*cbarg;
  int 			flag_lock;
} pip_queue_info_t;

static void pip_sched_next( pip_task_internal_t *taski,
			    pip_task_internal_t *nexti,
			    pip_queue_info_t *qip ) {
  void pip_enqueue_qi( pip_task_t *task, pip_queue_info_t *qip ) {
    pip_task_queue_t	*queue = qip->queue;

    if( qip->flag_lock ) {
      pip_task_queue_lock( queue );
      TASK_QUEUE_DUMP( queue );
      pip_task_queue_enqueue( queue, task );
      TASK_QUEUE_DUMP( queue );
      pip_task_queue_unlock( queue );
    } else {
      TASK_QUEUE_DUMP( queue );
      pip_task_queue_enqueue( queue, task );
      TASK_QUEUE_DUMP( queue );
    }
    if( qip->callback != NULL ) qip->callback( qip->cbarg );
  }
  pip_task_internal_t	*schedi = taski->task_sched;
  pip_ctx_t		*ctxp;
  volatile int 		flag_jump = 0;	/* must be volatile */

  ENTER;
  PIP_SUSPEND( taski );

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

    taski->flag_stackp = 1;	/* protect current stack before enqueue */
    if( qip != NULL ) {
      /* enqueue */
      pip_enqueue_qi( PIP_TASKQ(taski), qip );
    }
    if( nexti != NULL ) {
      /* context-switch to the next task */
      DBGF( "PIPID:%d(ctxp:%p) -->> PIPID:%d(CTXP:%p)",
	    taski->pipid, taski->ctx_suspend,
	    nexti->pipid, nexti->ctx_suspend );
      pip_stack_set_unprotect( taski, nexti ); /* next task unprotect me */
      pip_task_sched_with_tls( nexti );

    } else {
      /* no task to be scheduled next, and block the scheduling task */
      pip_switch_stack_and_sleep( schedi, taski );
    }
    NEVER_REACH_HERE;
  }
  pip_stack_unprotect_prevt( taski );
}

static void pip_task_terminate( pip_task_internal_t *taski, int extval ) {
  extern void pip_do_terminate_( pip_task_internal_t*, int );
  int ntc;

  pip_do_terminate_( taski, extval );
  ntc = pip_atomic_sub_and_fetch( &taski->task_sched->ntakecare, 1 );
  ASSERT( ntc < 0 );
  if( ntc == 0 ) pip_wakeup( taski->task_sched );
  DBGF( "%d[%d] - NTC:%d", taski->pipid, taski->task_sched->pipid, ntc );
}

void pip_do_exit( pip_task_internal_t *taski, int extval ) {
  pip_task_internal_t	*schedi, *nexti;
  /* this is called when 1) return from main (or func) function */
  /*                     2) pip_exit() is explicitly called     */
  ENTER;
  DBGF( "PIPID:%d", taski->pipid );

  pip_task_terminate( taski, extval );
  schedi = taski->task_sched;
  ASSERT( schedi == NULL );
  DBGF( "TASKI: %d[%d]",  taski->pipid,  taski->task_sched->pipid );
  DBGF( "SCHEDI: %d[%d]", schedi->pipid, schedi->task_sched->pipid );

  if( schedi != taski ) {
    /* wake up to terminate */
    pip_wakeup( taski );
  }

  if( ( nexti = pip_schedq_next( taski ) ) != NULL ) {
    DBGF( "NEXT %d[%d]", nexti->pipid, nexti->task_sched->pipid );
    pip_sched_next( taski, nexti, NULL );
  } else {
    DBGF( "No sched task" );
    DBGF( "schedi->ntakecare:%d", (int) schedi->ntakecare );
#ifdef AH
    if( schedi->ntakecare > 0 ) {
      pip_sched_next( schedi, NULL, NULL ); /* sleep */
    }
#else
    pip_switch_stack_and_sleep( schedi, NULL );
#endif
  }
  /* and termintae myself */
  pip_force_exit( taski );
  NEVER_REACH_HERE;
}

static void pip_do_suspend( pip_task_internal_t *taski,
			    pip_task_internal_t *nexti,
			    pip_queue_info_t	*qip ) {
  pip_sched_next( taski, nexti, qip );
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
  DBGF( "Sched PIPID:%d (PID:%d) takes in PIPID:%d (PID:%d)",
	schedi->pipid, schedi->annex->pid, taski->pipid, taski->annex->pid );

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

int pip_get_sched_task_id( int *pipidp ) {
  if( pipidp                == NULL ) RETURN( EINVAL );
  if( pip_task_             == NULL ) RETURN( EPERM  );
  if( pip_task_->task_sched == NULL ) RETURN( ENOENT );
  *pipidp = pip_task_->task_sched->pipid;
  RETURN( 0 );
}

int pip_get_sched_task( pip_task_t **taskp ) {
  if( taskp                 == NULL ) RETURN( EINVAL );
  if( pip_task_             == NULL ) RETURN( EPERM  );
  if( pip_task_->task_sched == NULL ) RETURN( ENOENT );
  *taskp = PIP_TASKQ( pip_task_->task_sched );
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

int pip_suspend_and_enqueue( pip_task_queue_t *queue,
			     pip_enqueue_callback_t callback,
			     void *cbarg ) {
  pip_task_internal_t	*nexti;
  pip_queue_info_t	qi;

  ENTER;
  nexti = pip_schedq_next( pip_task_ );
  qi.queue     = queue;
  qi.flag_lock = 1;
  qi.callback  = callback;
  qi.cbarg     = cbarg;
  /* pip_scheq_next() protected current stack. and now we can enqueue myself */
  pip_do_suspend( pip_task_, nexti, &qi );
  RETURN( 0 );
}

int pip_suspend_and_enqueue_nolock( pip_task_queue_t *queue,
				    pip_enqueue_callback_t callback,
				    void *cbarg ) {
  pip_task_internal_t	*nexti;
  pip_queue_info_t	qi;

  ENTER;
#ifdef DEBUG
  ASSERT( pip_task_queue_trylock( queue ) );
  pip_task_queue_unlock( queue );
#endif

  nexti = pip_schedq_next( pip_task_ );
  qi.queue     = queue;
  qi.flag_lock = 0;
  qi.callback  = callback;
  qi.cbarg     = cbarg;
  /* pip_scheq_next() protected current stack. and now we can enqueue myself */
  pip_do_suspend( pip_task_, nexti, &qi );
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

pip_task_t *pip_task_self( void ) { return PIP_TASKQ(pip_task_); }

int pip_yield( void ) {
  pip_task_t		*queue, *next;
  pip_task_internal_t	*sched = pip_task_->task_sched;

  ENTER;
  queue = &sched->schedq;
  PIP_TASKQ_ENQ_LAST( queue, PIP_TASKQ( pip_task_ ) );
  next = PIP_TASKQ_NEXT( queue );
  PIP_TASKQ_DEQ( next );
  pip_task_switch( pip_task_, PIP_TASKI( next ) );
  RETURN( 0 );
}

int pip_yield_to( pip_task_t *task ) {
  pip_task_internal_t	*taski = PIP_TASKI( task );
  pip_task_internal_t	*sched;
  pip_task_t		*queue;

  IF_UNLIKELY( task  == NULL ) RETURN( pip_yield() );
  sched = pip_task_->task_sched;
  /* the target task must be in the same scheduling domain */
  IF_UNLIKELY( taski->task_sched != sched ) RETURN( EPERM );

  queue = &sched->schedq;
  PIP_TASKQ_DEQ( PIP_TASKQ(task) );
  PIP_TASKQ_ENQ_LAST( queue, PIP_TASKQ(pip_task_) );

  pip_task_switch( pip_task_, taski );
  RETURN( 0 );
}

void pip_set_aux( pip_task_t *task, void *aux ) {
  pip_task_internal_t 	*taski= PIP_TASKI( task );
  if( task == NULL ) taski = pip_task_;
  taski->annex->aux = aux;
}

void pip_get_aux( pip_task_t *task, void **auxp ) {
  pip_task_internal_t 	*taski= PIP_TASKI( task );
  if( task == NULL ) taski = pip_task_;
  *auxp = taski->annex->aux;
}
