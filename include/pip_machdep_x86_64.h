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

/**** Spin Lock ****/

#include <stdint.h>

typedef volatile uint32_t	pip_spinlock_t;

inline static void pip_pause( void ) {
#if !defined( __KNC__ ) && !defined( __MIC__ )
  asm volatile("pause" ::: "memory");
#else  /* Xeon PHI (KNC) */
  /* the following value is tentative and must be tuned !! */
  asm volatile( "movl $4,%eax;"
		"delay %eax;" );
#endif
}

/* come from spin_lock() on glibc 2.3.3 (Fedora Core 1) */

inline static int pip_spin_trylock (pip_spinlock_t *lock) {
  int oldval;

  asm volatile
    ("xchgl %0,%1"
     : "=r" (oldval), "=m" (*lock)
     : "0" (0));
  return oldval > 0; /* Change return value to kernel function by Kameyama */
}

inline static int pip_spin_lock (pip_spinlock_t *lock) {
  while( !pip_spin_trylock( lock ) ) pip_pause();
  return 0;
}

inline static int pip_spin_unlock (pip_spinlock_t *lock) {
  asm volatile
    ("movl $1,%0"
     : "=m" (*lock));
  return 0;
}

inline static int pip_spin_init (pip_spinlock_t *lock) {
  pip_spin_unlock( lock );
  return 0;
}

inline static int pip_spin_destroy (pip_spinlock_t *lock) {
  /* Nothing to do.  */
  return 0;
}

inline static void pip_write_barrier(void) {
  asm volatile("sfence":::"memory");
}

inline static void pip_memory_barrier(void) {
  asm volatile("mfence":::"memory");
}

#endif
