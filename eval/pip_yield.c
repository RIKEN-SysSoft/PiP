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
#define WITERS		(10)
#define NITERS		(10*1000)

#ifdef NTASKS
#undef NTASKS
#endif
#define NTASKS		(2)

typedef struct exp {
  pip_task_queue_t	queue;
} exp_t;

int bar(int);

int foo( int x ) {
  if( x > 0 ) x = bar( x - 1 );
  return x;
}

int bar( int x ) {
  if( x > 0 ) x = foo( x - 1 );
  return x;
}

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
    double   t, t0[NSAMPLES], t1[NSAMPLES];
    uint64_t c, c0[NSAMPLES], c1[NSAMPLES];
    int x;

    if( pipid < ntasks ) {
      for( j=0; j<NSAMPLES; j++ ) {
	for( i=0; i<witers; i++ ) {
	  pip_yield( PIP_YIELD_USER );
	}
	for( i=0; i<niters; i++ ) {
	  pip_yield( PIP_YIELD_USER );
	}
      }
    } else {

      for( j=0; j<NSAMPLES; j++ ) {
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
	c0[j] = get_cycle_counter() - c;
	t0[j] = pip_gettime() - t;

	for( i=0; i<witers; i++ ) {
	  c1[j] = 0;
	  t1[j] = 0.0;
	  pip_gettime();
	  get_cycle_counter();
	  x = foo( NTASKS*100 );
	}
	t = pip_gettime();
	c = get_cycle_counter();
	for( i=0; i<niters; i++ ) {
	  x = foo( NTASKS*100 );
	}
	c1[j] = get_cycle_counter() - c;
	t1[j] = pip_gettime() - t;
      }
      double dn = (double) ( niters + NTASKS );
      for( j=0; j<NSAMPLES; j++ ) {
	printf( "pip_yield : %g  (%lu)\n", t0[j] / dn,             c0[j] / (niters*NTASKS) );
	//printf( "funcall   : %g  (%lu)\n", t1[j] / ( dn * 100.0 ), c1[j] / (niters*NTASKS*100) );
      }
      printf( " -- dummy (%d)\n", x );
    }
  }
  CHECK( pip_fin(), RV, return(EXIT_FAIL) );
  return 0;
}
