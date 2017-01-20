

#include <sys/time.h>
#include <stdint.h>
#include <stdio.h>

extern double foo( int );
extern double bar( double*, int );

#define ARRAYSZ		(1024)
double a[ARRAYSZ];

#define RDTSC(X)\
asm volatile ("rdtsc; shlq $32,%%rdx; orq %%rdx,%%rax" : "=a" (X) :: "%rdx")

inline double gettime( void ) {
  struct timeval tv;
  gettimeofday( &tv, NULL );
  return ((double)tv.tv_sec + (((double)tv.tv_usec) * 1.0e-6));
}

#define AH
inline double rdtsc() {
#ifdef AH
  unsigned long long x;
  RDTSC(x);
  return (double)x;
#else
  return gettime();
#endif
}

void foo_loop() {
  double start, delta;
  double ss;
  double x;
  int i, c, sz;
  int ii;

  x = 0.0;
  //  for( sz=128; sz<=ARRAYSZ; sz*=2 ) {
  {
    sz = 1024;
    x = 0.0;
    for( i=0; i<sz; i++ ) x += foo( i );
    c = 0;
    delta = 0.0;
    ss = gettime();

    start = rdtsc();
    for( ii=0; ii<1000; ii++ ) {
      for( i=0; i<sz; i++ ) x += foo( i );
    }
    delta += rdtsc() - start;
    //delta /= (double) c;

    printf( "GOT\t %d\t %g\t %g\t (%g)\n", sz, delta, delta/(double)sz, x );
  }
}

void bar_loop( void ) {
  double start, end, delta;
  double ss;
  double x;
  int i, c, sz;

  //  for( sz=128; sz<=ARRAYSZ; sz*=2 ) {
  {
    sz = 1024;
    x = 0.0;
    for( i=0; i<sz; i++ ) x += bar( a, i );
    c = 0;
    delta = 0.0;
    ss = gettime();
    do {
      x = 0.0;
      c++;
      start = rdtsc();
      for( i=0; i<sz; i++ ) x += bar( a, i );
      delta += rdtsc() - start;
    } while( ( gettime() - ss ) < 2.0 );
    delta /= (double) c;

    printf( "PTR\t %d\t %g\t %g\t (%g)\n", sz, delta, delta/(double)sz, x );
  }
}

int main() {
  int i;

  for( i=0; i<ARRAYSZ; i++ ) a[i] = (double) 0;

  foo_loop();
  //  foo_loop();
  //  bar_loop();
  //  bar_loop();

  return 0;
}
