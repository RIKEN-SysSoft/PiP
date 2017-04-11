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

char *tasks[] = { "./a", "./b", "./c", NULL };

int root_exp = 0;

int main( int argc, char **argv ) {
  char *newav[3];
  char pipid_str[64];
  void *exp = (void*) &root_exp;
  int pipid, ntasks;
  int i, j;
  int err;

  ntasks = NTASKS;
  TESTINT( pip_init( &pipid, &ntasks, &exp, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    for( i=0, j=0; i<NTASKS; i++ ) {
      if( tasks[j] == NULL ) j = 0;
      sprintf( pipid_str, "%d", i );
      newav[0] = tasks[j++];
      newav[1] = pipid_str;
      newav[2] = NULL;
      pipid = i;
      err = pip_spawn( newav[0], newav, NULL, i%4, &pipid, NULL, NULL, NULL );
      if( err != 0 ) break;

      if( i != pipid ) {
	fprintf( stderr, "pip_spawn(%d!=%d)=%d !!!!!!\n", i, pipid, err );
	break;
      }
    }
    ntasks = i;

    //printf( "%d\n", ntasks );
    for( i=0; i<ntasks; i++ ) {
      DBGF( "caling pip_wait(%d)", i );
      TESTINT( pip_wait( i, NULL ) );
    }
    TESTINT( pip_fin() );
  } else {
    /* should not reach here */
    fprintf( stderr, "ROOT: NOT ROOT ????\n" );
  }
  return 0;
}
