/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2016
*/

#include <test.h>

static int pipid;

int main( int argc, char **argv ) {
  char *prog = argv[0];
  volatile int *root_exp;
  int ntasks;

  if( argc < 2 ) {
    fprintf( stderr, "%s: argc=%d\n", prog, argc );
  } else {
    TESTINT( pip_init( &pipid, &ntasks, (void**) &root_exp, 0 ) );

    if( pipid == PIP_PIPID_ROOT ) {
      fprintf( stderr, "%s: ROOT ????\n", prog );
    } else if( pipid != atoi( argv[1] ) ) {
      fprintf( stderr, "%s: %d != %s\n", prog, pipid, argv[1] );
    } else {
      TESTINT( pip_export( (void*) &pipid ) );
      while( 1 ) {
	pip_import( PIP_PIPID_ROOT, (void**) &root_exp );
	if( root_exp != NULL ) break;
	pause_and_yield( 100 );
      }
      while( !*root_exp ) pause_and_yield( 100 );
      fprintf( stderr, "%s#%d -- Hello, I am fine !\n", prog, pipid );
    }
  }
  return 0;
}
