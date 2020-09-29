/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019, 2020$
 * $PIP_VERSION: Version 3.0.0$
 * $PIP_license$
 */

#include <test.h>

#define NITERS		(100)

#define MAGIC		(12345)


int main( int argc, char **argv ) {
#ifdef BARRIER
  pip_barrier_t barrier;
  pip_barrier_t	*barrp;
#endif
  int 	pipid, ntasks, prev, next;
  int 	niters = 0, i;
  int 	count, *counts, *countp;

  if( argc > 1 ) {
    niters = strtol( argv[1], NULL, 10 );
  }
  niters = ( niters <= 0 ) ? NITERS : niters;

  CHECK( pip_init( &pipid, &ntasks, NULL, 0 ), RV, return(EXIT_FAIL)     );
  CHECK( pipid==PIP_PIPID_ROOT,                RV, return(EXIT_UNTESTED) );
  CHECK( ntasks<2,                             RV, return(EXIT_UNTESTED) );

  prev = pipid - 1;
  prev = ( prev < 0 ) ? ntasks - 1 : prev;
  next = pipid + 1;
  next = ( next == ntasks ) ? 0 : next;

  CHECK( pip_named_import( pipid, (void**) &countp, "must not be found" ),
	 RV!=EDEADLK, return(EXIT_FAIL) );
  count = MAGIC;
  CHECK( pip_named_export( (void*) &count, "must be found" ),
	 RV, return(EXIT_FAIL) );
  CHECK( pip_named_import( pipid, (void**) &countp, "must be found" ),
	 RV, return(EXIT_FAIL) );
  CHECK( *countp!=MAGIC, RV, return(EXIT_FAIL) );

  CHECK( counts=(int*)malloc(sizeof(int)*niters), RV==0, return(EXIT_UNTESTED) );
  for( i=0; i<niters; i++ ) {
    counts[i] = i + MAGIC;
  }
#ifdef BARRIER
  if( pipid == 0 ) {
    barrp = &barrier;
    CHECK( pip_barrier_init( barrp, ntasks ), RV, return(EXIT_FAIL) );
    CHECK( pip_named_export( (void*) &barrier, "barrier" ),
	   RV, return(EXIT_FAIL) );
  } else {
    CHECK( pip_named_import( 0, (void**) &barrp, "barrier" ),
	   RV, return(EXIT_FAIL) );
  }
  CHECK( pip_barrier_wait( barrp ), RV, return(EXIT_FAIL) );
#endif
  for( i=0; i<niters; i++ ) {
    if( pipid == 0 ) {
      CHECK( pip_named_export( (void*) &counts[i], "forward:%d", i ),
	     RV, return(EXIT_FAIL) );
      CHECK( pip_named_export( (void*) &counts[i], "backward:%d", i ),
	     RV, return(EXIT_FAIL) );
      CHECK( pip_named_import( next, (void**) &countp, "forward:%d", i ),
	     RV, return(EXIT_FAIL) );
      CHECK( *countp!=i+MAGIC, RV, return(EXIT_FAIL) );
      CHECK( pip_named_import( prev, (void**) &countp, "backward:%d", i ),
	     RV, return(EXIT_FAIL) );
      CHECK( *countp!=i+MAGIC, RV, return(EXIT_FAIL) );
    } else {
      CHECK( pip_named_import( prev, (void**) &countp, "forward:%d", i ),
	     RV, return(EXIT_FAIL) );
      CHECK( *countp!=i+MAGIC, RV, return(EXIT_FAIL) );
      CHECK( pip_named_export( (void*) &counts[i], "forward:%d", i ),
	     RV, return(EXIT_FAIL) );
      CHECK( pip_named_export( (void*) &counts[i], "backward:%d", i ),
	     RV, return(EXIT_FAIL) );
      CHECK( pip_named_import( next, (void**) &countp, "backward:%d", i ),
	     RV, return(EXIT_FAIL) );
      CHECK( *countp!=i+MAGIC, RV, return(EXIT_FAIL) );
    }
  }
#ifdef BARRIER
  CHECK( pip_barrier_wait( barrp ), RV, return(EXIT_FAIL) );
#endif
  CHECK( pip_fin(), RV, return(EXIT_FAIL) );
  return EXIT_PASS;
}
