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
  * Written by Atsushi HORI <ahori@riken.jp>, 2018
*/

//#define DEBUG
#include <test.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

int root_exp = 0;

int main( int argc, char **argv ) {
  int pipid, ntasks = 0;
  int i, retval = 0;
  int err;

  if( argc > 1 ) ntasks = atoi( argv[1] );
  if( ntasks == 0 ) ntasks = NTASKS;

  TESTINT( pip_init( &pipid, &ntasks, NULL, PIP_MODE_PROCESS ) );
  if( pipid == PIP_PIPID_ROOT ) {
    for( i=0; i<ntasks; i++ ) {

      pipid = i;
      err = pip_spawn( argv[0], argv, NULL, i % cpu_num_limit(),
		       &pipid, NULL, NULL, NULL );
      if( err ) {
	fprintf( stderr, "pip_spawn(%d/%d): %s\n",
		 i, NTASKS, strerror( err ) );
	break;
      }
    }
    for( i=0; i<ntasks; i++ ) {
      pid_t pid;

      TESTINT( pip_get_id( i, (intptr_t*) &pid ) );
      retval = 99999;
      {
	int wst;
	while( 1 ) {
	  if( waitpid( pid, &wst, 0 ) >= 0 ) {
	    if( WIFEXITED( wst ) ) {
	      retval = WEXITSTATUS( wst );
	    }
	    break;
	  } else if( errno != EINTR ) {
	    fprintf( stderr, "waitpid()=%d !!!!!!\n", errno );
	    break;
	  }
	}
      }
      if( retval != ( i & 0xFF ) ) {
	fprintf( stderr, "[PIPID=%d] waitpid() returns %d ???\n", i, retval );
      } else {
	fprintf( stderr, " terminated. OK\n" );
      }
    }
  } else {
    fprintf( stderr, "Hello, I am PIPID[%d, pid=%d]\n", pipid, getpid() );
  }
  TESTINT( pip_fin() );

  return pipid;
}
