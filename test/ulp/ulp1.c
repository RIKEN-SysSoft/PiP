/*
 * $RIKEN_copyright: 2018 Riken Center for Computational Sceience, 
 * 	  System Software Devlopment Team. All rights researved$
 * $PIP_VERSION: Version 1.0$
 * $PIP_license: Redistribution and use in source and binary forms,
 * with or without modification, are permitted provided that the following
 * conditions are met:
 * 
 *     Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 
 *     Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * official policies, either expressed or implied, of the FreeBSD Project.$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#define PIP_INTERNAL_FUNCS

#define NULPS	(10)

#define DEBUG
#include <test.h>

pip_ulp_t ulp_old;

int main( int argc, char **argv ) {
  int pipid = 999;
  int ntasks;
  void *expp;
  pip_ulp_t ulp_new, *ulp_bakp;

  fprintf( stderr, "PID %d\n", getpid() );

  ntasks = 20;
  TESTINT( pip_init( &pipid, &ntasks, NULL, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    pipid = 0;
    TESTINT( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			NULL, NULL, NULL ) );
    TESTINT( pip_wait( 0, NULL ) );
    TESTINT( pip_fin() );
  } else if( pipid == 0 ) {
    fprintf( stderr, "<%d> Hello, I am a parent task !!\n", pipid );
    TESTINT( pip_export( &ulp_old ) );
    pipid ++;
    TESTINT( pip_ulp_create( argv[0], argv, NULL, &pipid, NULL, NULL, &ulp_new ));
    TESTINT( pip_make_ulp( PIP_PIPID_MYSELF, NULL, NULL, &ulp_old ) );
    TESTINT( pip_ulp_yield_to( &ulp_old, &ulp_new ) );
  } else if( pipid < NULPS ) {
    fprintf( stderr, "<%d> Hello, I am a ULP task !!\n", pipid );
    TESTINT( pip_export( &ulp_old ) );
    TESTINT( pip_import( pipid - 1, &expp ) );
    pipid ++;
    TESTINT( pip_ulp_create( argv[0], argv, NULL, &pipid, NULL, NULL, &ulp_new ));
    TESTINT( pip_make_ulp( PIP_PIPID_MYSELF, NULL, NULL, &ulp_old ) );
    TESTINT( pip_ulp_yield_to( &ulp_old, &ulp_new ) );
    ulp_bakp = (pip_ulp_t*) expp;
    TESTINT( pip_ulp_yield_to( NULL, ulp_bakp ) );
  } else {
    fprintf( stderr, "<%d> Hello, I am the last ULP task !!\n", pipid );
    TESTINT( pip_ulp_yield_to( NULL, &ulp_new ) );
  }
  TESTINT( pip_fin() );
  return 0;
}
