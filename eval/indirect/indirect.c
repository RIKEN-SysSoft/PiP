
double foo( int idx ) {
  extern double a[];
  return a[idx];
}

double bar( double a[], int idx ) {
  return a[idx];
}


#ifdef AH

#define NOP10	\
  asm volatile("pause" ::: "memory"); \
  asm volatile("pause" ::: "memory"); \
  asm volatile("pause" ::: "memory"); \
  asm volatile("pause" ::: "memory"); \
  asm volatile("pause" ::: "memory"); \
  asm volatile("pause" ::: "memory"); \
  asm volatile("pause" ::: "memory"); \
  asm volatile("pause" ::: "memory"); \
  asm volatile("pause" ::: "memory"); \
  asm volatile("pause" ::: "memory")

#define NOP100 \
  NOP10; NOP10; NOP10; NOP10; NOP10; NOP10; NOP10; NOP10; NOP10; NOP10;

#define NOP1000 \
  NOP100; NOP100; NOP100; NOP100; NOP100; NOP100; NOP100; NOP100; NOP100; NOP100;

#define NOP10000 \
  NOP1000; NOP1000; NOP1000; NOP1000; NOP1000; NOP1000; NOP1000; NOP1000; NOP1000; NOP1000;

void nop100000_0( void ) {
  NOP10000; NOP10000; NOP10000; NOP10000; NOP10000; NOP10000; NOP10000; NOP10000; NOP10000; NOP10000;
}

void nop100000_1( void ) {
  NOP10000; NOP10000; NOP10000; NOP10000; NOP10000; NOP10000; NOP10000; NOP10000; NOP10000; NOP10000;
}

#endif
