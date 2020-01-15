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
  * Written by Atsushi HORI <ahori@riken.jp>
*/

#define NITERS		(100)

#include <test.h>

int foo( int x ) {
  return x * 10;
}

int bar( int x ) {
  return x + 1234;
}

int main( int argc, char **argv ) {
  char *envstr, *envval, keystr[64], valstr[64];
  int i, n, niters = 0;

  if( argc > 1 ) {
    niters = strtol( argv[1], NULL, 10 );
  }
  niters = ( niters == 0 ) ? NITERS : niters;

  /* putenv and getenv */
  for( i=0; i<niters; i++ ) {
    asprintf( &envstr, "ENV%d=%d", i, foo( i ) );
    putenv( envstr );
  }
  for( i=0; i<niters; i++ ) {
    sprintf( keystr, "ENV%d", i );
    envval = getenv( keystr );
    CHECK( envval==NULL, RV, RETURN(EXIT_FAIL) );
    n = strtol( envval, NULL, 10 );
    CHECK( n!=foo(i),    RV, RETURN(EXIT_FAIL) );
  }

  /* setenv and getenv */
  for( i=0; i<niters; i++ ) {
    sprintf( keystr, "env%d", i );
    sprintf( valstr, "%d", bar( i ) );
    CHECK( setenv(keystr,valstr,0), RV, RETURN(EXIT_FAIL) );
  }
  for( i=0; i<niters; i++ ) {
    sprintf( keystr, "env%d", i );
    envval = getenv( keystr );
    CHECK( envval==NULL, RV, RETURN(EXIT_FAIL) );
    n = strtol( envval, NULL, 10 );
    CHECK( n!=bar(i),    RV, RETURN(EXIT_FAIL) );
  }

  return 0;
}
