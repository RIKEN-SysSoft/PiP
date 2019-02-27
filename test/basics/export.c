/*
  * $RIKEN_copyright: Riken Center for Computational Sceience,
  * System Software Development Team, 2016, 2017, 2018, 2019$
  * $PIP_VERSION: Version 1.0.1$
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

int a[100];

int main( int argc, char **argv, char **envv ) {
  void *exp;
  int pipid, ntasks;
  int i;

  a[0] = 123456;

  ntasks = NTASKS;
  exp = (void*) a;
  TESTINT( pip_init( &pipid, &ntasks, &exp, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    for( i=0; i<NTASKS; i++ ) {
      int err;
      pipid = i;
      err = pip_spawn( argv[0], argv, NULL, i % cpu_num_limit(),
		       &pipid, NULL, NULL, NULL );
      if( err != 0 ) {
	fprintf( stderr, "pip_spawn(%d/%d): %s\n",
		 i, NTASKS, strerror( err ) );
	break;
      }
      if( i != pipid ) {
	fprintf( stderr, "pip_spawn(%d!=%d) !!!!!!\n", i, pipid );
      }
    }
    ntasks = i;
    for( i=0; i<ntasks; i++ ) {
      //fprintf( stderr, "[ROOT] >> import[%d]\n", i );
      while( 1 ) {
	TESTINT( pip_import( i, &exp ) );
	if( exp != NULL ) break;
	pause_and_yield( 0 );
      }
      DBG;
      //fprintf( stderr, "[ROOT] << import[%d]: %d\n", i, *(int*)exp );
    }
    DBG;
    for( i=0; i<ntasks; i++ ) {
      a[i+1] = i + 100;
      TESTINT( pip_wait( i, NULL ) );
    }
    TESTINT( pip_fin() );
  } else {
    void *root;

    a[0] = pipid * 10;
    TESTINT( pip_export( (void*) a ) );
    while( 1 ) {
      TESTINT( pip_import( PIP_PIPID_ROOT, &root ) );
      if( root != NULL ) break;
      pause_and_yield( 0 );
    }
    DBG;
    //fprintf( stderr, "[%d] import[ROOT]: %d\n", pipid, *(int*)root );
    while( 1 ) {
      if( ((int*)root)[pipid+1] == pipid + 100 ) break;
      pause_and_yield( 0 );
    }
    fprintf( stderr, "[PID=%d] Hello, my PIPID is %d\n", getpid(), pipid );
  }
  DBG;
  return 0;
}
