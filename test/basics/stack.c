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

pip_clone_t *pip_get_cloneinfo( void );

void print_stack( int pipid ) {
  int stack;
  pip_clone_t *info = pip_get_cloneinfo();
  fprintf( stderr, "[%d] clone-stack=%p  stack=%p  %d\n",
	   pipid, info->stack, &stack,
	   (int) (((intptr_t) info->stack) - ((intptr_t)&stack)) );
}

int main( int argc, char **argv ) {
  int pipid = 999;
  int ntasks;
  int i;
  int err;

  ntasks = NTASKS;
  TESTINT( pip_init( &pipid, &ntasks, NULL, PIP_MODEL_PROCESS ) );
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
