/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#define PIP_INTERNAL_FUNCS

#define NULPS	(10)
//#define NULPS	(2)

#define DEBUG
#include <test.h>

int a;

static struct root{
  pip_ulp_t 	root;
  int 		pipid;
  int		n;
  pip_ulp_t	ulp[NULPS];
} root;

void term_cb( void *aux ) {
  struct root *rp = (struct root*) aux;
  fprintf( stderr, "PIPID:%d - terminated (%p)\n", rp->pipid, rp );
  rp->n++;
  if( rp->n < NULPS ) {
    pip_ulp_yield_to( NULL, &rp->ulp[rp->n] );
  } else {
    pip_ulp_yield_to( NULL, &rp->root );
  }
}

int main( int argc, char **argv ) {
  int i, ntasks, pipid;

  fprintf( stderr, "PID %d\n", getpid() );

  ntasks = 20;
  TESTINT( pip_init( &pipid, &ntasks, NULL, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    root.pipid = pipid;
    root.n     = 0;
    TESTINT( pip_make_ulp( pipid,
			   term_cb,
			   &root,
			   &root.root ) );
    for( i=0; i<NULPS; i++ ) {
      pipid = i;
      TESTINT( pip_ulp_spawn( argv[0],
			      argv,
			      NULL,
			      &pipid,
			      term_cb,
			      &root,
			      &root.ulp[i] ) );
    }
    TESTINT( pip_ulp_yield_to( &root.root, &root.ulp[0] ) );
  } else {
    fprintf( stderr, "<%d> Hello, ULP (stackvar@%p staticvar@%p)\n",
	     pipid, &pipid, &root );
  }
  TESTINT( pip_fin() );
  return 0;
}
