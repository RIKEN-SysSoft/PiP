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
//#define DEBUG

#include <semaphore.h>
#include <test.h>
#include <pip_ulp.h>

#ifdef DEBUG
# ifdef NTASKS
#  undef NTASKS
#  define NTASKS	(10)
# endif
#endif

#ifndef DEBUG
#define NULPS		(NTASKS-10)
#else
#define NULPS		(NTASKS-5)
#endif

#define BIAS		(10000)

struct expo {
  sem_t			semaphore;
  pip_locked_queue_t	queue;
  volatile int		c;
  pip_ulp_barrier_t 	barr;
} expo;

void wakeup_task( pip_task_t *task ) {
  struct expo *expop;

  TESTINT( pip_import( PIP_PIPID_ROOT, (void**) &expop ) );
  TESTINT( sem_post( &expop->semaphore ) );
}

int main( int argc, char **argv ) {
  struct expo *expop = &expo;
  int ntasks = 0;
  int i, pipid, nulps, extval;

  if( argc   > 1 ) {
    ntasks = atoi( argv[1] );
  } else {
    ntasks = NTASKS;
  }
  if( ntasks < 2 ) {
    fprintf( stderr,
	     "Too small number of PiP tasks (must be latrger than 1)\n" );
    exit( 1 );
  }
  nulps = ntasks - 1;

  expo.c = BIAS;
  TESTINT( pip_init( &pipid, &ntasks, (void**) &expop, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    pip_spawn_program_t prog;

    TESTINT( sem_init( &expop->semaphore, 1, 0 ) );
    pip_ulp_barrier_init( &expo.barr, ntasks );
    TESTINT( pip_locked_queue_init( &expop->queue ) );

    pip_spawn_from_main( &prog, argv[0], argv, NULL );
    for( i=0; i<ntasks; i++ ) {
      pipid = i;
      TESTINT( pip_task_spawn( &prog, PIP_CPUCORE_ASIS, &pipid, NULL ));
    }
    extval = 0;
    for( i=0; i<ntasks; i++ ) {
      int status;
      TESTINT( pip_wait( i, &status ) );
      if( status != 0 ) extval = 1;
    }
    if( extval ) {
      printf( "NG !!\n" );
    } else {
      printf( "Good\n" );
    }
  } else {
    if( pipid == 0 ) {
      for( i=0; i<nulps; i++ ) {
	TESTINT( sem_wait( &expop->semaphore ) );
	TESTINT( pip_ulp_dequeue_and_involve( &expop->queue, NULL, 0 ) );
      }
    } else {
      /* become ULP */
      TESTINT( pip_sleep_and_enqueue( &expop->queue, wakeup_task, 0 ) );
    }
    extval = 0;
    for( i=0; i<nulps; i++ ) {
      /* Disturbances */
      //int nyields = rand() % pipid;
      //for( i=0; i<nyields; i++ ) TESTINT( pip_ulp_yield() );

      TESTINT( pip_ulp_barrier_wait( &expop->barr ) );
      if( i == pipid ) {
	if( expop->c == ( pipid + BIAS ) ) {
	  fprintf( stderr, "<%d> Looks good !!\n", pipid );
	} else {
	  fprintf( stderr, "<%d> Bad ULP (%d!=%d) !!\n",
		   pipid, expop->c, pipid + BIAS );
	  extval = 1;
	}
	expop->c ++;
      }
    }
  }
  TESTINT( pip_fin() );
  return extval;
}
