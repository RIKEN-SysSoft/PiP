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

#include <test.h>

static int static_entry( void *args ) {
  int arg = *((int*)args);
  int pipid = -1, rv;

  rv = pip_get_pipid( &pipid );
  if ( rv != EPERM ) {
    fprintf( stderr, "pip_get_pipid(static_entry): %s\n", strerror( rv ) );
    return 1;
  }

  rv = pip_init( NULL, NULL, NULL, 0 );
  if ( rv != 0 ) {
    fprintf( stderr, "pip_init(static_entry): %s\n", strerror( rv ) );
    return 1;
  }

  rv = pip_get_pipid( &pipid );
  if ( rv != 0 ) {
    fprintf( stderr, "pip_get_pipid(static_entry): %s\n", strerror( rv ) );
    return 1;
  }
  if( pipid != arg ) {
    fprintf( stderr, "static_entry: %d != %d\n", pipid, arg );
    return 1;
  }
  return 0;
}

int global_entry( void *args ) {
  int arg = *((int*)args);
  int pipid, rv;

  rv = pip_get_pipid( &pipid );
  if ( rv != EPERM ) {
    fprintf( stderr, "pip_get_pipid(global_entry): %s\n", strerror( rv ) );
    return 1;
  }

  rv = pip_init( NULL, NULL, NULL, 0 );
  if ( rv != 0 ) {
    fprintf( stderr, "pip_init(global_entry): %s\n", strerror( rv ) );
    return 1;
  }

  rv = pip_get_pipid( &pipid );
  if ( rv != 0 ) {
    fprintf( stderr, "pip_get_pipid(global_entry): %s\n", strerror( rv ) );
    return 1;
  }
  if( pipid != arg ) {
    fprintf( stderr, "global_entry: %d != %d\n", pipid, arg );
    return 1;
  }
  return 0;
}

int main( int argc, char **argv ) {
  pip_spawn_program_t prog;
  int pipid, ntasks, rv;

  /* before calling pip_init(), this must fail */
  pip_spawn_from_func( &prog, argv[0], "static_entry", (void*) &pipid, NULL );
  pipid = PIP_PIPID_ANY;
  rv = pip_task_spawn( &prog, PIP_CPUCORE_ASIS, 0, &pipid, NULL );
  if( rv != EPERM ) {
    fprintf( stderr, "pip_spawn: %s\n", strerror( rv ) );
    return EXIT_FAIL;
  }

  ntasks = NTASKS;
  rv = pip_init( NULL, &ntasks, NULL, 0 );
  if ( rv != 0 ) {
    fprintf( stderr, "pip_init: %s\n", strerror( rv ) );
    return EXIT_FAIL;
  }

  /* after calling pip_init(), this must succeed if it is the root process */
  pip_spawn_from_func( &prog, argv[0], "static_entry", (void*) &pipid, NULL );
  pipid = PIP_PIPID_ANY;
  rv = pip_task_spawn( &prog, PIP_CPUCORE_ASIS, 0, &pipid, NULL );
  if( rv != ENOEXEC ) {
    fprintf( stderr, "pip_spawn: %s\n", strerror( rv ) );
    return EXIT_FAIL;
  }

  pip_spawn_from_func( &prog, argv[0], "global_entry", (void*) &pipid, NULL );
  pipid = PIP_PIPID_ANY;
  rv = pip_task_spawn( &prog, PIP_CPUCORE_ASIS, 0, &pipid, NULL );
  if( rv != 0 ) {
    fprintf( stderr, "pip_spawn: %s\n", strerror( rv ) );
    return EXIT_FAIL;
  }

  if( pip_isa_task() ) {
    if( rv != EPERM ) {
      fprintf( stderr, "pip_spawn: %s\n", strerror( rv ) );
      return EXIT_FAIL;
    }
  } else {
    if( rv != 0 ) {
      fprintf( stderr, "pip_spawn: %s\n", strerror( rv ) );
      return EXIT_FAIL;
    }
    int status = 0, extval = 0;
    rv = pip_wait( pipid, &status );
    if( rv != 0 ) {
      fprintf( stderr, "pip_wait: %s\n", strerror( rv ) );
      return EXIT_UNTESTED;
    }
    if( WIFEXITED( status ) ) {
      extval = WEXITSTATUS( status );
      if( extval != 0 ) {
	fprintf( stderr, "task returns with an error\n" );
      }
    } else {
      extval = EXIT_UNRESOLVED;
    }
    return extval;
  }
  return EXIT_PASS;
}
