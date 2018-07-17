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
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#define _GNU_SOURCE
#include <sched.h>
#include <test.h>

pthread_barrier_t barrier;

int assign_core( void ) {
  static int init = 0;
  static int core = 0;
  static cpu_set_t allset;
  cpu_set_t cpuset;
  int nc, i;

  if( !init ) {
    init = 1;
    CPU_ZERO( &allset );
    for( i=0; i<sizeof(allset)*8; i++ ) {
      CPU_ZERO( &cpuset );
      CPU_SET( i, &cpuset );
      if( sched_setaffinity( 0, sizeof(allset), &cpuset ) == 0 ) {
	CPU_OR( &allset, &cpuset, &allset );
      }
    }
    if( ( nc = CPU_COUNT( &allset ) ) == 0 ) {
      fprintf( stderr, "No core found\n" );
      return -1;
    } else {
      fprintf( stderr, "%d cores found\n", nc );
    }
  }
  while( 1 ) {
    if( CPU_ISSET( core, &allset ) ) break;
    core++;
    if( core >= sizeof(allset)*8 ) core = 0;
  }
  return core++;
}

int pipid;

void check_coreno( int coreno ) {
  cpu_set_t cpuset;
  int nc, i;

  CPU_ZERO( &cpuset );
  TESTINT( sched_getaffinity( 0, sizeof(cpuset), &cpuset ) );
  if( ( nc = CPU_COUNT( &cpuset ) ) != 1 ) {
    fprintf( stderr, "[PIPID=%d] I have %d cores !!!!\n", pipid, nc );
  } else if( !CPU_ISSET( coreno, &cpuset ) ) {
    for( i=0; i<sizeof(cpuset)*8; i++ ) {
      if( CPU_ISSET( i, &cpuset ) ) break;
    }
    fprintf( stderr, "[PIPID=%d] I am running on %d, not %d !!!!\n",
	     pipid, i, coreno );
  } else {
    fprintf( stderr, "[PIPID=%d] Hello, I am OK (running on %d) !!\n",
	     pipid, coreno );
  }
}

int main( int argc, char **argv ) {
  void *exp;
  char corestr[32];
  char *nargv[3];
  int ntasks;
  int core, i;
  int err;

  ntasks = NTASKS;
  exp    = NULL;
  TESTINT( pip_init( &pipid, &ntasks, &exp, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    for( i=0; i<NTASKS; i++ ) {
      if( ( core = assign_core() ) < 0 ) break;
      sprintf( corestr, "%d", core );
      nargv[0] = argv[0];
      nargv[1] = corestr;
      nargv[2] = NULL;
      pipid = i;

      fprintf( stderr, "Spawning task[%d] on %s\n", i, corestr );

      err = pip_spawn( nargv[0], nargv, NULL, core, &pipid, NULL, NULL, NULL );
      if( err != 0 ) break;
      if( i != pipid ) {
	fprintf( stderr, "pip_spawn(%d!=%d) !!!!!!\n", i, pipid );
      }
    }
    ntasks = i;

    for( i=0; i<ntasks; i++ ) TESTINT( pip_wait( i, NULL ) );
    TESTINT( pip_fin() );

  } else {
    check_coreno( atoi( argv[1] ) );
  }
  return 0;
}
