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

//#define HUGETLB

#define SHMNAM		"shmem.dat"

static inline int create_shmem( void **vaddrp ) {
  void *vaddr;
  int fd, i, *data;
  uint64_t tm;

  tm = rdtscp();
  fd = shm_open( SHMNAM,
		 O_CREAT|O_TRUNC|O_RDWR,
		 S_IRUSR | S_IWUSR);
  tm = rdtscp() - tm;
  printf( "create: shm_open(): %lu\n", tm );
  tm = rdtscp();
  ftruncate( fd, MMAP_SIZE );
  tm = rdtscp() - tm;
  printf( "create: ftruncate(): %lu\n", tm );
  tm = rdtscp() - tm;
  vaddr = mmap( NULL,
		MMAP_SIZE,
		PROT_READ|PROT_WRITE,
#ifdef HUGETLB
		MAP_HUGETLB |
#endif
		MAP_SHARED,
		fd,
		0 );
  if( vaddr == MAP_FAILED ) {
    printf( "create_shmem(): map failed\n" );
    return -1;
  }
  madvise( vaddr, MMAP_SIZE, MADV_HUGEPAGE );
  tm = rdtscp() - tm;
  printf( "create: mmap(): %lu\n", tm );
  tm = rdtscp() - tm;
  close( fd );
  tm = rdtscp() - tm;
  printf( "create: close(): %lu\n", tm );
  *vaddrp = vaddr;
  data    = (int*) vaddr;

  for( i=0; i<MMAP_SIZE/sizeof(int); i++ ) {
    data[i] = i;
  }
  //printf( "create_shmem(): fd=%d\n", fd );
#ifdef HUGETLB
  system( "grep -i huge /proc/meminfo" );
#endif
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
    uint64_t tm;

    tm = rdtscp() - tm;
    fd = shm_open( argv[1], O_RDWR, 0 );
    tm = rdtscp() - tm;
    printf( "client: shm_open(): %lu\n", tm );
    //printf( "main(): fd=%d\n", fd );
    tm = rdtscp() - tm;
    vaddr = mmap( NULL,
		  MMAP_SIZE,
		  PROT_READ|PROT_WRITE,
#ifdef HUGETLB
		  MAP_HUGETLB |
#endif
		  MAP_SHARED,
		  fd,
		  0 );
    if( vaddr == MAP_FAILED ) {
      printf( "main()=%d: map failed\n", errno );
    }
    madvise( vaddr, MMAP_SIZE, MADV_HUGEPAGE );
    tm = rdtscp() - tm;
    printf( "client: mmap(): %lu\n", tm );
    tm = rdtscp() - tm;
    close( fd );
    tm = rdtscp() - tm;
    printf( "client: close(): %lu\n", tm );
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
