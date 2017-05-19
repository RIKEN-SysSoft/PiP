#include <sys/wait.h>
#include <stdio.h>
#include <pip.h>

#ifdef PRINT_MAPS
#include <pip_debug.h>
#endif

int pipid, ntasks;

int main( int argc, char **argv ) {

  ntasks = 10;
  pip_init( &pipid, &ntasks, NULL, 0 );

  if( pipid == PIP_PIPID_ROOT ) { // PIP Root process
    pipid = PIP_PIPID_ANY;
    printf( "<R> &pipid: %p\n", &pipid );
    pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
	       NULL, NULL, NULL );
    pip_wait( pipid, NULL );

  } else {			// PIP child task
#ifdef PRINT_MAPS
    pip_print_maps();
#endif
    printf( "<%d> &pipid: %p\n", pipid, &pipid );
    printf( "<%d> Hello, I am fine !!\n", pipid );
  }
  pip_fin();
  return 0;
}
