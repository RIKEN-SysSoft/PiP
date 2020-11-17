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

#define PIP_EXPERIMENTAL
#include <test.h>

#define NITERS		(100)

pip_barrier_t barr;

int main( int argc, char **argv ) {
  pip_barrier_t	*barrp = &barr;
  int 	ntasks, ntenv, pipid;
  int	niters, i, c;
  char	*env;

  ntasks = 0;
  if( argc > 1 ) {
    ntasks = strtol( argv[1], NULL, 10 );
  }
  ntasks = ( ntasks <= 0 ) ? NTASKS : ntasks;
  if( ( env = getenv( "NTASKS" ) ) != NULL ) {
    ntenv = strtol( env, NULL, 10 );
    if( ntasks > ntenv ) return(EXIT_UNTESTED);
  } else {
    if( ntasks > NTASKS ) return(EXIT_UNTESTED);
  }

  niters = 0;
  if( argc > 2 ) {
    niters = strtol( argv[2], NULL, 10 );
  }
  niters = ( niters <= 0 ) ? NITERS : niters;

  CHECK( pip_init(&pipid,&ntasks,(void**)&barrp,0), RV, return(EXIT_FAIL) );
  if( pipid == PIP_PIPID_ROOT ) {
    pip_spawn_program_t	prog;

    CHECK( pip_barrier_init( barrp, ntasks ), RV, return(EXIT_FAIL) );
    pip_spawn_from_main( &prog, argv[0], argv, NULL, NULL );
    for( i=0; i<ntasks; i++ ) {
      pipid = i;
      c = i % 10;
      CHECK( pip_blt_spawn( &prog, c, PIP_TASK_ACTIVE, &pipid,
			    NULL, NULL, NULL ),
	     RV,
	     return(EXIT_FAIL) );
    }
    for( i=0; i<ntasks; i++ ) {
      int status;
      CHECK( pip_wait_any(NULL,&status), RV, return(EXIT_FAIL) );
      if( WIFEXITED( status ) ) {
	CHECK( WEXITSTATUS( status ),  	 RV, return(EXIT_FAIL) );
      } else {
	CHECK( "Task is signaled",       RV, return(EXIT_UNRESOLVED) );
      }
    }

  } else {
    int nxt = ( pipid == ntasks -1 ) ? 0 : pipid + 1;
    pip_task_t *next;

    CHECK( pip_get_task_from_pipid(nxt,&next), RV, return(EXIT_FAIL) );

    CHECK( pip_barrier_wait( barrp ),          RV, return(EXIT_FAIL) );
    for( i=0; i<niters; i++ ) {
      CHECK( pip_migrate( next ),              RV, return(EXIT_FAIL) );
    }
    CHECK( pip_barrier_wait( barrp ),          RV, return(EXIT_FAIL) );
  }
  CHECK( pip_fin(),                            RV, return(EXIT_FAIL) );
  return 0;
}
