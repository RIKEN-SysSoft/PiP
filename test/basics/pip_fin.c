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

int task_main( void *arg ) {
  CHECK( pip_fin(), RV, return(EXIT_FAIL) );
  return 0;
}

int main( int argc, char **argv ) {
  pip_spawn_program_t prog;
  int pipid, ntasks;

  CHECK( pip_fin(), RV!=EPERM, return(EXIT_FAIL) );
  ntasks = 1;
  CHECK( pip_init( &pipid, &ntasks, NULL, 0), RV, return(EXIT_FAIL) );
  if( pipid == PIP_PIPID_ROOT ) {
    pip_spawn_from_func( &prog, argv[0], "task_main",
			 NULL, NULL, NULL, NULL );
    pipid = 0;
    CHECK( pip_task_spawn( &prog, PIP_CPUCORE_ASIS, 0, &pipid, NULL ),
	   RV,
	   return(EXIT_FAIL) );
    CHECK( pip_fin(), 		RV!=EBUSY, return(EXIT_FAIL) );
    CHECK( pip_wait( 0, NULL ), RV,	   return(EXIT_FAIL) );
    CHECK( pip_fin(), 		RV, 	   return(EXIT_FAIL) );
    CHECK( pip_fin(),           RV!=EPERM, return(EXIT_FAIL) );
  }
  return EXIT_PASS;
}
