/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019$
 * $PIP_VERSION: Version 1.0.0$
 * $PIP_license$
 */

//#define DEBUG

#define PIP_EXPERIMENTAL
#include <test.h>

#define NITERS		(100)

pip_barrier_t barr;

int main( int argc, char **argv ) {
  pip_barrier_t	*barrp = &barr;
  int 	ntasks, ntenv, pipid;
  int	niters, i, c;
  char	*env;

  ntasks = 0;
  if( argc > 1 ) {
    ntasks = strtol( argv[1], NULL, 10 );
  }
  ntasks = ( ntasks <= 0 ) ? NTASKS : ntasks;
  if( ( env = getenv( "NTASKS" ) ) != NULL ) {
    ntenv = strtol( env, NULL, 10 );
    if( ntasks > ntenv ) return(EXIT_UNTESTED);
  } else {
    if( ntasks > NTASKS ) return(EXIT_UNTESTED);
  }

  niters = 0;
  if( argc > 2 ) {
    niters = strtol( argv[2], NULL, 10 );
  }
  niters = ( niters <= 0 ) ? NITERS : niters;

  CHECK( pip_init(&pipid,&ntasks,(void**)&barrp,0), RV, return(EXIT_FAIL) );
  if( pipid == PIP_PIPID_ROOT ) {
    pip_spawn_program_t	prog;

    CHECK( pip_barrier_init( barrp, ntasks ), RV, return(EXIT_FAIL) );
    pip_spawn_from_main( &prog, argv[0], argv, NULL, NULL );
    for( i=0; i<ntasks; i++ ) {
      pipid = i;
      c = i % 10;
      CHECK( pip_blt_spawn( &prog, c, PIP_TASK_ACTIVE, &pipid,
			    NULL, NULL, NULL ),
	     RV,
	     return(EXIT_FAIL) );
    }
    for( i=0; i<ntasks; i++ ) {
      int status;
      CHECK( pip_wait_any(NULL,&status), RV, return(EXIT_FAIL) );
      if( WIFEXITED( status ) ) {
	CHECK( WEXITSTATUS( status ),  	 RV, return(EXIT_FAIL) );
      } else {
	CHECK( "Task is signaled",       RV, return(EXIT_UNRESOLVED) );
      }
    }

  } else {
    int nxt = ( pipid == ntasks -1 ) ? 0 : pipid + 1;
    pip_task_t *next;

    CHECK( pip_get_task_from_pipid(nxt,&next), RV, return(EXIT_FAIL) );

    CHECK( pip_barrier_wait( barrp ),          RV, return(EXIT_FAIL) );
    for( i=0; i<niters; i++ ) {
      CHECK( pip_migrate( next ),              RV, return(EXIT_FAIL) );
    }
    CHECK( pip_barrier_wait( barrp ),          RV, return(EXIT_FAIL) );
  }
  CHECK( pip_fin(),                            RV, return(EXIT_FAIL) );
  return 0;
}
