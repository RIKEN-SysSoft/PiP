
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>

#include <eval.h>

//#define PERF_PFBASE
//#define PERF_PF
#define BANDWIDTH
//#define HUGETLB
//#define OVERALL
//#define DETACH

#define NTASKS_MAX	(50)
#ifndef PERF_PF
#ifndef HUGETLB
#define MMAP_SIZE	((size_t)(1024*1024))
#else
#define MMAP_SIZE	((size_t)(2*1024*1024))
#endif
#else
#define MMAP_SIZE	((size_t)(1024*1024))
#endif

static inline int create_region( void **vaddrp ) {
  void *vaddr;
  int fd, i, *data;

  fd = -1;
  if( ( fd = open( "/dev/zero", O_RDWR ) ) < 0 ) {
    printf( "open() failed\n" );
    return errno ;
  }
  if( ( vaddr = mmap( NULL,
		      MMAP_SIZE,
		      PROT_READ|PROT_WRITE,
#ifdef HUGETLB
		      MAP_HUGETLB |
#endif
		      MAP_PRIVATE | MAP_ANON,
		      fd,
		      0 ) ) == MAP_FAILED ) {
    printf( "mmap() failed\n" );
    return errno;
  }
#ifndef OVERALL
  printf( "vaddr=%p\n", vaddr );
#endif

  *vaddrp = vaddr;
  data    = (int*) vaddr;
  close( fd );
  for( i=0; i<MMAP_SIZE/sizeof(int); i++ ) {
    data[i] = i;
  }
#ifdef HUGETLB
  system( "grep -i huge /proc/meminfo" );
#endif
  return 0;
}

#ifdef PERF_PFBASE
#ifndef PERF_PF
#define PERF_PF
#endif
#endif

#define CACHEBLK	(64)
#define PAGESZ		(4096)

#ifdef PERF_PF
#define STRIDE		PAGESZ
#else
#define STRIDE		CACHEBLK
#endif

#define NITERS		(MMAP_SIZE/STRIDE)

#ifndef BANDWIDTH
#ifndef OVERALL
uint64_t xptime[NITERS];
#endif
#endif

static inline int touch( void *region ) {
  int sum = 0;
#ifndef PERF_PFBASE
  int i;

#ifdef BANDWIDTH
  double dtm = gettime();
#endif

  for( i=0; i<NITERS; i++ ) {
#ifndef BANDWIDTH
#ifndef OVERALL
    uint64_t delta = rdtscp();
#endif
#endif
    sum += *((int*)region);
    region += STRIDE;
#ifndef BANDWIDTH
#ifndef OVERALL
    xptime[i] = rdtscp() - delta;
#endif
#endif
  }

#ifndef OVERALL
#ifdef BANDWIDTH
  dtm = gettime() - dtm;
  printf( "Time: %g\n", dtm );
#endif
#endif
#endif
  return sum;
}

static inline void print_time( void ) {
#ifndef BANDWIDTH
#ifndef PERF_PF
#ifndef OVERALL
  int i;
  for( i=0; i<NITERS; i++ ) {
    printf( "%lu\n", xptime[i] );
  }
#endif
#endif
#endif
}
