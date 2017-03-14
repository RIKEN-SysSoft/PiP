
#include <stdint.h>
#include <stdio.h>

#define RDTSCP(X)	\
  asm volatile ("rdtscp; shlq $32,%%rdx; orq %%rdx,%%rax;" : "=a" (X) :: "%rcx", "%rdx")

static inline uint64_t rdtscp() {
  uint64_t x;
  RDTSCP( x );
  return x;
}

#define ITER	(1000*1000*1000)

int main() {
  uint64_t tm;
  double tmd;
  int i;

  for( i=0; i<1000; i++ ) {
    rdtscp();
  }
  tm = rdtscp();
  for( i=0; i<ITER; i++ ) {
    rdtscp();
  }
  tm = rdtscp() - tm;

  tmd = tm;
  tmd /= (double) ITER;
  printf( "%g\n", tmd );
  return 0;
}
