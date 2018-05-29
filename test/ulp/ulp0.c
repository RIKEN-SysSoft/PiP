/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#define PIP_INTERNAL_FUNCS

//#define NULPS	(10)
#define NULPS	(10)
//#define NULPS	(0)

#define DEBUG
#include <test.h>
#include <pip_ulp.h>

int a;

static struct root{
  int 		pipid;
} root;

int main( int argc, char **argv ) {
  int i, ntasks, pipid;

  //fprintf( stderr, "PID %d\n", getpid() );

  //ntasks = NULPS*2;
  ntasks = 100;
  TESTINT( pip_init( &pipid, &ntasks, NULL, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    pip_ulp_t *ulp  = NULL;
    root.pipid = pipid;
    for( i=0; i<NULPS; i++ ) {
      pipid = i + 1;
      TESTINT( pip_ulp_new( argv[0],
			    argv,
			    NULL,
			    &pipid,
			    ulp,
			    &ulp ) );
    }
    pipid = 0;
    TESTINT( pip_spawn_with_ulps( argv[0], argv, NULL, 0, &pipid,
				  NULL, NULL, NULL, ulp ) );
    TESTINT( pip_wait( pipid, NULL ) );
  } else {
    fprintf( stderr, "\n<%d> Hello from ULP (stackvar@%p staticvar@%p)\n\n",
	     pipid, &pipid, &root );
    if( pipid == 0 ) pip_ulp_retire( pipid );
  }
  TESTINT( pip_fin() );
  return 0;
}
