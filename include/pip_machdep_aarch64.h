/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2016
*/

#ifndef _pip_machdep_x86_64_h
#define _pip_machdep_x86_64_h

#ifndef DOXYGEN_SHOULD_SKIP_THIS

/**** Spin Lock ****/

#include <stdint.h>

typedef volatile uint32_t	pip_spinlock_t;

inline static void pip_pause( void ) {
  asm volatile("wfe" :::"memory");
}

/* come from spin_lock() on glibc 2.3.3 (fedora core 1) */

inline static int pip_spin_trylock (pip_spinlock_t *lock) {
  int oldval;

  asm volatile
    ("  ldaxr %w0, [%1]\n"
     "  cbnz  %w0, 1f\n"
     "  stxr  %w0, %w2, [%1]\n"
     "1:\n"
     : "=&r" (oldval)
     : "r" (*lock), "r" (1)
     : "memory");
  return oldval == 0;
}

inline static int pip_spin_lock (pip_spinlock_t *lock) {
  while( !pip_spin_trylock( lock ) ) pip_pause();
  return 0;
}

inline static int pip_spin_unlock (pip_spinlock_t *lock) {
  asm volatile
    ("  stlr  %w1, [%0]\n"
     : : "r" (*lock), "r" (0) : "memory");
  return 0;
}

inline static int pip_spin_init (pip_spinlock_t *lock) {
  pip_spin_unlock( lock );
  return 0;
}

inline static int pip_spin_destroy (pip_spinlock_t *lock) {
  /* nothing to do.  */
  return 0;
}

inline static void pip_write_barrier(void) {
  asm volatile("dsb st" :::"memory");
}

inline static void pip_memory_barrier(void) {
  asm volatile("dsb sy" :::"memory");
}

#endif

#endif
