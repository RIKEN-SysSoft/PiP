#include <sys/mman.h>
#include <sys/wait.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include <pip.h>

#define NTASKS		(10)
#define PGSZ		(4096)

struct maps {
  pthread_barrier_t	barrier;
  void			*maps[NTASKS+1];
} maps;

int main( int argc, char **argv ) {
  void *export = (void*) &maps;
  void *map;
  int  i, ntasks, pipid;
  long ncpu = sysconf( _SC_NPROCESSORS_ONLN );

  ntasks = NTASKS;
  if ( ntasks > ncpu )
    ntasks = ncpu;

  pthread_barrier_init( &maps.barrier, NULL, ntasks + 1 );
  map = mmap( NULL,
	      PGSZ * ( ntasks + 1 ),
	      PROT_READ|PROT_WRITE,
	      MAP_PRIVATE|MAP_ANONYMOUS,
	      0,
	      0 );
  if( map == MAP_FAILED ) {
    fprintf( stderr, "mmap()=%d\n", errno );
  }
  maps.maps[ntasks] = map;

  pip_init( &pipid, &ntasks, (void*) &export, 0 );
  if( pipid == PIP_PIPID_ROOT ) {
    for( i=0; i<ntasks; i++ ) {
      pipid = i;
      pip_spawn( argv[0], argv, NULL, i, &pipid, NULL, NULL, NULL );
    }
    pthread_barrier_wait( &maps.barrier );
    for( i=0; i<ntasks+1; i++ ) {
      *((int*)(maps.maps[i]+(PGSZ*ntasks))) = 0;
    }
    pthread_barrier_wait( &maps.barrier );
    for( i=0; i<ntasks; i++ ) pip_wait( i, NULL );
    printf( "all done\n" );

  } else {	/* PIP child task */
    struct maps* import = (struct maps*) export;

    import->maps[pipid] = map;
    pthread_barrier_wait( &import->barrier );
    for( i=0; i<ntasks+1; i++ ) {
      *((int*)(import->maps[i]+(PGSZ*pipid))) = 0;
    }
    pthread_barrier_wait( &import->barrier );
    printf( "<%d> done\n", pipid );
  }
  pip_fin();
  munmap( map, PGSZ * (ntasks + 1 ) );
  return 0;
}
