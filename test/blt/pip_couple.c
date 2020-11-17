/*
 * $RIKEN_copyright: Riken Center for Computational Sceience (R-CCS),
 * System Software Development Team, 2016-2020
 * $
 * $PIP_VERSION: Version 3.0.0$
 * $PIP_license: <Simplified BSD License>
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *     Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 * 
 *     Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 * $
 */

#include <test.h>

#define NITERS		(100)

typedef struct exp {
  pip_task_queue_t	queue;
  pip_barrier_t		barrier;
} exp_t;


int main( int argc, char **argv ) {
  pip_spawn_program_t	prog;
  exp_t	exprt, *expp;
  int 	ntasks, ntenv, pipid;
  int	niters;
  int	i, extval = 0;
  char	*env;

  ntasks = 0;
  if( argc > 1 ) {
    ntasks = strtol( argv[1], NULL, 10 );
  }
  ntasks = ( ntasks <= 0      ) ? NTASKS-1 : ntasks;
  ntasks = ( ntasks >= NTASKS ) ? NTASKS-1 : ntasks;
  ntasks ++;
  if( ( env = getenv( "NTASKS" ) ) != NULL ) {
    ntenv = strtol( env, NULL, 10 );
    if( ntasks > ntenv ) return(EXIT_UNTESTED);
  }

  niters = 0;
  if( argc > 2 ) {
    niters = strtol( argv[2], NULL, 10 );
  }
  niters = ( niters <= 0 ) ? NITERS : niters;

  pip_spawn_from_main( &prog, argv[0], argv, NULL, NULL, NULL );
  expp = &exprt;

  CHECK( pip_init(&pipid,&ntasks,(void*)&expp,0), RV, return(EXIT_FAIL) );
  if( pipid == PIP_PIPID_ROOT ) {
    CHECK( pip_task_queue_init(&expp->queue ,NULL), RV, return(EXIT_FAIL) );
    CHECK( pip_barrier_init(&expp->barrier,ntasks), RV, return(EXIT_FAIL) );
    for( i=0; i<ntasks; i++ ) {
      pipid = i;
      CHECK( pip_blt_spawn( &prog, PIP_CPUCORE_ASIS, PIP_TASK_ACTIVE,
			    &pipid, NULL, NULL, NULL ),
	     RV,
	     return(EXIT_FAIL) );
    }
    for( i=0; i<ntasks; i++ ) {
      int status;
      CHECK( pip_wait_any(NULL,&status), RV, return(EXIT_FAIL) );
      if( WIFEXITED( status ) ) {
	CHECK( ( extval = WEXITSTATUS( status ) ),
	       RV,
	       return(EXIT_FAIL) );
      } else {
	CHECK( "Task is signaled", RV, return(EXIT_UNRESOLVED) );
      }
    }
  } else {
    if( pipid < ntasks-1 ) {
      CHECK( pip_suspend_and_enqueue(&expp->queue,NULL,NULL), RV, return(EXIT_FAIL) );
      for( i=0; i<niters; i++ ) {
	CHECK( pip_couple(),              RV, return(EXIT_FAIL) );
	CHECK( pip_decouple(NULL),        RV, return(EXIT_FAIL) );
      }
    } else {
      pip_task_t 	*task, *t;
      pip_task_queue_t 	queue;
      int 	 	m = ntasks - 1, n, i, err;

      pip_task_queue_init( &queue, NULL );
      CHECK( pip_get_task_by_pipid(PIP_PIPID_MYSELF,&task), RV,
	     return(EXIT_FAIL) );
      CHECK( pip_task_self()!=task, RV, return(EXIT_FAIL) );

      for( i=0; i<m; i++ ) {
	while( 1 ) {
	  t = pip_task_queue_dequeue( &expp->queue );
	  if( t != NULL ) break;
	  (void) pip_yield( PIP_YIELD_DEFAULT );
	}
	pip_task_queue_enqueue( &queue, t );
      }

      n = PIP_TASK_ALL;
      CHECK( pip_dequeue_and_resume_N(&queue,task,&n), RV, return(EXIT_FAIL) );
      for( i=0; i<niters; i++ ) {
	while( ( err = pip_yield(PIP_YIELD_DEFAULT) ) == 0 );
	CHECK( err != EINTR, RV, return(EXIT_FAIL) );
      }
    }
    CHECK( pip_barrier_wait(&expp->barrier), RV, return(EXIT_FAIL) );
  }
  CHECK( pip_fin(), RV, return(EXIT_FAIL) );
  return 0;
}
