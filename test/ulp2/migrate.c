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
#include <pip_universal.h>

//#define DEBUG

#ifndef DEBUG
//#define ULP_COUNT 	(1000)
#define ULP_COUNT 	(10)
#else
#define ULP_COUNT 	(10)
#endif

#ifdef DEBUG
# define MTASKS	(10)
#else
//# define MTASKS	(NTASKS/4)
# define MTASKS	(NTASKS/2)
//# define MTASKS	(10)
#endif

#define NULPS		(NTASKS-MTASKS)

#ifdef DEBUG
#define SCHED_CHECK	\
  if( pip_task->task_sched->pipid > MTASKS ) \
    fprintf( stderr, "%d: ??? [%d,%d] ???\n", \
	     __LINE__, pip_task->pipid, pip_task->task_sched->pipid )
#endif

#define OLD_BARRIER
#ifdef OLD_BARRIER
#define PIP_BARRIER_T		pip_task_barrier_t
#define PIP_BARRIER_INIT_F	pip_task_barrier_init
#define PIP_BARRIER_WAIT_F	pip_task_barrier_wait
#else
#define PIP_BARRIER_T		pip_universal_barrier_t
#define PIP_BARRIER_INIT_F	pip_universal_barrier_init
#define PIP_BARRIER_WAIT_F	pip_universal_barrier_wait
#endif

struct expo {
  PIP_BARRIER_T		barr_ulps;
  PIP_BARRIER_T		barr_tsks;
  pip_locked_queue_t	queue;
} expo;

int ulp_main( void* null ) {
  struct expo *expop;
  int pipid, pipid_sched0, pipid_sched1;
  int ulp_count = 0, mgrt_count = 0;
  pid_t pid = getpid();

  TESTINT( pip_init( &pipid, NULL, (void**) &expop, 0 ) );
  PIP_BARRIER_WAIT_F( &expop->barr_ulps );
  TESTINT( pip_sleep_and_enqueue( &expop->queue, pipid & 1 ) );

  TESTINT( pip_ulp_get_sched_task( &pipid_sched0 ) );
  while( ulp_count++ < ULP_COUNT ) {
    if( isatty( 1 ) ) {
      fprintf( stderr, "[%d] %d -> %d\n", pipid, pid, getpid() );
    }
    pid = getpid();
    TESTINT( pip_ulp_suspend_and_enqueue( &expop->queue, 0 ) );
    TESTINT( pip_ulp_get_sched_task( &pipid_sched1 ) );
    if( pipid_sched0 != pipid_sched1 ) mgrt_count ++;
    pipid_sched0 = pipid_sched1;
  }
  if( isatty( 1 ) ) fprintf( stderr, "[%d] mgrt:%d\n", pipid, mgrt_count );
  TESTINT( pip_fin() );
  return 0;
}

int task_main( void* null ) {
  struct expo *expop;
  int pipid, flag = 0, err;

  set_sigsegv_watcher();

  TESTINT( pip_init( &pipid, NULL, (void**) &expop, 0 ) );
  PIP_BARRIER_WAIT_F( &expop->barr_tsks );

  while( 1 ) {
    err = pip_ulp_dequeue_and_migrate( &expop->queue, NULL, 0 );
    if( err == 0 ) {
      TESTINT( pip_ulp_yield() );
      continue;
    } else if( err != ENOENT ) {
      fprintf( stderr, "[%d] pip_ulp_dequeue_and_migrate():%d\n", pipid, err );
      flag = 9;
      break;
    } else {      /* exit loop when no ulp to schedule (ENOENT) */
      break;
    }
  }
  PIP_BARRIER_WAIT_F( &expop->barr_tsks );
  TESTINT( pip_fin() );
  return flag;
}

int main( int argc, char **argv ) {
  pip_spawn_program_t task;
  pip_spawn_program_t ulpp;
  struct expo *expop;
  int ntasks, nulps;
  int tskc=0, ulpc=0;
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
  if( argc > 2 && argv[2] != NULL ) {
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
  nulps  = NTASKS - ntasks;
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
  PIP_BARRIER_INIT_F( &expop->barr_ulps, nulps + 1 );
  PIP_BARRIER_INIT_F( &expop->barr_tsks, ntasks    );
  TESTINT( pip_locked_queue_init( &expop->queue ) );

  pip_spawn_from_func( &ulpp, argv[0], "ulp_main",  NULL, NULL );
  j = ntasks;
  for( i=0; i<nulps; i++ ) {
    pipid = j++;
    TESTINT( pip_task_spawn( &ulpp, PIP_CPUCORE_ASIS, &pipid, NULL ) );
  }
  PIP_BARRIER_WAIT_F( &expop->barr_ulps );

  pip_spawn_from_func( &task, argv[0], "task_main", NULL, NULL );
  j = 0;
  for( i=0; i<ntasks; i++ ) {
    pipid = j++;
    TESTINT( pip_task_spawn( &task, PIP_CPUCORE_ASIS, &pipid, NULL ) );
  }

  for( i=0; i<nt; i++ ) {
    TESTINT( pip_wait_any( &pipid, &status ) );
    if( status == 0 ) {
      if( isatty( 1 ) ) {
	if( pipid >= ntasks ) {
	  ulpc ++;
	  fprintf( stderr, "ULP:%d terminated (%d/%d)\n", pipid, ulpc, nulps );
	} else {
	  tskc ++;
	  fprintf( stderr, "TSK:%d terminated (%d/%d)\n", pipid, tskc, ntasks);
	}
      }
    } else {
      fprintf( stderr, "PIPID:%d exited with non-zero value (%d)\n",
	       pipid, status );
      flag = ( status > flag ) ? status : flag;
    }
  }

  if( !flag ) {
    printf( "SUCCEEDED\n" );
  } else {
    printf( "FAILED !!!!\n" );
  }

  TESTINT( pip_fin() );
  return flag;
}
