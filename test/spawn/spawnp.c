/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2016
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include <test.h>

#define NTASKS	(20)

int main( int argc, char **argv ) {
  int pipid, ntasks;
  int i;

  ntasks = NTASKS;
  pip_init( &pipid, &ntasks );
  if( pipid == PIP_PIPID_ROOT ) {
    fprintf( stderr,
	     "[PID=%d] Hello, I am ROOT! -- env=%p:%p/%p(%s/%s)\n",
	     getpid(), &environ, environ, environv, environ[0], environv[0] );
    for( i=0; i<NTASKS; i++ ) {
      void *exp;

      pipid = i;
      TESTINT( pip_spawn( &pipid, i, argv[0], argv, environ ) );
      fprintf( stderr, "spanwed[%d:%d]\n", i, pipid );

      do {
	TESTINT( pip_import( i, &exp ) );
	usleep( 100000 );
	sched_yield();
      } while( exp == NULL );
      fprintf( stderr, "EXPORT[%d]: %p\n", i, exp );
      *((int*)exp) = 1;

      fprintf( stderr, "join[%d] ... ", i );
      TESTINT( pip_join( i ) );
      fprintf( stderr, " ...done\n" );
      //sleep( 1 );
      print_maps();
    }
    TESTINT( pip_fin() );
  } else {
    volatile int a = 0;

    fprintf( stderr,
	     "[PID=%d] Hello, my PIPID is %d (exp=%p) -- env=%p:%p(%s)\n",
	     getpid(), pipid, &a, &environ, environ, environ[0] );

    TESTINT( pip_export( (void*)&a ) );
    //print_maps();

    fprintf( stderr, "waiting..." );
    while( a == 0 ) usleep( 100 );
    fprintf( stderr, " ...done\n" );
  }
  return 0;
}
