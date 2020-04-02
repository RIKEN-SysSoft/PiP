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

#define DEBUG
#include <test.h>

static int static_entry( void *args ) __attribute__ ((unused));
static int static_entry( void *argp ) {
  /* indeed, this function is not reachable */
  int arg = *((int*)argp);
  int pipid = -100;

#ifdef NO_IMPLICIT_INIT
  CHECK( pip_get_pipid( &pipid ), 	  RV!=EPERM, return(1) );
#else
  CHECK( pip_get_pipid( &pipid ), 	  RV,        return(1) );
#endif
  CHECK( pip_init( NULL, NULL, NULL, 0 ), RV, 	     return(1) );
  CHECK( pip_get_pipid( &pipid ),	  RV,	     return(1) );
  CHECK( pipid!=arg, 			  RV,	     return(1) );
  return 0;
}

int global_entry( void *argp ) {
  int arg = *((int*)argp);
  int pipid = -100;

#ifdef NO_IMPLICIT_INIT
  CHECK( pip_get_pipid( &pipid ), 	  RV!=EPERM, return(1) );
#else
  CHECK( pip_get_pipid( &pipid ), 	  RV,        return(1) );
#endif
  CHECK( pip_init( NULL, NULL, NULL, 0 ), RV,        return(1) );
  CHECK( pip_get_pipid( &pipid ), 	  RV,        return(1) );
  CHECK( pipid!=arg, 			  RV,        return(1) );
  return 0;
}

int main( int argc, char **argv ) {
  pip_spawn_program_t prog;
  int pipid, arg, ntasks;

  /* before calling pip_init(), this must fail */
  pipid = PIP_PIPID_ANY;
  CHECK( pip_task_spawn( NULL, PIP_CPUCORE_ASIS, 0, &pipid, NULL ),
	 RV!=EPERM,
	 return(EXIT_FAIL) );

  memset( &prog, 0, sizeof(prog) );
  pipid = PIP_PIPID_ANY;
  CHECK( pip_task_spawn( &prog, PIP_CPUCORE_ASIS, 0, &pipid, NULL ),
	 RV!=EPERM,
	 return(EXIT_FAIL) );

  pip_spawn_from_func( &prog, argv[0], "static_entry", (void*) &arg, NULL );
  pipid = PIP_PIPID_ANY;
  CHECK( pip_task_spawn( &prog, PIP_CPUCORE_ASIS, 0, &pipid, NULL ),
	 RV!=EPERM,
	 return(EXIT_FAIL) );

  ntasks = NTASKS;
  CHECK( pip_init( NULL, &ntasks, NULL, 0 ), RV, return(EXIT_FAIL) );

  /* after calling pip_init(), it must succeed if it is called with right args */
  pipid = PIP_PIPID_ANY;
  CHECK( pip_task_spawn( NULL, PIP_CPUCORE_ASIS, 0, &pipid, NULL ),
	 RV!=EINVAL,
	 return(EXIT_FAIL) );

  memset( &prog, 0, sizeof(prog) );
  pipid = PIP_PIPID_ANY;
  CHECK( pip_task_spawn( &prog, PIP_CPUCORE_ASIS, 0, &pipid, NULL ),
	 RV!=EINVAL,
	 return(EXIT_FAIL) );

  pip_spawn_from_func( &prog, argv[0], "static_entry", (void*) &arg, NULL );
  pipid = PIP_PIPID_ANY;
  CHECK( pip_task_spawn( &prog, PIP_CPUCORE_ASIS, 0, &pipid, NULL ),
	 RV!=ENOEXEC,
	 return(EXIT_FAIL) );

  pip_spawn_from_func( &prog, argv[0], "global_entry", (void*) &arg, NULL );
  pipid = PIP_PIPID_ANY;

  if( pip_isa_task() ) {
    CHECK( pip_task_spawn( &prog, PIP_CPUCORE_ASIS, 0, &pipid, NULL ),
	   RV!=EPERM,
	   return(EXIT_FAIL) );

  } else {
    int status = 0, extval = 0;

    pipid = arg = 8;
    CHECK( pip_task_spawn( &prog, PIP_CPUCORE_ASIS, 0, &pipid, NULL ),
	   RV,
	   return(EXIT_FAIL) );
    CHECK( pip_wait( pipid, &status ), RV, return(EXIT_UNTESTED) );

    if( WIFEXITED( status ) ) {
      extval = WEXITSTATUS( status );
      CHECK( extval!=0, RV, return(EXIT_FAIL) );

    } else {
      CHECK( 1, RV, return(EXIT_UNRESOLVED) );
    }
    return extval;
  }
  CHECK( pip_fin(), RV, return(EXIT_FAIL) );
  return EXIT_PASS;
}
