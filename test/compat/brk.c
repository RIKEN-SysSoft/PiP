/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#include <unistd.h>
#include <test.h>

void print_brk( int id ) {
  printf( "<%d> brk: %p\n", id, sbrk( 0 ) );
#if defined(PIP_BUG_NO_FFLUSH_AT_MAIN_RETURN)
  fflush( stdout );
#endif
}

int main( int argc, char **argv ) {
  int pipid = 999;
  int ntasks;
  int i, err;

  ntasks = NTASKS;
  TESTINT( pip_init( &pipid, &ntasks, NULL, 0 ) );

  print_brk( pipid );

  if( pipid == PIP_PIPID_ROOT ) {
    for( i=0; i<NTASKS; i++ ) {
      pipid = i;
      err = pip_spawn( argv[0], argv, NULL, i%4, &pipid, NULL, NULL, NULL );
      if( err ) break;
      if( i != pipid ) {
	fprintf( stderr, "pip_spawn(%d!=%d)=%d !!!!!!\n", i, pipid, err );
	break;
      }
    }
    ntasks = i;
    for( i=0; i<ntasks; i++ ) {
      TESTINT( pip_wait( i, NULL ) );
    }
  }
  TESTINT( pip_fin() );
  return 0;
}
