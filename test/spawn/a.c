/*
  * $RIKEN_copyright: Riken Center for Computational Sceience,
  * System Software Development Team, 2016, 2017, 2018, 2019, 2020$
  * $PIP_VERSION: Version 1.2.0$
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

#include <test.h>

static int pipid;

int main( int argc, char **argv ) {
  char *prog = argv[0];
  volatile int *root_exp;
  int ntasks;

  if( argc < 2 ) {
    fprintf( stderr, "%s: argc=%d\n", prog, argc );
  } else {
    TESTINT( pip_init( &pipid, &ntasks, (void**) &root_exp, 0 ) );

    if( pipid == PIP_PIPID_ROOT ) {
      fprintf( stderr, "%s: ROOT ????\n", prog );
    } else if( pipid != atoi( argv[1] ) ) {
      fprintf( stderr, "%s: %d != %s\n", prog, pipid, argv[1] );
    } else {
      TESTINT( pip_export( (void*) &pipid ) );
      while( 1 ) {
	pip_import( PIP_PIPID_ROOT, (void**) &root_exp );
	if( root_exp != NULL ) break;
	pause_and_yield( 100 );
      }
      while( !*root_exp ) pause_and_yield( 100 );
      fprintf( stderr, "%s#%d -- Hello, I am fine !\n", prog, pipid );
    }
  }
  return 0;
}
