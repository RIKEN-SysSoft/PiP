/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

//#define DEBUG

#define PIP_INTERNAL_FUNCS
#include <test.h>
#include <pip_internal.h>

pip_clone_t *pip_get_cloneinfo_( void );

void print_stack( int pipid ) {
  pip_clone_t *info = pip_get_cloneinfo_();
  if( info != NULL ) {
    int stack;
    fprintf( stderr, "[%d] clone-stack=%p  stack=%p  %d\n",
	     pipid, info->stack, &stack,
	     (int) (((intptr_t) info->stack) - ((intptr_t)&stack)) );
  } else {
    int mode;
    TESTINT( pip_get_mode( &mode ) );
    if( mode & PIP_MODE_PTHREAD ) {
      fprintf( stderr, "[%d] PTHREAD execution mode\n", pipid );
    } else {
      fprintf( stderr, "[%d] Unable to get stack address\n", pipid );
    }
  }
}

int main( int argc, char **argv ) {
  int pipid = 999;
  int ntasks;
  int i;
  int err;

  ntasks = NTASKS;
  TESTINT( pip_init( &pipid, &ntasks, NULL, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    for( i=0; i<NTASKS; i++ ) {
      pipid = i;
      err = pip_spawn( argv[0], argv, NULL, i%8, &pipid, NULL, NULL, NULL );
      if( err != 0 ) break;
      if( i != pipid ) {
	fprintf( stderr, "pip_spawn(%d!=%d) !!!!!!\n", i, pipid );
      }
    }
    ntasks = i;

    for( i=0; i<ntasks; i++ ) TESTINT( pip_wait( i, NULL ) );
    TESTINT( pip_fin() );

  } else {
    print_stack( pipid );
  }
  return 0;
}
