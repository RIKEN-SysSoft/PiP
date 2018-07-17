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

//#define DEBUG
#define PIP_INTERNAL_FUNCS
#include <test.h>

#include <fcntl.h>

#define ENVNAM		"HOOK_FD"
#define FDNUM		(100)

#define SZ	(16)

int bhook( void *hook_arg ) {
  int *array = (int*) hook_arg;
  int i;
  fprintf( stderr, "before hook is called\n" );
  for( i=0; i<SZ; i++ ) {
    if( array[i] != i ) {
      fprintf( stderr, "Hook_arg[%d] != %d\n", i, array[i] );
      return -1;
    }
  }
  return 0;
}

int ahook( void *hook_arg ) {
  fprintf( stderr, "after hook is called\n" );
  free( hook_arg );
  return 0;
}

int main( int argc, char **argv ) {
  int pipid = 999;
  int ntasks;
  int *hook_arg;
  int i;

  ntasks = 1;
  TESTINT( pip_init( &pipid, &ntasks, NULL, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    pipid = 0;
    hook_arg = (int*) malloc( sizeof(int) * SZ );
    for( i=0; i<SZ; i++ ) hook_arg[i] = i;
    TESTINT( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			bhook, ahook, (void*) hook_arg ) );
    TESTINT( pip_wait( 0, NULL ) );

    char *mesg = "This message should not apper\n";
    if( write( FDNUM, mesg, strlen( mesg ) ) > 0 ) {
      fprintf( stderr, "something wrong !!\n" );
    }
    TESTINT( pip_fin() );

  } else {
    fprintf( stderr, "[%d] Hello, I am fine !!\n", pipid );
  }
  return 0;
}
