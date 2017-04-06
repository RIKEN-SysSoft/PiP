#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include <pip.h>

#define NTASKS	(10)

int main( int argc, char **argv ) {
  pid_t sid_old, sid_new;
  int  i, ntasks, pipid;

  ntasks = NTASKS;

  sid_old = getsid( getpid() );

  pip_init( &pipid, &ntasks, NULL, PIP_MODE_PROCESS );
  if( pipid == PIP_PIPID_ROOT ) {
    if( ( sid_new = setsid() ) < 0 ) {
      printf( "ROOT: setsid(): %d\n", errno );
    }
    printf( "ROOT[%d]: sid %d -> %d\n", getpid(), sid_old, sid_new );
    for( i=0; i<ntasks; i++ ) {
      pipid = i;
      pip_spawn( argv[0], argv, NULL, i, &pipid, NULL, NULL, NULL );
    }
    for( i=0; i<ntasks; i++ ) wait( NULL );
    sid_new = getsid( getpid() );
    printf( "ROOT[%d]: sid %d\n", getpid(), sid_new );
    printf( "all done\n" );

  } else {	/* PIP child task */
    if( ( sid_new = setsid() ) < 0 ) {
      printf( "CHILD: setsid(): %d\n", errno );
    }
    printf( "CHILD[%d]: sid %d -> %d\n", getpid(), sid_old, sid_new );
  }
  pip_fin();
  return 0;
}
