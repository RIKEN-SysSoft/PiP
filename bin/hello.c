
#define _GNU_SOURCE
#include <sys/types.h>
#include <sched.h>
#include <unistd.h>
#include <stdio.h>

int main( int argc, char **argv ) {
#ifdef AH
  cpu_set_t cpuset;
  char cpustr[128], *p;

  p = cpustr;
  *p = '\0';
  if( sched_getaffinity( getpid(), sizeof(cpuset), &cpuset ) == 0 ) {
    int ncpus = CPU_COUNT( &cpuset );
    int i, nc = 0;

    for( i=0; sizeof(cpuset)*8; i++ ) {
      if( CPU_ISSET( i, &cpuset ) ) {
	sprintf( p, "%d.", i );
	p = strlen( cpustr ) + cpustr;
	if( ++nc > 3 ) break;
      }
    }
  }
#endif
  if( argc > 1 ) {
    printf( "Hello (PID:%d,CPU:%d) : '%s'\n", getpid(), sched_getcpu(), argv[1] );
  } else {
    printf( "Hello (PID:%d,CPU:%d)\n", getpid(), sched_getcpu() );
  }
  return 0;
}
