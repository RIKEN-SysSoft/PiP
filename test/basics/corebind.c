/*
 * $RIKEN_copyright: Riken Center for Computational Sceience (R-CCS),
 * System Software Development Team, 2016-2020
 * $
 * $PIP_VERSION: Version 2.0.0$
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
/*
 * Written by Atsushi HORI <ahori@riken.jp>
 */

#include <test.h>

static int nth_core( int nth, cpu_set_t *cpuset ) {
  int i, j, ncores;
  ncores = CPU_COUNT( cpuset );
  nth %= ncores;
  for( i=0, j=0; i<ncores; i++ ) {
    if( CPU_ISSET( i, cpuset ) ) {
      if( j++ == nth ) return i;
    }
  }
  return -1;
}

int main( int argc, char **argv ) {
  cpu_set_t	init_set, *init_setp;
  void		*exp;
  char		*env;
  int 		ntasks, ntenv, pipid;
  int		core, i, extval = 0;

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

  exp = &init_set;
  CHECK( pip_init(&pipid,&ntasks,(void**)&exp,0), RV, return(EXIT_FAIL) );
  if( pipid == PIP_PIPID_ROOT ) {
    CPU_ZERO( &init_set );
    CHECK( sched_getaffinity( 0, sizeof(init_set), &init_set ),
	   RV,
	   return(EXIT_UNRESOLVED) );
    for( i=0; i<ntasks; i++ ) {
      core = nth_core( i, &init_set );
      pipid = i;
      printf( ">> PIPID:%d core:%d\n", pipid, core );
      CHECK( pip_spawn(argv[0],argv,NULL,core,&pipid,NULL,NULL,NULL),
	     RV,
	     return(EXIT_FAIL) );
    }
    for( i=0; i<ntasks; i++ ) {
      int status;
      CHECK( pip_wait_any( NULL, &status ), RV, return(EXIT_FAIL) );
      if( WIFEXITED( status ) ) {
	extval = WEXITSTATUS( status );
	CHECK( ( extval != 0 ), RV, return(EXIT_FAIL) );
      } else {
	CHECK( 1, RV, RV=0 );
	extval = EXIT_UNRESOLVED;
	break;
      }
    }

  } else {
    init_setp = (cpu_set_t*) exp;
    core = nth_core( pipid, init_setp );
    CHECK( CPU_ISSET( core, init_setp ),
	   !RV,
	   return(EXIT_FAIL) );
    printf( "<< PIPID:%d core:%d\n", pipid, core );
  }
  CHECK( pip_fin(), RV, return(EXIT_FAIL) );
  return extval;
}
