

#include <sys/time.h>
#include <stdint.h>
#include <stdio.h>

extern double foo( int );
extern double bar( double*, int );

#define ARRAYSZ		(128*1024*1024)
double a[ARRAYSZ];

#define RDTSC(X)\
asm volatile ("rdtsc; shlq $32,%%rdx; orq %%rdx,%%rax" : "=a" (X) :: "%rdx")

inline double rdtsc() {
  unsigned long long x;
  RDTSC(x);
  return (double)x;
}

inline double gettime( void ) {
  struct timeval tv;
  gettimeofday( &tv, NULL );
  return ((double)tv.tv_sec + (((double)tv.tv_usec) * 1.0e-6));
}

void foo_loop() {
  double start, delta;
  double ss;
  double x;
  int i, c, sz;

  for( sz=128; sz<ARRAYSZ; sz*=2 ) {
    x = 0.0;
    for( i=0; i<sz; i++ ) x += foo( i );
    x = 0.0;
    c = 0;
    delta = 0.0;
    ss = gettime();
    do {
      c++;
      start = rdtsc();
      for( i=0; i<sz; i++ ) x += foo( i );
      delta += rdtsc() - start;
    } while( ( gettime() - ss ) < 0.5 );
    delta /= (double) c;

    printf( "GOT\t %d\t %g\t %g\n", sz, delta, delta/(double)sz );
  }
}

void bar_loop( void ) {
  double start, end, delta;
  double ss;
  double x;
  int i, c, sz;

  for( sz=128; sz<ARRAYSZ; sz*=2 ) {
    x = 0.0;
    for( i=0; i<sz; i++ ) x += bar( a, i );
    x = 0.0;
    c = 0;
    delta = 0.0;
    ss = gettime();
    do {
      c++;
      start = rdtsc();
      for( i=0; i<sz; i++ ) x += bar( a, i );
      delta += rdtsc() - start;
    } while( ( gettime() - ss ) < 0.5 );
    delta /= (double) c;

    printf( "PTR\t %d\t %g\t %g\n", sz, delta, delta/(double)sz );
  }
}

int main() {

  foo_loop();
  bar_loop();

  foo_loop();
  bar_loop();

  return 0;
}
