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
 * Query:   procinproc-info+noreply@googlegroups.com
 * User ML: procinproc-users+noreply@googlegroups.com
 * $
 */

//#define DEBUG
#include <pip/pip_internal.h>

/* BARRIER */

int pip_barrier_init_( pip_barrier_t *barrp, int n ) {
  if( barrp == NULL || n < 1 ) RETURN( EINVAL );
  memset( (void*) barrp, 0, sizeof(pip_barrier_t) );
  if( !pip_is_initialized() ) RETURN( EPERM );
  if( n < 0 ) return EINVAL;
  pip_task_queue_init( &barrp->queue, NULL );
  barrp->count      = n;
  barrp->count_init = n;
  RETURN( 0 );
}

int pip_barrier_wait_( pip_barrier_t *barrp ) {
  pip_task_queue_t *qp = &barrp->queue;
  int init = barrp->count_init;
  int n, c, err = 0;

  ENTERF( "PIPID:%d", TA(pip_task)->pipid );
  IF_UNLIKELY( !pip_is_initialized() ) RETURN( EPERM );
  IF_UNLIKELY( init == 1 ) RETURN( 0 );

  c = pip_atomic_sub_and_fetch( &barrp->count, 1 );
  DBGF( "init:%d:%d c:%d:%d",
	init, (int) barrp->count_init, c, (int) barrp->count );
  ASSERTD( c < 0 );
  IF_LIKELY( c > 0 ) {
    /* noy yet. enqueue the current task */
    err = pip_suspend_and_enqueue( qp, NULL, NULL );

  } else {
    pip_task_queue_t 	queue;
    int 		i;
    /* the last task. we must wait until all tasks are enqueued */
    pip_task_queue_init( &queue, NULL );
    c = init - 1;  /* the last one (myself) is not in the queue */
    for( i=0; i<c; i++ ) {
      pip_task_t	*t;

      while( 1 ) {
	pip_task_queue_lock( qp );
	t = pip_task_queue_dequeue( qp );
	pip_task_queue_unlock( qp );

	IF_LIKELY( t != NULL ) break;
	(void) pip_yield( PIP_YIELD_DEFAULT );
      }
      pip_task_queue_enqueue( &queue, t );
    }
#ifdef DEBUG
    pip_task_queue_describe( &queue, "BARRIER-DONE", stderr );
#endif
    /* really done. dequeue all tasks in the queue and resume them */
    barrp->count = init;
    pip_memory_barrier();
    n = PIP_TASK_ALL;			/* number of tasks to resume */
    err = pip_dequeue_and_resume_N_( &queue, NULL, &n );
    DBGF( "n:%d  c:%d", n, c );
    ASSERTD( n != c );
  }
  RETURN( err );
}

int pip_barrier_fin_( pip_barrier_t *barrp ) {
  ENTERF( "PIPID:%d", TA(pip_task)->pipid );
  if( !pip_is_initialized() ) RETURN( EPERM );
  if( !PIP_TASKQ_ISEMPTY( (pip_task_t*) &barrp->queue ) ) {
    RETURN( EBUSY );
  }
  RETURN( 0 );
}

/* MUTEX */

int pip_mutex_init_( pip_mutex_t *mutex ) {
  ENTERF( "PIPID:%d", TA(pip_task)->pipid );
  if( !pip_is_initialized() ) RETURN( EPERM );
  memset( (void*) mutex, 0, sizeof(pip_mutex_t) );
  RETURN( pip_task_queue_init( &mutex->queue, NULL ) );
}

int pip_mutex_lock_( pip_mutex_t *mutex ) {
  int c, err = 0;

  ENTERF( "PIPID:%d", TA(pip_task)->pipid );
  if( !pip_is_initialized() ) RETURN( EPERM );
  c = pip_atomic_fetch_and_add( &mutex->count, 1 );
  IF_LIKELY( c > 0 ) {
    /* already locked by someone */
    err = pip_suspend_and_enqueue( &mutex->queue, NULL, NULL );
  }
  RETURN( err );
}

int pip_mutex_unlock_( pip_mutex_t *mutex ) {
  int c,  err = 0;

  ENTERF( "PIPID:%d", TA(pip_task)->pipid );
  if( !pip_is_initialized() ) RETURN( EPERM );
  IF_UNLIKELY( mutex->count == 0 ) RETURN( EPERM );
  c = pip_atomic_sub_and_fetch( &mutex->count, 1 );
  while( c > 0 ) {
    err = pip_dequeue_and_resume_( &mutex->queue, NULL );
    IF_LIKELY( err == 0 ) break;
    if( err == ENOENT ) {
      (void) pip_yield( PIP_YIELD_DEFAULT );
    } else {
      break;
    }
  }
  RETURN( err );
}

int pip_mutex_fin_( pip_mutex_t *mutex ) {
  ENTERF( "PIPID:%d", TA(pip_task)->pipid );
  if( !pip_is_initialized() ) RETURN( EPERM );
  if( !pip_task_queue_isempty( (pip_task_t*) &mutex->queue ) ) {
    RETURN( EBUSY );
  }
  RETURN( 0 );
}
