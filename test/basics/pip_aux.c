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

int user_func( void *argp ) {
  void *auxp;
  int pipid, *aux;

  CHECK( ( pip_get_pipid( &pipid ) ), RV, return(EXIT_FAIL) );
  CHECK( ( pip_get_aux(   &auxp  ) ), RV, return(EXIT_FAIL) );
  aux = (int*) auxp;
  if( *aux != pipid * 100 ) return 1;
  return 0;
}

int main( int argc, char **argv ) {
  pip_spawn_program_t prog;
  int pipid, arg, ntasks, ntenv;
  int *aux, i;
  int status = 0, extval = 0;
  char*env;


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

  CHECK( pip_init( NULL, &ntasks, NULL, 0 ), RV, return(EXIT_FAIL) );

  CHECK( ( aux = (int*) malloc( sizeof(int) * ntasks ) ),
	 RV==0, return(EXIT_FAIL) );

  for( i=0; i<ntasks; i++ ) {
    aux[i] = i * 100;
    memset( &prog, 0, sizeof(prog) );
    pip_spawn_from_func( &prog, argv[0], "user_func", (void*) &arg,
			 NULL, NULL, (void*) &aux[i] );
    pipid = i;
    CHECK( pip_task_spawn( &prog, PIP_CPUCORE_ASIS, 0, &pipid, NULL ),
	   RV,
	   return(EXIT_FAIL) );
  }
  for( i=0; i<ntasks; i++ ) {
    CHECK( pip_wait( i, &status ), RV, return(EXIT_FAIL) );
    if( WIFEXITED( status ) ) {
      extval = WEXITSTATUS( status );
      CHECK( extval!=0, RV, return(EXIT_FAIL) );
    } else {
      CHECK( 1, RV, return(EXIT_UNRESOLVED) );
    }
  }
  CHECK( pip_fin(), RV, return(EXIT_FAIL) );
  return EXIT_PASS;
}
