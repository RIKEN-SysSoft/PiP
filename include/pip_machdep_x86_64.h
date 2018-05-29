/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2016-2017
*/

#ifndef _pip_machdep_x86_64_h
#define _pip_machdep_x86_64_h

#ifndef DOXYGEN_SHOULD_SKIP_THIS

/**** Spin Lock ****/

#include <stdint.h>

typedef volatile uint32_t	pip_spinlock_t;
#define PIP_LOCK_TYPE

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

int arch_prctl( int, unsigned long* );

inline static void pip_print_fs_segreg( void ) {
  intptr_t fsreg;
  if( arch_prctl( ARCH_GET_FS, (unsigned long*) &fsreg ) == 0 ) {
    fprintf( stderr, "FS REGISTER: 0x%lx\n", (intptr_t) fsreg );
  } else {
    fprintf( stderr, "FS REGISTER: (unable to get:%d)\n", errno );
  }
}
#define PIP_PRINT_FSREG

#endif

#endif
