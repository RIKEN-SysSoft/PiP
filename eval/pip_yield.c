 /*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019$
 * $PIP_VERSION: Version 1.0.0$
 * $PIP_license$
 */

//#define DEBUG

#include <eval.h>
#include <test.h>

#define NSAMPLES	(10)
#define WITERS		(1000*1000)
#define NITERS		(10*1000)

#ifdef NTASKS
#undef NTASKS
#endif
#define NTASKS		(2)

typedef struct exp {
  pip_task_queue_t	queue;
} exp_t;

int main( int argc, char **argv ) {
  pip_spawn_program_t	prog;
  exp_t	exprt, *expp;
  int 	ntasks, pipid;
  int	witers = WITERS, niters = NITERS;
  int	status, i, j, extval = 0;

  ntasks = NTASKS - 1;

  pip_spawn_from_main( &prog, argv[0], argv, NULL );
  expp = &exprt;

  CHECK( pip_init(&pipid,NULL,(void*)&expp,PIP_MODE_PROCESS), RV,
	 return(EXIT_FAIL) );
  if( pipid == PIP_PIPID_ROOT ) {
    CHECK( pip_task_queue_init(&expp->queue ,NULL), RV, return(EXIT_FAIL) );

    for( i=0; i<ntasks; i++ ) {
      pipid = i;
      CHECK( pip_blt_spawn( &prog, 0, PIP_TASK_PASSIVE, &pipid,
			    NULL, &expp->queue, NULL ),
	     RV,
	     abend(EXIT_UNTESTED) );
    }
    pipid = i;
    CHECK( pip_blt_spawn( &prog, 1, PIP_TASK_ACTIVE, &pipid,
			  NULL, &expp->queue, NULL ),
	   RV,
	   abend(EXIT_UNTESTED) );

    for( i=0; i<ntasks+1; i++ ) {
      CHECK( pip_wait_any(NULL,&status), RV, return(EXIT_FAIL) );
      if( WIFEXITED( status ) ) {
	CHECK( ( extval = WEXITSTATUS( status ) ),
	       RV,
	       return(EXIT_FAIL) );
      } else {
	CHECK( "Task is signaled", RV, return(EXIT_UNRESOLVED) );
      }
    }
  } else {
    double   t, t0[NSAMPLES];
    double   dn = (double) ( niters * NTASKS );
    uint64_t c, c0[NSAMPLES];

    if( pipid == 0 ) {		/* passive */
      for( j=0; j<NSAMPLES; j++ ) {
	for( i=0; i<witers; i++ ) {
	  pip_yield( PIP_YIELD_USER );
	}
	for( i=0; i<niters; i++ ) {
	  pip_yield( PIP_YIELD_USER );
	}
      }
    } else {
      for( j=0; j<NSAMPLES; j++ ) { /* active */
	for( i=0; i<witers; i++ ) {
	  c0[j] = 0;
	  t0[j] = 0.0;
	  pip_gettime();
	  get_cycle_counter();
	  pip_yield( PIP_YIELD_USER );
	}
	t = pip_gettime();
	c = get_cycle_counter();
	for( i=0; i<niters; i++ ) {
	  pip_yield( PIP_YIELD_USER );
	}
	c0[j] = ( get_cycle_counter() - c ) / ( niters * NTASKS );
	t0[j] = ( pip_gettime() - t ) / dn;
      }
      double min = t0[0];
      int    idx = 0;
      for( j=0; j<NSAMPLES; j++ ) {
	printf( "[%d] pip_yield : %g  (%lu)\n", j, t0[j], c0[j] );
	if( min > t0[j] ) {
	  min = t0[j];
	  idx = j;
	}
      }
      printf( "[[%d]] pip_yield : %.3g  (%lu)\n", idx, t0[idx], c0[idx] );
    }
  }
  CHECK( pip_fin(), RV, return(EXIT_FAIL) );
  return 0;
}
