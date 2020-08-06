/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019$
 * $PIP_VERSION: Version 1.0.0$
 * $PIP_license$
 */

#include <test.h>

#define NITERS		(1000)

int main( int argc, char **argv ) {
  pip_barrier_t	barr, *barrp;
  int 	ntasks, pipid;
  int 	niters = 0, i;
  volatile int	count, *countp;

  if( argc > 1 ) {
    niters = strtol( argv[1], NULL, 10 );
  }
  niters = ( niters <= 0 ) ? NITERS : niters;

  CHECK( pip_init( &pipid, &ntasks, NULL, 0 ),		       RV, return(EXIT_FAIL) );
  if( pipid == 0 ) {
    CHECK( pip_barrier_init( &barr, ntasks ), 		       RV, return(EXIT_FAIL) );
    barrp = &barr;
    CHECK( pip_named_export( (void*) barrp,  "BARRIER" ),      RV, return(EXIT_FAIL) );
    count = 0;
    countp = &count;
    CHECK( pip_named_export( (void*) countp, "COUNT" ),        RV, return(EXIT_FAIL) );
  } else {
    barrp = NULL;
    CHECK( pip_named_import( 0, (void**) &barrp,  "BARRIER" ), RV, return(EXIT_FAIL) );
    CHECK( barrp==NULL,					       RV, return(EXIT_FAIL) );
    countp = NULL;
    CHECK( pip_named_import( 0, (void**) &countp, "COUNT" ),   RV, return(EXIT_FAIL) );
    CHECK( countp==NULL,			               RV, return(EXIT_FAIL) );
  }
  for( i=0; i<niters; i++ ) {
    CHECK( *countp!=i,                 			       RV, return(EXIT_FAIL) );
    CHECK( pip_barrier_wait( barrp ), 			       RV, return(EXIT_FAIL) );
    if( ( i % ntasks ) == pipid ) (*countp) ++;
    CHECK( pip_barrier_wait( barrp ), 			       RV, return(EXIT_FAIL) );
  }
  if( pipid == 0 ) {
    CHECK( count==niters,   				      !RV, return(EXIT_FAIL) );
  }
  CHECK( pip_fin(), 					       RV, return(EXIT_FAIL) );
  return EXIT_PASS;
}
