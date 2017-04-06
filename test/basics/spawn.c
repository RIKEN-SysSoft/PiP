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

#include <pip.h>
#include <test.h>

#define NTASKS_MAX	(100)

double	time_start, time_spawn, time_end;

void spawn_tasks( int ntasks ) {
  char *argv[] = { "./null", NULL };
  int pipid, i;

  TESTINT( pip_init( &pipid, &ntasks, NULL, 0 ) );
  TESTINT( ( pipid != PIP_PIPID_ROOT ) );

  time_start = pip_gettime();
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
  time_spawn = pip_gettime();
  for( i=0; i<ntasks; i++ ) {
    TESTINT( pip_wait( i, NULL ) );
  }
  time_end = pip_gettime();

  TESTINT( pip_fin() );
}

int main( int argc, char **argv ) {
  int ntasks;

  if( argc < 2 ) {
    fprintf( stderr, "spawn-XXX <ntasks>\n" );
    exit( 1 );
  }
  ntasks = atoi( argv[1] );
  if( ntasks > NTASKS_MAX ) {
    fprintf( stderr, "Illegal number of tasks is specified.\n" );
    exit( 1 );
  }

  spawn_tasks( ntasks );
  printf( "%g  %g  [sec] %d tasks\n",
	  time_spawn - time_start, time_end - time_spawn, ntasks );

  return 0;
}
