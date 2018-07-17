/*
 * $RIKEN_copyright: 2018 Riken Center for Computational Sceience, 
 * 	  System Software Devlopment Team. All rights researved$
 * $PIP_VERSION: Version 1.0$
 * $PIP_license: Redistribution and use in source and binary forms,
 * with or without modification, are permitted provided that the following
 * conditions are met:
 * 
 *     Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 
 *     Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * official policies, either expressed or implied, of the FreeBSD Project.$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2017
 */

#include <xpmem_eval.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

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
    printf( "create_shmem(): map failed (%d)\n", errno );
    return -1;
  }
#ifdef HUGETLB
#ifdef MADV_HUGEPAGE
  madvise( vaddr, MMAP_SIZE, MADV_HUGEPAGE );
#endif
#endif
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
  int fd;
  long long sum;
  static char *nargv[3];

  if( argc < 2 ) {
    fd = create_shmem( &vaddr );
    if( ( pid = fork() ) == 0 ) {
      corebind( 1 );
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
#ifdef MADV_HUGEPAGE
    madvise( vaddr, MMAP_SIZE, MADV_HUGEPAGE );
#endif
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
