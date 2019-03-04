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

#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>

#include <test.h>

struct comm_st {
  pthread_barrier_t barrier;
  int	go;
};

struct comm_st	comm;

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

int main( int argc, char **argv ) {
  struct comm_st *commp;
  void *exp;
  char corestr[32];
  char *nargv[3];
  int pipid;
  int ntasks;
  int core, i;
  int err;

  ntasks = NTASKS;
  exp    = &comm;
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
    TESTINT( pthread_barrier_init( &comm.barrier, NULL, ntasks+1 ) );
    comm.go = 1;
    pthread_barrier_wait( &comm.barrier );
    print_numa();
    fflush( NULL );
    comm.go = 0;
    pthread_barrier_wait( &comm.barrier );

    fprintf( stderr, "Root: done\n" );

    for( i=0; i<ntasks; i++ ) TESTINT( pip_wait( i, NULL ) );
    TESTINT( pip_fin() );

  } else {
    commp = (struct comm_st*) exp;
    TESTINT( pip_export( &comm ) );
    while( 1 ) {
      if( commp->go ) break;
      pause_and_yield( 10 );
    }
    pthread_barrier_wait( &commp->barrier );
    pthread_barrier_wait( &commp->barrier );
  }
  return 0;
}
