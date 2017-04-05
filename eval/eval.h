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

#ifdef __x86_64__
static inline uint64_t rdtsc() {
  uint64_t x;
  __asm__ volatile ("rdtsc" : "=A" (x));
  return x;
}

#define RDTSCP(X)	\
  asm volatile ("rdtscp; shlq $32,%%rdx; orq %%rdx,%%rax;" : "=a" (X) :: "%rcx", "%rdx")

static inline uint64_t rdtscp() {
  uint64_t x;
  RDTSCP( x );
  return x;
}
#else
static inline uint64_t rdtscp() {
  return 0;
}
#endif
