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

#define DO_COREBIND

#ifdef DO_COREBIND
static int get_ncpus( void ) {
  cpu_set_t cpuset;
  CHECK( sched_getaffinity( 0, sizeof(cpuset), &cpuset ),
	 RV, exit(EXIT_UNTESTED) );
  return CPU_COUNT( &cpuset );
}
#endif

static pip_task_queue_t	queues[NTASKS+1];

static int is_taskq_empty( pip_task_queue_t *queue ) {
  int rv;

  pip_task_queue_lock( queue );
  rv = pip_task_queue_isempty( queue );
  pip_task_queue_unlock( queue );
  return rv;
}

int main( int argc, char **argv ) {
  pip_spawn_program_t	prog;
  int 	nacts, npass, ntasks, ntenv, pipid, extval;
  char 	env_ntasks[128], env_pipid[128], *env;
  int 	c, nc, i, j, err;

  set_sigint_watcher();

  if( argc < 4 ) return EXIT_UNTESTED;
  CHECK( access( argv[3], X_OK ),     RV, return(EXIT_UNTESTED) );
  CHECK( pip_check_pie( argv[3], 1 ), RV, return(EXIT_UNTESTED) );

  nacts = strtol( argv[1], NULL, 10 );
  CHECK( nacts,  RV<0||RV>NTASKS,  return(EXIT_UNTESTED) );
  npass = strtol( argv[2], NULL, 10 );
  CHECK( npass,  RV<0||RV>NTASKS,  return(EXIT_UNTESTED) );
  ntasks = nacts + npass;
  CHECK( ntasks, RV<=0||RV>NTASKS, return(EXIT_UNTESTED) );
  if( ( env = getenv( "NTASKS" ) ) != NULL ) {
    ntenv = strtol( env, NULL, 10 );
    if( ntasks > ntenv ) return(EXIT_UNTESTED);
  }

  for( i=0; i<nacts+1; i++ ) pip_task_queue_init( &queues[i], NULL );

  CHECK( pip_init( &pipid, &ntasks, NULL, 0 ), RV, return(EXIT_FAIL) );

  sprintf( env_ntasks, "%s=%d", PIP_TEST_NTASKS_ENV, ntasks );
  putenv( env_ntasks );
  pip_spawn_from_main( &prog, argv[3], &argv[3], NULL, NULL, NULL );

  nc = 0;
#ifdef DO_COREBIND
  nc = get_ncpus() - 1;
#endif

  for( i=0,j=0; i<npass; i++,j++ ) {
    pipid = i;
#ifndef DO_COREBIND
    c = PIP_CPUCORE_ASIS;
#else
    if( nc == 0 ) {
      c = PIP_CPUCORE_ASIS;
    } else {
      c = i % nc;
    }
#endif
    sprintf( env_pipid, "%s=%d", PIP_TEST_PIPID_ENV, pipid );
    putenv( env_pipid );

    j = ( j >= nacts ) ? 0 : j;
    CHECK( pip_blt_spawn( &prog, c,
			  PIP_TASK_INACTIVE,
			  &pipid, NULL, &queues[j], NULL ),
	   RV,
	   return(EXIT_UNTESTED) );
    CHECK( is_taskq_empty( &queues[j]), RV, return(EXIT_FAIL) );
  }
  for( i=npass,j=0; i<ntasks; i++,j++ ) {
    pipid = i;
#ifndef DO_COREBIND
    c = PIP_CPUCORE_ASIS;
#else
    if( nc == 0 ) {
      c = PIP_CPUCORE_ASIS;
    } else {
      c = ( i % nc ) + 1;
    }
#endif
    sprintf( env_pipid, "%s=%d", PIP_TEST_PIPID_ENV, pipid );
    putenv( env_pipid );
    CHECK( pip_blt_spawn( &prog, c,
			  PIP_TASK_ACTIVE,
			  &pipid, NULL, &queues[j], NULL ),
	   RV,
	   return(EXIT_UNTESTED) );
    CHECK( is_taskq_empty( &queues[j]), !RV, return(EXIT_FAIL) );
  }
  err    = 0;
  extval = 0;
  for( i=0; i<ntasks; i++ ) {
    int status = 0;
    CHECK( pip_wait_any( &pipid, &status ), RV, return(EXIT_FAIL) );
    if( WIFEXITED( status ) ) {
      extval = WEXITSTATUS( status );
      if( extval ) {
	fprintf( stderr, "Task[%d] returns %d\n", pipid, extval );
      }
    } else if( WIFSIGNALED( status ) ) {
      extval = WTERMSIG( status );
      fprintf( stderr, "Task[%d] is signaled %d\n", pipid, extval );
      extval = EXIT_UNRESOLVED;
    } else {
      extval = EXIT_UNRESOLVED;
    }
    if( err == 0 && extval != 0 ) err = extval;
  }
  CHECK( pip_fin(), RV, return(EXIT_UNTESTED) );
  return extval;
}
