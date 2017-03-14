
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>

#include <eval.h>

//#define PERF_PFBASE
//#define PERF_PF
#define BANDWIDTH

#define NTASKS_MAX	(50)
#ifndef PERF_PF
#define MMAP_SIZE	((size_t)(1024*1024))
#else
#define MMAP_SIZE	((size_t)(128*1024*1024))
#endif

static inline int create_region( void **vaddrp ) {
  void *vaddr;
  int fd, i, *data;

  if( ( fd = open( "/dev/zero", O_RDWR ) ) < 0 ) return errno ;
  if( ( vaddr = mmap( NULL, MMAP_SIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE,
		      fd, 0 ) ) == MAP_FAILED ) return errno;
  *vaddrp = vaddr;
  data    = (int*) vaddr;

  close( fd );
  for( i=0; i<MMAP_SIZE/sizeof(int); i++ ) {
    data[i] = i;
  }
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
uint64_t xptime[NITERS];
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
    uint64_t delta = rdtscp();
#endif
    sum += *((int*)region);
    region += STRIDE;
#ifndef BANDWIDTH
    xptime[i] = rdtscp() - delta;
#endif
  }

#ifdef BANDWIDTH
  dtm = gettime() - dtm;
  printf( "Time: %g\n", dtm );
#endif
#endif
  return sum;
}

static inline void print_time( void ) {
#ifndef BANDWIDTH
#ifndef PERF_PF
  int i;
  for( i=0; i<NITERS; i++ ) {
    printf( "%lu\n", xptime[i] );
  }
#endif
#endif
}
