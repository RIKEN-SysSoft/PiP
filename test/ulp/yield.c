/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#define PIP_INTERNAL_FUNCS

#define NULPS	(NTASKS-10)
//#define NULPS	(10)
//#define NULPS	(3)

//#define NYIELDS	(10*1000)
#define NYIELDS	(1*1000)

//#define DEBUG
#define PIP_EVAL

#include <test.h>
#include <pip_ulp.h>

int main( int argc, char **argv ) {
  int ntasks = 0;
  int nulps;
  int i, pipid;

  if( argc   > 1 ) {
    ntasks = atoi( argv[1] );
  } else {
    ntasks = NTASKS;
  }
  if( ntasks < 2 ) {
    fprintf( stderr,
	     "Too small number of PiP tasks (must be latrger than 1)\n" );
    exit( 1 );
  }
  nulps = ntasks - 1;

  TESTINT( pip_init( &pipid, &ntasks, NULL, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    pip_spawn_program_t prog;
    pip_ulp_t ulps;

    PIP_ULP_INIT( &ulps );
    pip_spawn_from_main( &prog, argv[0], argv, NULL );
    for( i=0; i<nulps; i++ ) {
      pipid = i;
      TESTINT( pip_ulp_new( &prog, &pipid, &ulps, NULL ) );
    }
    pipid = i;
    TESTINT( pip_task_spawn( &prog, PIP_CPUCORE_ASIS, &pipid, NULL, &ulps ) );

    for( i=0; i<nulps+1; i++ ) {
      TESTINT( pip_wait( i, NULL ) );
    }
  } else {
    double time_yield = 0.0;
    double one_yield;
    for( i=0; i<NYIELDS; i++ ) {
      PIP_ACCUM( time_yield, pip_ulp_yield()==0 );
    }
    one_yield = time_yield;
    one_yield /= ((double) NYIELDS ) * ((double)(nulps+1));
    if( pip_is_task() ) {
      fprintf( stderr,
	       "Hello, this is TASK (%d)  "
	       "[one yield() takes %g usec / %g sec]\n",
	       pipid,
	       one_yield*1e6, time_yield );
    } else if( pip_is_ulp() ) {
      fprintf( stderr,
	       "Hello, this is ULP (%d)  "
	       "[one yield() takes %g usec / %g sec]\n",
	       pipid,
	       one_yield*1e6, time_yield );
    }
  }
  TESTINT( pip_fin() );
  return 0;
}
