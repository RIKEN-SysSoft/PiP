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
#if !defined( __KNC__ ) && !defined( __MIC__ )
  asm volatile("pause" ::: "memory");
#else  /* Xeon PHI (KNC) */
  /* the following value is tentative and must be tuned !! */
  asm volatile( "movl $4,%eax;"
		"delay %eax;" );
#endif
}
#define PIP_PAUSE

/* come from spin_lock() on glibc 2.3.3 (Fedora Core 1) */

inline static int pip_spin_trylock (pip_spinlock_t *lock) {
  int oldval;

  asm volatile
    ("xchgl %0,%1"
     : "=r" (oldval), "=m" (*lock)
     : "0" (0));
  return oldval > 0; /* Change return value to kernel function by Kameyama */
}
#define PIP_SPIN_TRYLOCK

inline static int pip_spin_unlock (pip_spinlock_t *lock) {
  asm volatile
    ("movl $1,%0"
     : "=m" (*lock));
  return 0;
}
#define PIP_SPIN_UNLOCK

inline static void pip_write_barrier(void) {
  asm volatile("sfence":::"memory");
}
#define PIP_WRITE_BARRIER

inline static void pip_memory_barrier(void) {
  asm volatile("mfence":::"memory");
}
#define PIP_MEMORY_BARRIER

#include <asm/prctl.h>
#include <sys/prctl.h>
#include <errno.h>

inline static void pip_print_fs_segreg( void ) {
  int arch_prctl(int, unsigned long*);
  unsigned long fsreg;
  if( arch_prctl( ARCH_GET_FS, &fsreg ) == 0 ) {
    fprintf( stderr, "FS REGISTER: 0x%lx\n", fsreg );
  } else {
    fprintf( stderr, "FS REGISTER: (unable to get:%d)\n", errno );
  }
}
#define PIP_PRINT_FSREG

#endif

#endif
