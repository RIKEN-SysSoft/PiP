/*
  * $RIKEN_copyright: 2018 Riken Center for Computational Sceience,
  * 	  System Software Devlopment Team. All rights researved$
  * $PIP_VERSION: Version 1.0$
  * $PIP_license: <Simplified BSD License>
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted provided that the following conditions are
  * met:
  *
  * 1. Redistributions of source code must retain the above copyright
  *    notice, this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright
  *    notice, this list of conditions and the following disclaimer in the
  *    documentation and/or other materials provided with the distribution.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  * The views and conclusions contained in the software and documentation
  * are those of the authors and should not be interpreted as representing
  * official policies, either expressed or implied, of the PiP project.$
*/
/*
 * Written by Atsushi HORI <ahori@riken.jp>
 */

#ifndef _pip_machdep_aarch64_h
#define _pip_machdep_aarch64_h

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#define PIP_STACK_DESCENDING

typedef intptr_t		pip_tls_t;

inline static int pip_save_tls( pip_tls_t *tlsp ) {
  pip_tls_t tls;
  asm volatile ("mrs  %0, tpidr_el0" : "=r" (tls));
  *tlsp = tls;
  return 0;
}

inline static int pip_load_tls( pip_tls_t tls ) {
  asm volatile ("msr tpidr_el0, %0" : : "r" (tls));
  return 0;
}

inline static void pip_pause( void ) {
  asm volatile("sevl" :::"memory");
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
