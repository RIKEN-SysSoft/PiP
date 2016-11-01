/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
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

#if defined(FORK_EXEC)
#include <sys/wait.h>
#endif

#define NTASKS_MAX	(50)

double	time_start, time_spawn, time_end;

#ifdef PIP
void spawn_tasks( int ntasks ) {
  char *argv[] = { "./foo", NULL };
  int pipid, i;

  TESTINT( pip_init( &pipid, &ntasks, NULL, PIP_MODEL_PROCESS ) );
  TESTINT( ( pipid != PIP_PIPID_ROOT ) );

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

#if defined( PIP )
  spawn_tasks( ntasks );
  tag = "PIP";
#elif defined( FORK_EXEC )
  fork_exec( ntasks );
  tag = "FORK_EXEC";
#elif defined( THREAD )
  create_threads( ntasks );
  tag = "PTHREADS";
#endif
  printf( "%g  %g  [sec] %d tasks -- %s\n",
	  time_spawn - time_start, time_end - time_spawn, ntasks, tag );

  return 0;
}
