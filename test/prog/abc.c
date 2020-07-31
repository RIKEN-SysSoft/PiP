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

#ifdef PIP
#include <pip.h>
#else
#include <stdio.h>
#endif

static char *msg;

int func( void ) {
  printf( "[func] %s\n", msg );
  return 0;
}

int main( int argc, char **argv ) {
  msg = argv[0];
#ifndef PIP
  printf( "[main] %s\n", msg );
#else
  if( argc > 2 ) {
    int pipid, ntasks=-1, id, prevn, err;

    if( ( err = pip_init( &pipid, &ntasks, NULL, 0 ) ) != 0 ) {
      fprintf( stderr, "Invoke this program by using piprun\n" );
      return 1;
    }
    prevn = strtol( argv[1], NULL, 10 );
    if( pipid < prevn ) {
      fprintf( stderr, "Illegal argument(s)\n" );
      return 9;
    }
    if( argc < pipid - prevn + 2 ) {
      fprintf( stderr, "PIPID list is short\n" );
      return 9;
    }
    id = strtol( argv[pipid-prevn+2], NULL, 10 );
    printf( "PIPID:%d (%d,%d)\n", pipid, argc, ntasks );
    if( id != pipid ) {
      printf( "[%d] %s != %d\n", pipid, argv[pipid+1], id );
      return 9;
    }
    printf( "[PIP:main:%d] %s\n", pipid, msg );
  } else {
    printf( "[PIP:main] %s\n", msg );
  }
#endif
  return 0;
}
