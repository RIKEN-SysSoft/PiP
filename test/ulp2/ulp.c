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

#define PIP_INTERNAL_FUNCS

//#define DEBUG
#include <test.h>
#include <pip_ulp.h>


struct expo {
  pip_task_barrier_t barrier0;
  pip_task_barrier_t barrier1;
  pip_locked_queue_t queue;
} expo;

int main( int argc, char **argv ) {
  struct expo *expop = &expo;
  int ntasks, nulps, nt;
  int i, pipid;

  set_sigsegv_watcher();

  if( argc > 1 ) {
    ntasks = atoi( argv[1] );
  } else {
    ntasks = NTASKS;
  }
  if( ntasks < 2 ) {
    fprintf( stderr, "Too small numebr of tsaks\n" );
    exit( 9 );
  }
  nulps = ( ntasks + 1 ) / 2;
  nt = ntasks;

  TESTINT( pip_init( &pipid, &nt, (void**) &expop, 0 ) );
  printf( "[%d] ntasks:%d  nulps:%d\n", pipid, ntasks, nulps );
  if( pipid == PIP_PIPID_ROOT ) {
    pip_spawn_program_t prog;

    pip_spawn_from_main( &prog, argv[0], argv, NULL );
    pip_task_barrier_init( &expop->barrier0, ntasks );
    pip_task_barrier_init( &expop->barrier1, ntasks - nulps );
    pip_locked_queue_init( &expop->queue );

    for( i=0; i<ntasks; i++ ) {
      pipid = i;
      TESTINT( pip_task_spawn( &prog, PIP_CPUCORE_ASIS, &pipid, NULL ));
    }
    for( i=0; i<ntasks; i++ ) {
      TESTINT( pip_wait( i, NULL ) );
    }
  } else {
    pip_task_barrier_wait( &expop->barrier0 );
    if( pipid < nulps ) {
      fprintf( stderr, "[%d] going to sleep\n", pipid );
      TESTINT( pip_sleep_and_enqueue( &expop->queue, NULL, 0 ) );
      fprintf( stderr, "[%d] wakeup\n", pipid );
    } else {
      pip_task_barrier_wait( &expop->barrier1 );
      while( 1 ) {
	int err = pip_dequeue_and_wakeup( &expop->queue );
	if( err == 0 ) break;
	if( err == ENOENT ) continue;
	if( err != 0 ) {
	  fprintf( stderr, "[%d] pip_dequeue_and_wakeup():%d\n", pipid, err );
	}
      }
      pip_task_barrier_wait( &expop->barrier1 );
      while( 1 ) {
	int err = pip_dequeue_and_wakeup( &expop->queue );
	if( err == 0 || err == ENOENT ) break;
	if( err != 0 ) {
	  fprintf( stderr, "[%d] pip_dequeue_and_wakeup():%d\n", pipid, err );
	}
      }
    }
    fprintf( stderr, "[%d] reching the final barrier\n", pipid );
    pip_task_barrier_wait( &expop->barrier0 );
  }
  TESTINT( pip_fin() );
  return 0;
}
