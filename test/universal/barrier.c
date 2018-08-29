
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
#include <pip.h>
#include <pip_ulp.h>
#include <pip_universal.h>

#define NITERS	(100)
//#define NITERS	(10)
#define NULPS	(5)

struct expo {
  pip_locked_queue_t		queue;
  pip_task_barrier_t		pbarr;
  pip_universal_barrier_t	ubarr;
  sem_t				*sem_array;
  int				*count;
  int				error;
} expo;

int ntasks = 0;
int nulps  = 0;
int pipid;

void wakeup_task( int id ) {
  struct expo *expop;
  int nt = id / nulps;

  TESTINT( pip_import( PIP_PIPID_ROOT, (void**) &expop ) );
  TESTINT( sem_post( &expop->sem_array[nt] ) );
}

int main( int argc, char **argv, char **envv ) {
  struct expo *expop = &expo;
  int total, niters=0, i;

  if( argc == 1 ) {
    nulps  = NULPS;
    ntasks = NTASKS / nulps;
  } else {
    switch( argc ) {
    default:
    case 4:
      niters = atoi( argv[3] );
    case 3:
      nulps  = atoi( argv[2] );
    case 2:
      ntasks = atoi( argv[1] );
      if( nulps == 0 ) nulps  = NTASKS / ntasks;
      break;
    case 1:
      nulps  = NULPS;
      ntasks = NTASKS / nulps;
      break;
    }
  }
  if( ntasks == 0 ) {
      fprintf( stderr, "Not enough number of tasks\n" );
      exit( 1 );
  }
  if( nulps == 0 ) {
      fprintf( stderr, "Not enough number of ULPs\n" );
      exit( 1 );
  }
  total = ntasks * nulps;
  if( total > NTASKS ) {
    fprintf( stderr, "Too large\n" );
    exit( 1 );
  }
  if( niters == 0 ) niters = NITERS;

  TESTINT( pip_init( &pipid, &total, (void**) &expop, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    pip_spawn_program_t prog;

    TESTINT( pip_locked_queue_init(      &expop->queue         ) );
    TESTINT( pip_task_barrier_init(      &expop->pbarr, ntasks ) );
    TESTINT( pip_universal_barrier_init( &expop->ubarr, total  ) );
    expop->sem_array = (sem_t*) malloc( sizeof(sem_t) * ntasks );
    expop->count     = (int*)   malloc( sizeof(int)   * total  );
    TESTINT( expop->sem_array == NULL || expop->count == NULL );
    for( i=0; i<ntasks; i++ ) TESTINT( sem_init( &expop->sem_array[i], 0, 0 ));
    for( i=0; i<total;  i++ ) expop->count[i] = 0;
    expop->error = 0;

    pip_spawn_from_main( &prog, argv[0], argv, NULL );
    for( i=0; i<total; i++ ) {
      pipid = i;
      TESTINT( pip_task_spawn( &prog, PIP_CPUCORE_ASIS, &pipid, NULL ));
    }
    for( i=0; i<total; i++ ) {
      TESTINT( pip_wait_any( NULL, NULL ) );
    }
    TESTINT( pip_universal_barrier_fin( &expop->ubarr ) );
    if( expop->error ) {
      printf( "failure\n" );
    } else {
      printf( "SUCCESS\n" );
    }
  } else {
    TESTINT( pip_task_barrier_wait( &expop->pbarr ) );
    if( pipid % nulps == 0 ) {
      int nt = pipid / nulps;
      for( i=0; i<nulps-1; i++ ) {
#ifdef DEBUG
	fprintf( stderr, "[%d] sem_wait(nt:%d, %d/%d)\n", pipid, nt, i, nulps);
#endif
	TESTINT( sem_wait( &expop->sem_array[nt] ) );
	TESTINT( pip_ulp_dequeue_and_involve( &expop->queue, NULL, 0 ) );
      }
#ifdef DEBUG
      fprintf( stderr, "[%d] sem_wait(DONE)\n", pipid);
#endif
    } else {
      TESTINT( pip_sleep_and_enqueue( &expop->queue, wakeup_task, 0 ) );
    }

    TESTINT( pip_universal_barrier_wait( &expop->ubarr ) );
    for( i=0; i<niters; i++ ) {
      if( pipid == 0 ) {
	int j;
	for( j=0; j<total; j++ ) {
	  if( expop->count[j] != i ) {
	    fprintf( stderr, "[%d] count[%d]:%d != %d\n",
		     pipid, j, expop->count[j], i );
	    expop->error = 1;
	  }
	}
	if( isatty(1) && i%10 == 0 ) fprintf( stderr, "[%d] %d\n", pipid, i );
      }
      TESTINT( pip_universal_barrier_wait( &expop->ubarr ) );
      if( rand() % total == pipid ) usleep( 10 ); /* disturbance */
      expop->count[pipid] ++;
      TESTINT( pip_universal_barrier_wait( &expop->ubarr ) );
    }
  }
  TESTINT( pip_fin() );
  return 0;
}
