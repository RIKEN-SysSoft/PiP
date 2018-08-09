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

#include <test.h>
#include <pip_ulp.h>

//#define DEBUG

#ifndef DEBUG
#define ULP_COUNT 	(100);
#else
#define ULP_COUNT 	(10);
#endif

#ifdef DEBUG
# define MTASKS	(10)
#else
# define MTASKS	(NTASKS/2)
#endif

#define NULPS		(NTASKS/2)

struct expo {
  pip_barrier_t			barr;
  pip_ulp_locked_queue_t	queue;
} expo;

int ulp_main( void* null ) {
  struct expo *expop;
  int ulp_count = ULP_COUNT;
  int pipid;

  set_sigsegv_watcher();

  TESTINT( pip_init( &pipid, NULL, (void**) &expop, 0 ) );
  TESTINT( !pip_is_ulp() );
  while( --ulp_count >= 0 ) {
#ifdef DEBUG
    int pipid_sched;
    TESTINT( pip_get_ulp_sched( &pipid_sched ) );
    fprintf( stderr, "[ULPID:%d]  sched-task: %d  [%d]\n",
	     pipid, pipid_sched, ulp_count );
#endif
    pipid = 0;
    TESTINT( pip_ulp_suspend_and_enqueue( &expop->queue, 0 ) );
  }
  DBGF( "ULP terminates" );
  return 0;
}

int main( int argc, char **argv ) {
  struct expo *expop;
  int ntasks, nulps;
  int i, j, pipid, status, flag=0;

  set_sigsegv_watcher();

  if( argv[1] != NULL ) {
    ntasks = atoi( argv[1] );
  } else {
    ntasks = MTASKS;
  }
  if( ntasks < 1 ) {
    fprintf( stderr,
	     "Too small number of PiP tasks (must be latrger than 0)\n" );
    exit( 1 );
  } else if( ntasks > PIP_NTASKS_MAX ) {
    fprintf( stderr, "Number of PiP tasks (%d) is too large\n", ntasks );
    exit( 1 );
  }
  if( argv[1] != NULL && argv[2] != NULL ) {
    nulps = atoi( argv[2] );
    if( nulps == 0 ) {
      fprintf( stderr, "Number of ULPs must be larget than zero\n" );
      exit( 1 );
    }
  } else {
    nulps = NULPS;
  }
  DBGF( "ntasks:%d  nulps:%d", ntasks, nulps );
  int nt = ntasks + nulps;

  if( nt > PIP_NTASKS_MAX ) {
    fprintf( stderr,
	     "Number of PiP tasks (%d) and ULPs (%d) are too large\n",
	     ntasks,
	     nulps );
  }

  expop = &expo;
  TESTINT( pip_init( &pipid, &nt, (void**) &expop, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    pip_spawn_program_t func;
    pip_spawn_program_t prog;

    pip_spawn_from_func( &func, argv[0], "ulp_main", NULL, NULL );
    pip_task_barrier_init( &expop->barr, ntasks );
    TESTINT( pip_ulp_locked_queue_init( &expop->queue ) );

    j = ntasks;
    for( i=0; i<nulps; i++ ) {
      pip_ulp_t *ulp;
      pipid = j++;
      TESTINT( pip_ulp_new( &func, &pipid, NULL, &ulp ) );
      TESTINT( pip_ulp_enqueue_with_lock( &expop->queue, ulp, 0 ) );
    }
    pip_spawn_from_main( &prog, argv[0], argv, NULL );
    j = 0;
    for( i=0; i<ntasks; i++ ) {
      pipid = j++;
      TESTINT( pip_task_spawn( &prog, PIP_CPUCORE_ASIS, &pipid, NULL, NULL ) );
    }
    for( i=0; i<ntasks+nulps; i++ ) {
      TESTINT( pip_wait( i, &status ) );
      if( status != 0 ) {
	fprintf( stderr, "PIPID:%d exited with non-zero value (%d)\n",
		 i, status );
	flag = ( status > flag ) ? status : flag;
      }
    }
    if( !flag ) {
      fprintf( stderr, "SUCCEEDED\n" );
    } else {
      fprintf( stderr, "FAILED !!!!\n" );
    }
  } else {
    TESTINT( pip_import( PIP_PIPID_ROOT, (void**) &expop ) );
    pip_barrier_wait( &expop->barr );
    while( 1 ) {
      pip_ulp_t *next;
      int err = pip_ulp_dequeue_and_migrate( &expop->queue, &next, 0 );
      if( err == 0 ) {
	if( ( err = pip_ulp_yield_to( next ) ) != 0 ) {
	  fprintf( stderr, "**** pip_ulp_yield_to():%d\n", err );
	  pip_ulp_describe( stderr, "", next );
	  flag = 9;
	  break;
	}
      } else {	  /* there is no ulp to schedule (or error happens) */
	if( err == ENOENT ) break;
	fprintf( stderr, "**** pip_ulp_dequeue_and_migrate():%d\n", err );
	pip_ulp_describe( stderr, "", next );
	flag = 9;
      }
    }
  }
  TESTINT( pip_fin() );
  return flag;
}
