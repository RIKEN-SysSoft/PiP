/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2016
*/

#include <test.h>

int a[100];

int main( int argc, char **argv, char **envv ) {
  void *exp;
  int pipid, ntasks;
  int i;

  a[0] = 123456;

  ntasks = NTASKS;
  exp = (void*) a;
  TESTINT( pip_init( &pipid, &ntasks, &exp, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    for( i=0; i<NTASKS; i++ ) {
      int err;
      pipid = i;
      if( ( err = pip_spawn( argv[0], argv, NULL, i%4, &pipid ) ) != 0 ) break;
      if( i != pipid ) {
	fprintf( stderr, "pip_spawn(%d!=%d) !!!!!!\n", i, pipid );
      }
    }
    ntasks = i;
    for( i=0; i<ntasks; i++ ) {
      //fprintf( stderr, "[ROOT] >> import[%d]\n", i );
      while( 1 ) {
	TESTINT( pip_import( i, &exp ) );
	if( exp != NULL ) break;
	pause_and_yield( 0 );
      }
      DBG;
      //fprintf( stderr, "[ROOT] << import[%d]: %d\n", i, *(int*)exp );
    }
    DBG;
    for( i=0; i<ntasks; i++ ) {
      a[i+1] = i + 100;
      TESTINT( pip_wait( i, NULL ) );
    }
    TESTINT( pip_fin() );
  } else {
    void *root;

    a[0] = pipid * 10;
    TESTINT( pip_export( (void*) a ) );
    while( 1 ) {
      TESTINT( pip_import( PIP_PIPID_ROOT, &root ) );
      if( root != NULL ) break;
      pause_and_yield( 0 );
    }
    DBG;
    //fprintf( stderr, "[%d] import[ROOT]: %d\n", pipid, *(int*)root );
    while( 1 ) {
      if( ((int*)root)[pipid+1] == pipid + 100 ) break;
      pause_and_yield( 0 );
    }
    fprintf( stderr, "[PID=%d] Hello, my PIPID is %d\n", getpid(), pipid );
  }
  DBG;
  return 0;
}
