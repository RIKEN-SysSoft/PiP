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
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include <eval.h>
#include <pip_machdep.h>

#ifdef PIP
#include <pip.h>
#endif

#if defined(FORK_EXEC) || defined(VFORK_EXEC) || defined(POSIX_SPAWN)
#include <sys/wait.h>
#endif

#ifdef POSIX_SPAWN
#include <spawn.h>
#endif

#define NTASKS_MAX	(200)

double	time_start, time_spawn, time_end;
char *mode = "";

#ifdef PIP
void spawn_tasks( int ntasks ) {
  char *argv[] = { "./foo", NULL };
  int pipid, i;

  TESTINT( pip_init( &pipid, &ntasks, NULL, 0 ) );
  TESTINT( ( pipid != PIP_PIPID_ROOT ) );

  mode = (char*) pip_get_mode_str();

  time_start = gettime();
  for( i=0; i<ntasks; i++ ) {
    pipid = i;
    TESTINT( pip_spawn( argv[0],
			argv,
			NULL,
			PIP_CPUCORE_ASIS,
			&pipid,
			NULL,
			NULL,
			NULL ) );
  }
  time_spawn = gettime();
  for( i=0; i<ntasks; i++ ) {
    TESTINT( pip_wait( i, NULL ) );
  }
  time_end = gettime();

  TESTINT( pip_fin() );
}
#endif

#ifdef FORK_EXEC

void fork_exec( int ntasks ) {
  extern char **environ;
  char *argv[] = { "./foo", NULL };
  pid_t pid;
  int i;

  time_start = gettime();
  for( i=0; i<ntasks; i++ ) {
    if( ( pid = fork() ) == 0 ) {
      TESTSC( execve( argv[0], argv, environ ) );
    }
  }
  time_spawn = gettime();
  for( i=0; i<ntasks; i++ ) wait( NULL );
  time_end = gettime();
}
#endif

#ifdef VFORK_EXEC

void vfork_exec( int ntasks ) {
  extern char **environ;
  char *argv[] = { "./foo", NULL };
  pid_t pid;
  int i;

  time_start = gettime();
  for( i=0; i<ntasks; i++ ) {
    if( ( pid = vfork() ) == 0 ) {
      TESTSC( execve( argv[0], argv, environ ) );
    }
  }
  time_spawn = gettime();
  for( i=0; i<ntasks; i++ ) wait( NULL );
  time_end = gettime();
}
#endif

#ifdef POSIX_SPAWN

void posixspawn( int ntasks ) {
  extern char **environ;
  char *argv[] = { "./foo", NULL };
  pid_t pid;
  int i;

  time_start = gettime();
  for( i=0; i<ntasks; i++ ) {
    TESTSC( posix_spawn( &pid, argv[0], NULL, NULL, argv, environ ) );
  }
  time_spawn = gettime();
  for( i=0; i<ntasks; i++ ) wait( NULL );
  time_end = gettime();
}
#endif

#ifdef THREAD
void foo( void ) {
  pthread_exit( NULL );
}

void create_threads( int ntasks ) {
  pthread_t threads[NTASKS_MAX];
  pthread_attr_t attr;
  int i;

  time_start = gettime();
  for( i=0; i<ntasks; i++ ) {
    TESTINT( pthread_attr_init( &attr ) );
    TESTINT( pthread_create( &threads[i],
			     &attr,
			     (void*(*)(void*))foo,
			     NULL ) );
  }
  time_spawn = gettime();
  for( i=0; i<ntasks; i++ ) {
    TESTINT( pthread_join( threads[i], NULL ) );
  }
  time_end = gettime();
}
#endif

int main( int argc, char **argv ) {
  int ntasks;
  char *tag;

  if( argc < 2 ) {
    fprintf( stderr, "spawn-XXX <ntasks>\n" );
    exit( 1 );
  }
  ntasks = atoi( argv[1] );
  if( ntasks > NTASKS_MAX ) {
    fprintf( stderr, "Illegal number of tasks is specified.\n" );
    exit( 1 );
  }

  mode = "";
#if defined( PIP )
  spawn_tasks( ntasks );
  tag = "PIP";
#elif defined( FORK_EXEC )
  fork_exec( ntasks );
  tag = "FORK_EXEC";
#elif defined( VFORK_EXEC )
  vfork_exec( ntasks );
  tag = "VFORK_EXEC";
#elif defined( POSIX_SPAWN )
  posixspawn( ntasks );
  tag = "POSIX_SPAWN";
#elif defined( THREAD )
  create_threads( ntasks );
  tag = "PTHREADS";
#endif
  printf( "%g,  %g,  %g,  [sec] %d tasks -- %s %s\n",
	  time_end   - time_start,
	  time_spawn - time_start,
	  time_end   - time_spawn,
	  ntasks, tag, mode );

  return 0;
}
