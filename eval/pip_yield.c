/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019$
 * $PIP_VERSION: Version 1.0.0$
 * $PIP_license$
 */

//#define DEBUG

#include <test.h>

#define WITERS		(10)
#define NITERS		(1000)

#ifdef NTASKS
#undef NTASKS
#endif
#define NTASKS		(100)

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
  double t0, t1;

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
    if( pipid < ntasks ) {
      for( j=0; j<10; j++ ) {
	for( i=0; i<witers; i++ ) {
	  pip_yield( PIP_YIELD_USER );
	}
	for( i=0; i<niters; i++ ) {
	  pip_yield( PIP_YIELD_USER );
	}
      }
    } else {
      int x;

      for( j=0; j<10; j++ ) {
	for( i=0; i<witers; i++ ) {
	  pip_gettime();
	  pip_yield( PIP_YIELD_USER );
	}
	t0 = - pip_gettime();
	for( i=0; i<niters; i++ ) {
	  pip_yield( PIP_YIELD_USER );
	}
	t0 += pip_gettime();

	for( i=0; i<witers; i++ ) {
	  pip_gettime();
	  x = foo( NTASKS*100 );
	}
	t1 = - pip_gettime();
	for( i=0; i<niters; i++ ) {
	  x = foo( NTASKS*100 );
	}
	t1 += pip_gettime();

	printf( "pip_yield : %g\n", t0 / ((double) niters*NTASKS) );
	printf( "funcall   : %g  (%d)\n", t1 / ((double) niters*NTASKS*100), x );
      }
    }
  }
  CHECK( pip_fin(), RV, return(EXIT_FAIL) );
  return 0;
}
