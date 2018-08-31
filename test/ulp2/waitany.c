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

//#define DEBUG

#include <semaphore.h>
#include <test.h>
#include <pip_ulp.h>

struct expo {
  sem_t			semaphore;
  pip_task_barrier_t	barrier;
  pip_locked_queue_t	queue;
} expo;

void wakeup_task( int pipid ) {
  struct expo *expop;

  TESTINT( pip_import( PIP_PIPID_ROOT, (void**) &expop ) );
  TESTINT( sem_post( &expop->semaphore ) );
}

int my_ulp_main( void ) {
  struct expo *expop;
  int pipid;

  TESTINT( pip_init( &pipid, NULL, (void**) &expop, 0 ) );
  TESTINT( pip_task_barrier_wait( &expop->barrier ) );
  TESTINT( pip_sleep_and_enqueue( &expop->queue, wakeup_task, 0 ) );
  return pipid;
}

int my_tsk_main( void ) {
  struct expo *expop;
  int pipid;

  TESTINT( pip_init( &pipid, NULL, (void**) &expop, 0 ) );
  TESTINT( pip_ulp_dequeue_and_involve( &expop->queue, NULL, 0 ) );
  TESTINT( pip_task_barrier_wait( &expop->barrier ) );
  TESTINT( pip_ulp_yield() );
  return pipid;
}

int main( int argc, char **argv ) {
  struct expo 	       *expop = &expo;
  pip_spawn_program_t	prog_ulp, prog_tsk;
  int			ntasks=0, pipid, i, extval=0;

  if( argc == 1 ) {
    ntasks = NTASKS;
  } else {
    if( argc > 1 ) ntasks = atoi( argv[1] );
  }
  ntasks = ( ntasks / 2 ) * 2;
  if( ntasks == 0 ) {
      fprintf( stderr, "Not enough number of tasks\n" );
      exit( 1 );
  }
  TESTINT( sem_init( &expop->semaphore, 1, 0 ) );
  TESTINT( pip_task_barrier_init ( &expop->barrier, ntasks / 2 ) );
  TESTINT( pip_locked_queue_init( &expop->queue ) );

  pip_spawn_from_func( &prog_ulp, argv[0], "my_ulp_main", NULL, NULL );
  pip_spawn_from_func( &prog_tsk, argv[0], "my_tsk_main", NULL, NULL );

  TESTINT( pip_init( NULL, &ntasks, (void**) &expop, 0 ) );

  for( i=0; i<ntasks/2; i++ ) {
    pipid = i;
    TESTINT( pip_task_spawn( &prog_ulp, PIP_CPUCORE_ASIS, &pipid, NULL ));
  }
  for( i=0; i<ntasks/2; i++ ) {
    TESTINT( sem_wait( &expop->semaphore ) );
  }
  for( i=ntasks/2; i<ntasks; i++ ) {
    pipid = i;
    TESTINT( pip_task_spawn( &prog_tsk, PIP_CPUCORE_ASIS, &pipid, NULL ));
  }

  for( i=0; i<ntasks; i++ ) {
    int status;
    TESTINT( pip_wait_any( &pipid, &status ) );
    if( status != ( pipid & 0xFF ) ) {
      fprintf( stderr, "[%d] pip_wait_any(%d):%d -- FAILED\n",
	       pipid, i, status );
      extval = 9;
    }
  }
  TESTINT( pip_fin() );
  if( !extval ) fprintf( stderr, "SUCCEEDED\n" );
  return 0;
}
