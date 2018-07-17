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

//#define DEBUG
#include <test.h>
#include <netdb.h>

struct hostent *my_gethostbyname( char *name ) {
  DBG;
  pip_print_fs_segreg();
  CHECK_CTYPE;
  return gethostbyname( name );
}

int main( int argc, char **argv ) {
  int pipid = 999;
  int ntasks;
  struct hostent *he;

  CHECK_CTYPE;
  pip_print_fs_segreg();
  ntasks = 1;
  TESTINT( pip_init( &pipid, &ntasks, NULL, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    if( ( he = gethostbyname( "localhost" ) ) == NULL ) {
      fprintf( stderr, "gethostbyname( localhost ) fails\n" );
    } else {
      fprintf( stderr, "hostname: %s\n", he->h_name );
    }
    if( ( he = gethostbyname( "127.0.0.1" ) ) == NULL ) {
      fprintf( stderr, "gethostbyname( 127.0.0.1 ) fails\n" );
    } else {
      fprintf( stderr, "hostname: %s\n", he->h_name );
    }
    print_maps();
  CHECK_CTYPE;

    pipid = 0;
    TESTINT( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			NULL, NULL, NULL ) );
    TESTINT( pip_wait( 0, NULL ) );
    TESTINT( pip_fin() );

  } else {
    //attachme();
    //print_maps();
  CHECK_CTYPE;

    if( ( he = gethostbyname( "127.0.0.1" ) ) == NULL ) {
      DBG;
      fprintf( stderr, "gethostbyname( 127.0.0.1 ) fails\n" );
    } else {
      DBG;
      fprintf( stderr, "hostname: %s\n", he->h_name );
    }
  CHECK_CTYPE;
    if( ( he = my_gethostbyname( "localhost" ) ) == NULL ) {
      DBG;
      fprintf( stderr, "gethostbyname( localhost ) fails\n" );
    } else {
      DBG;
      fprintf( stderr, "hostname: %s\n", he->h_name );
    }
  }
  return 0;
}
