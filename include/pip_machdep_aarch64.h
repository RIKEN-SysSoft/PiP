/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2017
 */

#ifndef _pip_machdep_x86_64_h
#define _pip_machdep_x86_64_h

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <stdint.h>

typedef volatile uint32_t	pip_spinlock_t;
#define PIP_LOCK_TYPE

inline static void pip_pause( void ) {
  asm volatile("wfe" :::"memory");
}
#define PIP_PAUSE

inline static void pip_write_barrier(void) {
  asm volatile("dmb ishst" :::"memory");
}
#define PIP_WRITE_BARRIER

inline static void pip_memory_barrier(void) {
  asm volatile("dmb ish" :::"memory");
}
#define PIP_MEMORY_BARRIER

inline static void pip_print_fs_segreg( void ) {
  register unsigned long result asm ("x0");
  asm ("mrs %0, tpidr_el0; " : "=r" (result));
  fprintf( stderr, "TPIDR_EL0 REGISTER: 0x%lx\n", result );
}
#define PIP_PRINT_FSREG

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#endif
