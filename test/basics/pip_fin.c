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
 * Written by Atsushi HORI <ahori@riken.jp>, 2016, 2017, 2018, 2019
 */

#include <test.h>

int main( int argc, char **argv ) {
  int pipid, ntasks, rv;
  void *exp;

  ntasks = 1;
  exp = NULL;
  rv = pip_init( &pipid, &ntasks, &exp, 0);


  if( pipid == PIP_PIPID_ROOT ) {
    pipid = 0;
    rv = pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
		    NULL, NULL, NULL );
    if( rv != 0 ) {
      fprintf( stderr, "pip_spawn: %s\n", strerror( rv ) );
      return EXIT_FAIL;
    }
    rv = pip_fin();
    if( rv != EBUSY ) {
      fprintf( stderr, "pip_fin/1: %s\n", strerror( rv ) );
      return EXIT_FAIL;
    }
    rv = pip_wait( 0, NULL );
    if( rv != 0 ) {
      fprintf( stderr, "pip_wait: %s\n", strerror( rv ) );
      return EXIT_FAIL;
    }
    rv = pip_fin();
    if( rv != 0 ) {
      fprintf( stderr, "pip_fin/2: %s\n", strerror( rv ) );
      return EXIT_FAIL;
    }
    printf( "Hello, pip root task is fine !!\n" );
  } else {
    printf( "<%d> sleeping...\n", pipid );
    sleep(1);
    printf( "<%d> slept!!!\n", pipid );
  }

  return EXIT_PASS;
}
