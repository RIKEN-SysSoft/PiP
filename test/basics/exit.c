/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2016
*/

//#define DEBUG
#include <test.h>

int root_exp = 0;

int main( int argc, char **argv ) {
  int pipid, ntasks;
  int i;
  int err;

  ntasks = NTASKS;
  TESTINT( pip_init( &pipid, &ntasks, NULL, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    for( i=0; i<NTASKS; i++ ) {
      int retval;

      pipid = i;
      err = pip_spawn( argv[0], argv, NULL, i % cpu_num_limit(),
		       &pipid, NULL, NULL, NULL );
      if( err ) {
	fprintf( stderr, "pip_spawn(%d/%d): %s\n",
		 i, NTASKS, strerror( err ) );
	break;
      }

      if( i != pipid ) {
	fprintf( stderr, "pip_spawn(%d!=%d)=%d !!!!!!\n", i, pipid, err );
	break;
      }
      DBGF( "calling pip_wait(%d)", i );
      TESTINT( pip_wait( i, &retval ) );
      if( retval != ( i & 0xFF ) ) {
	fprintf( stderr, "[PIPID=%d] pip_wait() returns %d ???\n", i, retval );
      } else {
	fprintf( stderr, " terminated. OK\n" );
      }
    }
    TESTINT( pip_fin() );

  } else {
    fprintf( stderr, "Hello, I am PIPID[%d/%d] ...", pipid, getpid() );
    pip_exit( pipid );
  }
  return 0;
}
