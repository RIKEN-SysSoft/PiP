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
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#define PIP_INTERNAL_FUNCS

//#define NITERS	(1000*1000)
#define NITERS	(1000)
//#define NITERS	(2)

//#define DEBUG

#include <sched.h>
#include <test.h>

struct expo {
  pip_task_barrier_t	barr;
  pip_locked_queue_t	queue;
} expo;

void  __attribute__ ((noinline))
sleep_and_enqueue( int n, pip_locked_queue_t *qp ) {
  /* grow stack intentionally */
  if( n == 0 ) {
    TESTINT( pip_sleep_and_enqueue( qp, NULL, 0 ) );
  } else {
    sleep_and_enqueue( n-1, qp );
  }
}

int main( int argc, char **argv ) {
  struct expo		*expop = &expo;
  pip_task_barrier_t	*barrp = &expop->barr;
  pip_locked_queue_t	*qp    = &expop->queue;
  int i, pipid, core;
  int ntasks = 2;

  set_sigsegv_watcher();

  TESTINT( pip_init( &pipid, &ntasks, (void**) &qp, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    pip_spawn_program_t prog;

    pip_spawn_from_main( &prog, argv[0], argv, NULL );
    TESTINT( pip_task_barrier_init( barrp, 2 ) );
    TESTINT( pip_locked_queue_init( qp ) );

    pipid = core =0;
    TESTINT( pip_task_spawn( &prog, core, &pipid, NULL ) );
    pipid = core = 1;
    TESTINT( pip_task_spawn( &prog, core, &pipid, NULL ) );

    TESTINT( pip_wait( 0, NULL ) );
    TESTINT( pip_wait( 1, NULL ) );
  } else {
    pip_task_barrier_wait( barrp );
    double tm = -pip_gettime();
    if( pipid == 0 ) {
      for( i=0; i<NITERS; i++ ) {
	sleep_and_enqueue( i, qp );
//fprintf( stderr, "sleep_and_enqueue(%d)\n", i );
      }
    } else {
      for( i=0; i<NITERS; i++ ) {
	while( 1 ) {
	  int err = pip_dequeue_and_wakeup( qp );
	  if( err == ENOENT ) {
	    continue;
	  } else {
	    TESTINT( err );
	    break;
	  }
	}
//fprintf( stderr, "dequeue_and_wakeup(%d)\n", i );
      }
    }
    pip_task_barrier_wait( barrp );
    tm += pip_gettime();
    fprintf( stderr, "[%d] %g [S*%d]  %g [usec]\n",
	     pipid, tm, NITERS, tm / (double) NITERS * 1e6 );
  }
  TESTINT( pip_fin() );
  return 0;
}
