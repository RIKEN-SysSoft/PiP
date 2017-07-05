/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2016
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#define PRINT_FL(FSTR,V)	\
  fprintf(stderr,"%s:%d %s=%d\n",__FILE__,__LINE__,FSTR,V)

#define TESTINT(F)		\
  do{int __xyz=(F); if(__xyz){PRINT_FL(#F,__xyz);exit(9);}} while(0)

#define TESTSC(F)		\
  do{int __xyz=(F); if(__xyz){PRINT_FL(#F,errno);exit(9);}} while(0)

#include <sys/time.h>

static inline double gettime( void ) {
  struct timeval tv;
  gettimeofday( &tv, NULL );
  return ((double)tv.tv_sec + (((double)tv.tv_usec) * 1.0e-6));
}

/*
 * rdtsc (read time stamp counter)
 */

//#define NO_CLOCK_GETTIME
#if defined(NO_CLOCK_GETTIME)
static inline uint64_t rdtscp() {
  struct timeval tv;
  (void) gettimeofday( &tv, NULL );
  return ((uint64_t) tv.tv_sec) * 1000000000LLU
    + (uint64_t) tv.tv_usec * 1000LLU;
}
#elif defined(__GNUC__) && (defined(__sparcv9__) || defined(__sparc_v9__))
static inline uint64_t rdtscp( void ) {
  uint64_t rval;
  asm volatile("rd %%tick,%0" : "=r"(rval));
  return rval;
}
#else
#include <time.h>
static inline uint64_t rdtscp( void ) {
  struct timespec ts;
  (void) clock_gettime( CLOCK_REALTIME, &ts );
  return ((uint64_t) ts.tv_sec) * 1000000000LLU
    + (uint64_t) ts.tv_nsec;
}
#endif
