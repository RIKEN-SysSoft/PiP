/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#define PIP_INTERNAL_FUNCS

#define NULPS		(NTASKS-10)
//#define NULPS	(10)
//#define NULPS	(3)

//#define DEBUG
#include <test.h>
#include <pip_ulp.h>

int a;

static struct root{
  int 		pipid;
} root;

int main( int argc, char **argv ) {
  pip_spawn_program_t prog;
  int ntasks, nulps;
  int i, pipid;

  if( argc   > 1 ) {
    ntasks = atoi( argv[1] );
  } else {
    ntasks = NTASKS;
  }
  if( ntasks < 2 ) {
    fprintf( stderr,
	     "Too small number of PiP tasks (must be latrger than 1)\n" );
  }
  nulps = ntasks - 1;

  pip_spawn_from_main( &prog, argv[0], argv, NULL );

  TESTINT( pip_init( &pipid, NULL, NULL, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {

    pip_ulp_t *ulp = NULL;
    root.pipid = pipid;
    for( i=0; i<nulps; i++ ) {
      pipid = i + 1;
      TESTINT( pip_ulp_new( &prog, &pipid, ulp, &ulp ) );
    }
    pipid = 0;
    TESTINT( pip_task_spawn( &prog, PIP_CPUCORE_ASIS, &pipid, NULL, ulp ));
    for( i=0; i<ntasks; i++ ) {
      TESTINT( pip_wait( i, NULL ) );
    }
  } else {
    pip_ulp_t *ulp;

    TESTINT( pip_ulp_myself( &ulp ) );
    char *type = pip_type_str();
    fprintf( stderr,
#ifndef DEBUG
	     "<%d> Hello, %s (stackvar@%p staticvar@%p)\n",
#else
	     "\n<%d> Hello, %s (stackvar@%p staticvar@%p)\n\n",
#endif
	     pipid, type, &pipid, &root );
    //pip_exit( 0 );
  }
  TESTINT( pip_fin() );
  return 0;
}
