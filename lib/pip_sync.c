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

#include <pip_internal.h>

/* BARRIER */

int pip_barrier_init_( pip_barrier_t *barrp, int n ) {
  if( barrp == NULL || n < 1 ) RETURN( EINVAL );
  memset( (void*) barrp, 0, sizeof(pip_barrier_t) );
  pip_task_queue_init( &barrp->queue, NULL );
  barrp->head.count      = n;
  barrp->head.count_init = n;
  RETURN( 0 );
}

int pip_barrier_wait_( pip_barrier_t *barrp ) {
  int n, c, err = 0;

  ENTER;
  IF_UNLIKELY( barrp->head.count_init == 1 ) RETURN( 0 );
  c = pip_atomic_sub_and_fetch( &barrp->head.count, 1 );
  DBGF( "c:%d", c );
  IF_UNLIKELY( c > 0 ) {
    /* noy yet. enqueue the current task */
    err = pip_suspend_and_enqueue( &barrp->queue, NULL, NULL );
    DBG;
  } else {
    /* done. dequeue all tasks is=n the queue and resume them */
    DBG;
    do {
      pip_task_queue_count( &barrp->queue, &c );
      pip_pause();
    } while ( c < barrp->head.count_init - 1 );
    barrp->head.count = barrp->head.count_init;
    DBG;

    c = barrp->head.count_init - 1; /* the last one is not in the queue */
    do {
      n = c;			/* number of rests to go */
      DBGF( "n:%d", n );
      err = pip_dequeue_and_resume_N( &barrp->queue, NULL, &n );
      if( err ) break;
      c -= n;
    } while( c > 0 );
  }
  RETURN( err );
}

int pip_barrier_fin_( pip_barrier_t *barrp ) {
  if( !PIP_TASKQ_ISEMPTY( &barrp->queue.queue ) ) RETURN( EBUSY );
  RETURN( 0 );
}

/* MUTEX */

int pip_mutex_init_( pip_mutex_t *mutex ) {
  ENTER;
  memset( (void*) mutex, 0, sizeof(pip_mutex_t) );
  RETURN( pip_task_queue_init( &mutex->queue, NULL ) );
}

int pip_mutex_lock_( pip_mutex_t *mutex ) {
  int err = 0;
  ENTER;
  if( pip_comp2_and_swap( &mutex->head.lock, 0, 1 ) ) {
    /* already locked by someone */
    err = pip_suspend_and_enqueue( &mutex->queue, NULL, NULL );
  }
  RETURN( err );
}

int pip_mutex_unlock_( pip_mutex_t *mutex ) {
  int err = 0;
  ENTER;
  err = pip_dequeue_and_resume( &mutex->queue, NULL );
  if( err == ENOENT ) {	       /* the queue is empty, simply unlock */
    mutex->head.lock = 0; /* otherwise resume the first one in the queue */
    RETURN( 0 );
  } else if( err ) {
    RETURN( err );
  }
  RETURN( 0 );
}

int pip_mutex_fin_( pip_mutex_t *mutex ) {
  if( !PIP_TASKQ_ISEMPTY( &mutex->queue.queue ) ) RETURN( EBUSY );
  RETURN( 0 );
}
