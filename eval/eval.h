
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>

#if defined(__x86_64__)
#define RDTSC(X)\
  asm volatile ("rdtsc; shlq $32,%%rdx; orq %%rdx,%%rax" : "=a" (X) :: "%rdx")

static inline uint64_t get_cycle_counter( void ) {
  uint64_t x;
  RDTSC(x);
  return x;
}
#elif defined(__aarch64__)
static inline uint64_t get_cycle_counter( void ) {
  int64_t virtual_timer_value;
  asm volatile("mrs %0, cntvct_el0" : "=r"(virtual_timer_value));
  return virtual_timer_value;
}
#elif defined(__GNUC__) && (defined(__sparcv9__) || defined(__sparc_v9__))
static inline uint64_t get_cycle_counter( void ) {
  uint64_t rval;
  asm volatile("rd %%tick,%0" : "=r"(rval));
  return rval;
}
#else
#include <time.h>
static inline uint64_t get_cycle_counter( void ) {
  struct timespec ts;
  (void) clock_gettime( CLOCK_REALTIME, &ts );
  return ((uint64_t) ts.tv_sec) * 1000000000LLU
    + (uint64_t) ts.tv_nsec;
}
#endif
