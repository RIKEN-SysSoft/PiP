/*
  * $RIKEN_copyright: Riken Center for Computational Sceience,
  * System Software Development Team, 2016, 2017, 2018, 2019$
  * $PIP_VERSION: Version 1.0.0$
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

//#define DEBUG
#include <test.h>

char *tasks[] = { "./a.out", "./b.out", "./c.out", NULL };

int root_exp = 0;

int main( int argc, char **argv ) {
  char *newav[3];
  char pipid_str[64];
  void *exp = (void*) &root_exp;
  int pipid, ntasks;
  int i, j;
  int err;

  ntasks = NTASKS;
  TESTINT( pip_init( &pipid, &ntasks, &exp, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    for( i=0, j=0; i<NTASKS; i++ ) {
      if( tasks[j] == NULL ) j = 0;
      sprintf( pipid_str, "%d", i );
      newav[0] = tasks[j++];
      newav[1] = pipid_str;
      newav[2] = NULL;
      pipid = i;
      err = pip_spawn( newav[0], newav, NULL, i % cpu_num_limit(),
		       &pipid, NULL, NULL, NULL );
      if( err ) {
	fprintf( stderr, "pip_spawn(%d/%d): %s\n",
		 i, NTASKS, strerror( err ) );
	break;
      }

      if( i != pipid ) {
	fprintf( stderr, "pip_spawn(%d!=%d)=%d !!!!!!\n", i, pipid, err );
	break;
      }
    }
    ntasks = i;

    for( i=0; i<ntasks; i++ ) {
      DBGF( "%d", i );
      while( 1 ) {
	int *exp;
	TESTINT( pip_import( i, (void**) &exp ) );
	DBGF( "pip_import(%d,%p)", i, exp );
	if( exp != NULL ) {
	  if( *exp != i ) {
	    fprintf( stderr, "<R> expt[%d]=%d !!!!\n", i, *(int*)exp );
	  }
	  break;
	}
	pause_and_yield( 1000*1000 );
      }
    }
    root_exp = 1;

    for( i=0; i<ntasks; i++ ) {
      DBGF( "caling pip_wait(%d)", i );
      TESTINT( pip_wait( i, NULL ) );
    }
    TESTINT( pip_fin() );
  } else {
    /* should not reach here */
    fprintf( stderr, "ROOT: NOT ROOT ????\n" );
  }
  return 0;
}
