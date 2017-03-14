/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <xpmem_eval.h>

#define SHMNAM		"shmem.dat"

static inline int create_shmem( void **vaddrp ) {
  void *vaddr;
  int fd, i, *data;

  fd = shm_open( SHMNAM,
		 O_CREAT|O_TRUNC|O_RDWR,
		 S_IRUSR | S_IWUSR);
  ftruncate( fd, MMAP_SIZE );
  vaddr = mmap( NULL,
		MMAP_SIZE,
		PROT_READ|PROT_WRITE,
		MAP_SHARED,
		fd,
		0 );
  close( fd );
  if( vaddr == MAP_FAILED ) {
    printf( "create_shmem(): map failed\n" );
    return -1;
  }
  *vaddrp = vaddr;
  data    = (int*) vaddr;

  for( i=0; i<MMAP_SIZE/sizeof(int); i++ ) {
    data[i] = i;
  }
  printf( "create_shmem(): fd=%d\n", fd );
  return fd;
}

int main( int argc, char **argv ) {
  extern char **environ;
  void *vaddr;
  pid_t pid;
  uint64_t tm;
  int fd, sum;
  static char *nargv[3];

  if( argc < 2 ) {
    fd = create_shmem( &vaddr );
    if( ( pid = fork() ) == 0 ) {
      nargv[0] = argv[0];
      nargv[1] = SHMNAM;
      nargv[3] = NULL;
      execve( nargv[0], nargv, environ );
      printf( "execve()=%d\n", errno );
    } else {
      wait( NULL );
      shm_unlink( SHMNAM );
    }
  } else {
    fd = shm_open( argv[1], O_RDWR, 0 );
    printf( "main(): fd=%d\n", fd );
    vaddr = mmap( NULL,
		  MMAP_SIZE,
		  PROT_READ|PROT_WRITE,
		  MAP_SHARED,
		  fd,
		  0 );
    close( fd );
    if( vaddr == MAP_FAILED ) {
      printf( "main()=%d: map failed\n", errno );
    }
    sum = touch( vaddr );
    print_time();
    printf( "sum=%d\n", sum );
#ifndef PERF_PF
    sum = touch( vaddr );
    print_time();
#endif
  }
  return 0;
}
