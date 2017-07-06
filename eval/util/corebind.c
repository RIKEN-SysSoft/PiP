/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2017
 */

#define _GNU_SOURCE

#include <sched.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

int main( int argc, char **argv ) {
  extern char **environ;
  cpu_set_t cpuset;
  int coreno;
  int retval = 0;

  if( argv[1] == NULL || *argv[1] == '\0' ) {
    fprintf( stderr, "No core number specified\n" );
    retval = EINVAL;
  } else if( argv[2] == NULL || *argv[2] == '\0' ) {
    fprintf( stderr, "No command\n" );
    retval = EINVAL;
  } else {
    coreno = atoi( argv[1] );
    CPU_ZERO( &cpuset );
    CPU_SET( coreno, &cpuset );
    if( sched_setaffinity( 0, sizeof(cpuset), &cpuset ) != 0 ) {
      retval = errno;
    } else {
      execve( argv[2], &argv[2], environ );
      fprintf( stderr, "execve(%s) faild (%d)\n", argv[2], errno );
      retval = errno;
    }
  }
  return retval;
}
