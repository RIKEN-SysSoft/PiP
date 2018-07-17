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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define DEBUG
#include <pip.h>

int
main(int argc, char **argv)
{
  int c, mode, rv = 0;
  int option_pip_mode = 0;
  int pipid;

  while ((c = getopt( argc, argv, "CLPT" )) != -1) {
    switch (c) {
    case 'C':
      option_pip_mode = PIP_MODE_PROCESS_PIPCLONE;
      break;
    case 'L':
      option_pip_mode = PIP_MODE_PROCESS_PRELOAD;
      break;
    case 'P':
      option_pip_mode = PIP_MODE_PROCESS;
      break;
    case 'T':
      option_pip_mode = PIP_MODE_PTHREAD;
      break;
    default:
      fprintf(stderr, "Usage: pip_mode [-CLPT]\n");
      exit(2);
    }
  }

  rv = pip_init( &pipid, NULL, NULL, option_pip_mode );
  if ( rv != 0 ) {
    fprintf( stderr, "pip_init: %s\n", strerror( rv ));
    return EXIT_FAILURE;
  }
  if( pipid != PIP_PIPID_ROOT ) return 0;
  pipid = 0;
  rv = pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS,
		  &pipid, NULL, NULL, NULL );
  if ( rv != 0 ) {
    fprintf( stderr, "pip_spawn: %s\n", strerror( rv ));
    return EXIT_FAILURE;
  } else {
    (void) pip_wait( 0, NULL );
  }

  rv = pip_get_mode( &mode );
  if ( rv != 0 ) {
    fprintf( stderr, "pip_get_mode: %s\n", strerror( rv ));
    return EXIT_FAILURE;
  }

  switch ( mode ) {
  case PIP_MODE_PROCESS_PIPCLONE:
    printf( "%s\n", PIP_ENV_MODE_PROCESS_PIPCLONE );
    break;
  case PIP_MODE_PROCESS_PRELOAD:
    printf( "%s\n", PIP_ENV_MODE_PROCESS_PRELOAD );
    break;
  case PIP_MODE_PROCESS:
    printf( "%s\n", PIP_ENV_MODE_PROCESS );
    break;
  case PIP_MODE_PTHREAD:
    printf( "%s\n", PIP_ENV_MODE_PTHREAD );
    break;
  }

  return EXIT_SUCCESS;
}
