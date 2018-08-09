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
#define ULP_COUNT 	(100)
#else
#define ULP_COUNT 	(10)
#endif

#ifdef DEBUG
# define MTASKS	(10)
#else
# define MTASKS	(NTASKS/2)
//# define MTASKS	(10)
#endif

#define NULPS		(NTASKS/2)

#ifdef DEBUG
#define SCHED_CHECK	\
  if( pip_task->task_sched->pipid > MTASKS ) \
    fprintf( stderr, "%d: ??? [%d,%d] ???\n", \
	     __LINE__, pip_task->pipid, pip_task->task_sched->pipid )
#endif

struct expo {
  pip_barrier_t			barr;
  pip_ulp_locked_queue_t	queue;
} expo;

int ulp_main( void* null ) {
  struct expo *expop;
  int ulp_count = ULP_COUNT;
  int pipid;

  TESTINT( pip_init( &pipid, NULL, (void**) &expop, 0 ) );
  TESTINT( !pip_is_ulp() );
  while( --ulp_count >= 0 ) {
#define AH
#ifdef AH
    int pipid_sched;
    TESTINT( pip_ulp_get_sched( &pipid_sched ) );
    fprintf( stderr, "[ULPID:%d]  sched-task: %d  [%d/%d]\n",
	     pipid, pipid_sched, ulp_count, ULP_COUNT );
#endif
#ifdef SCHED_CHECK
    SCHED_CHECK;
#endif
    TESTINT( pip_ulp_suspend_and_enqueue( &expop->queue, 0 ) );
  }
  TESTINT( pip_fin() );
  return 0;
}

int task_main( void* null ) {
  struct expo *expop;
  pip_ulp_t *next;
  int pipid, flag = 0, err;

  set_sigsegv_watcher();

  TESTINT( pip_init( &pipid, NULL, (void**) &expop, 0 ) );
  pip_task_barrier_wait( &expop->barr );

  while( 1 ) {
#ifdef SCHED_CHECK
    SCHED_CHECK;
#endif
    err = pip_ulp_dequeue_and_migrate( &expop->queue, &next, 0 );
    if( err == 0 ) {
      if( ( err = pip_ulp_yield() ) != 0 ) {
	fprintf( stderr, "[%d] pip_ulp_yield_to():%d\n", pipid, err );
	pip_ulp_describe( stderr, "", next );
	flag = 9;
	break;
      }
    } else if( err != ENOENT ) {
      fprintf( stderr, "[%d] pip_ulp_dequeue_and_migrate():%d\n", pipid, err );
      pip_ulp_describe( stderr, "", next );
      flag = 9;
    } else {      /* exit loop when no ulp to schedule (ENOENT) */
      break;
    }
  }
  TESTINT( pip_fin() );
  return flag;
}

int main( int argc, char **argv ) {
  pip_spawn_program_t task;
  pip_spawn_program_t ulpp;
  struct expo *expop;
  int ntasks, nulps;
  int i, j, pipid, status, flag=0;

  set_sigsegv_watcher();

#ifndef SCHED_CHECK
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
    nulps = NTASKS - ntasks;
  }
#else
  ntasks = MTASKS;
  nulps = NTASKS - ntasks;
#endif

  DBGF( "ntasks:%d  nulps:%d", ntasks, nulps );
  int nt = ntasks + nulps;

  if( nt > NTASKS ) {
    fprintf( stderr,
	     "Number of PiP tasks (%d) and ULPs (%d) are too large\n",
	     ntasks,
	     nulps );
  }

  expop = &expo;
  TESTINT( pip_init( &pipid, &nt, (void**) &expop, 0 ) );
  pip_task_barrier_init( &expop->barr, ntasks );
  TESTINT( pip_ulp_locked_queue_init( &expop->queue ) );

  pip_spawn_from_func( &ulpp, argv[0], "ulp_main",  NULL, NULL );
  j = ntasks;
  for( i=0; i<nulps; i++ ) {
    pip_ulp_t *ulp;
    pipid = j++;
    TESTINT( pip_ulp_new( &ulpp, &pipid, NULL, &ulp ) );
    TESTINT( pip_ulp_enqueue_with_lock( &expop->queue, ulp, 0 ) );
  }

  pip_spawn_from_func( &task, argv[0], "task_main", NULL, NULL );
  j = 0;
  for( i=0; i<ntasks; i++ ) {
    pipid = j++;
    TESTINT( pip_task_spawn( &task, PIP_CPUCORE_ASIS, &pipid, NULL, NULL ) );
  }

  for( i=0; i<nt; i++ ) {
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

  TESTINT( pip_fin() );
  return flag;
}
