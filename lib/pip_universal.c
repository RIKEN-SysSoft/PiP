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
  * Written by Atsushi HORI <ahori@riken.jp>, 2018
*/

#ifdef DEBUG
//#undef DEBUG
#endif

#include <pip.h>
#include <pip_ulp.h>
#include <pip_internal.h>
#include <pip_universal.h>
#include <pip_util.h>

/* barrier */

int pip_universal_barrier_init( pip_universal_barrier_t *ubarr, int n ) {
  int err = 0;

  if( ubarr == NULL        ) RETURN( EINVAL );
  if( n < 1                ) RETURN( EINVAL );

  if( sem_init( &ubarr->semaphore[0], 0, 0 ) != 0 ||
      sem_init( &ubarr->semaphore[1], 0, 0 ) != 0 ) {
    err = errno;
  } else {
    pip_spin_init( &ubarr->lock  );
    PIP_LIST_INIT( &ubarr->queue[0] );
    PIP_LIST_INIT( &ubarr->queue[1] );
    ubarr->count_init = n;
    ubarr->count_barr = n;
    ubarr->turn       = 0;
    ubarr->count_sem  = 0;
  }
  RETURN( err );
}

int pip_universal_barrier_wait( pip_universal_barrier_t *ubarr ) {
  pip_task_t 	*sched = pip_task->task_sched;;
  pip_ulp_t 	*ulp, *next;
  int		t, err = 0;

  if( ubarr== NULL ) RETURN( EINVAL );
  if( ubarr->count_init == 1 ) return 0;

  pip_spin_lock( &ubarr->lock );
  {
    DBGF( ">> count_barr:%d  count_sem:%d  turn=%d",
	  ubarr->count_barr, ubarr->count_sem, ubarr->turn );
    t = ubarr->turn;
    if( -- ubarr->count_barr == 0 ) { /* barrier done */
      int count = ubarr->count_sem;

      ubarr->count_sem  = 0;
      ubarr->count_barr = ubarr->count_init;
      ubarr->turn       = t ^ 1;
      pip_memory_barrier();
      //ubarr->gsense = lsense;
      while( count++ < 0 ) {
	DBGF( "sem_post (%d)", count );
	if( sem_post( &ubarr->semaphore[t] ) != 0 ) {
	  err = errno;
	  goto error;
	}
      }

    } else if( PIP_ULP_ISEMPTY( &sched->schedq ) ) {
      /* wait on semaphore */
      ubarr->count_sem --;
      pip_spin_unlock( &ubarr->lock );
      DBGF( ">> sem_wait[%p,%p,%d]", ubarr, &ubarr->semaphore[t], t );
      while( 1 ) {
	if( sem_wait( &ubarr->semaphore[t] ) == 0 ) break;
	if( errno != EINTR ) {
	  err = errno;
	  break;
	}
      }
      DBGF( "<< sem_wait" );
      pip_spin_lock( &ubarr->lock );
    } else {	/* switch to the other ULP */
      pip_task->task_resume = sched;
      PIP_LIST_ADD( &ubarr->queue[t], PIP_ULP( pip_task ) );
      pip_spin_unlock( &ubarr->lock );
      DBGF( ">> ulp_suspend" );
      err = pip_ulp_suspend();
      DBGF( "<< ulp_suspend()=%d", err );
      pip_spin_lock( &ubarr->lock );
      goto done;
    }
  }
  if( err == 0 ) {
    DBGF( "resume ulps" );
    PIP_ULP_QUEUE_DESCRIBE( &ubarr->queue[t] );
    PIP_LIST_FOREACH_SAFE( &ubarr->queue[t], ulp, next ) {
      if( PIP_TASK(ulp)->task_resume == pip_task->task_sched ) {
	PIP_LIST_DEL( ulp );
	err = pip_ulp_resume( ulp, 1 );
	DBGF( "pip_ulp_resume(PIPID:%d)=%d", PIP_TASK(ulp)->pipid, err );
	if( err ) break;
      }
    }
  }
 error:
 done:
  DBGF( "<< count_barr:%d  count_sem:%d  turn=%d",
	ubarr->count_barr, ubarr->count_sem, ubarr->turn );
  pip_spin_unlock( &ubarr->lock );
  RETURN( err );
}

int pip_universal_barrier_fin( pip_universal_barrier_t *ubarr ) {
  int 	err = 0;
  if( sem_destroy( &ubarr->semaphore[0] ) != 0 ||
      sem_destroy( &ubarr->semaphore[1] ) != 0 ) err = errno;
  RETURN( err );
}
