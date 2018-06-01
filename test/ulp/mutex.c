/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#define PIP_INTERNAL_FUNCS

#include <test.h>
#include <pip_ulp.h>

//#define DEBUG

#ifdef DEBUG
# ifdef NTASKS
#  undef NTASKS
#  define NTASKS	(10)
# endif
#else
# ifdef NTASKS
#  undef NTASKS
#  define NTASKS	(250)
# endif
#endif

struct expo {
  pip_ulp_mutex_t mutex;
  int 		  x;
} expo;

int main( int argc, char **argv ) {
  struct expo	*expop;
  int		ntasks, nulps, pipid;
  int 		i;

  if( argc > 1 ) {
    ntasks = atoi( argv[1] );
  } else {
    ntasks = NTASKS;
  }
  if( ntasks < 2 ) {
    fprintf( stderr,
	     "Too small number of PiP tasks (must be latrger than 3)\n" );
  }
  if( ntasks >= 256 ) {
    fprintf( stderr,
	     "Too many number of PiP tasks (must be less than 256)\n" );
  }
  nulps = ntasks - 1;

  if( !pip_isa_piptask() ) {
    TESTINT( pip_ulp_mutex_init( &expo.mutex ) );
    expo.x = 0;
    expop = &expo;
  }

  TESTINT( pip_init( &pipid, &ntasks, (void**) &expop, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    pip_spawn_program_t prog;
    pip_ulp_t *ulp = NULL;

    pip_spawn_from_main( &prog, argv[0], argv, NULL );
    for( i=0; i<nulps; i++ ) {
      pipid = i;
      TESTINT( pip_ulp_new( &prog, &pipid, ulp, &ulp ) );
    }
    pipid = i;
    TESTINT( pip_task_spawn( &prog, PIP_CPUCORE_ASIS, &pipid, NULL, ulp ));
    for( i=0; i<ntasks; i++ ) {
      TESTINT( pip_wait( i, NULL ) );
    }
    if( expop->x == ntasks ) {
      fprintf( stderr, "Succeeded\n" );
    } else {
      fprintf( stderr, "FAILED (%d!=%d) !!!!\n", expop->x, ntasks );
    }
  } else {
    int xx;

#ifndef DEBUG
    /* Disturbances */
    int nyields = rand() % ntasks;
    for( i=0; i<nyields; i++ ) TESTINT( pip_ulp_yield() );
#endif

    TESTINT( pip_ulp_mutex_lock( &expop->mutex ) );
    xx = expop->x;

#ifndef DEBUG
    /* Disturbances */
    nyields = rand() % ntasks;
    for( i=0; i<nyields; i++ ) TESTINT( pip_ulp_yield() );
#endif

    expop->x = xx + 1;
    TESTINT( pip_ulp_mutex_unlock( &expop->mutex ) );

    //if( pip_is_task() ) (void) pip_ulp_wait_sisters();
  }
  TESTINT( pip_fin() );
  return 0;
}
