/*
 * $PIP_license: <Simplified BSD License>
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *     Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 * 
 *     Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 * $
 * $RIKEN_copyright: Riken Center for Computational Sceience (R-CCS),
 * System Software Development Team, 2016-2021
 * $
 * $PIP_VERSION: Version 3.0.0$
 *
 * $Author: Atsushi Hori (R-CCS)
 * Query:   procinproc-info+noreply@googlegroups.com
 * User ML: procinproc-users+noreply@googlegroups.com
 * $
 */

#ifndef _pip_machdep_aarch64_h
#define _pip_machdep_aarch64_h
#ifdef DOXYGEN_INPROGRESS
#ifndef INLINE
#define INLINE
#endif
#else
#ifndef INLINE
#define INLINE			inline static
#endif
#endif

#ifndef DOXYGEN_INPROGRESS

#include <stdint.h>

#define PIP_STACK_DESCENDING

typedef intptr_t		pip_tls_t;

INLINE int pip_save_tls( pip_tls_t *tlsp ) {
  pip_tls_t tls;
  asm volatile ("mrs  %0, tpidr_el0" : "=r" (tls));
  *tlsp = tls;
  return 0;
}

INLINE int pip_load_tls( pip_tls_t tls ) {
  asm volatile ("msr tpidr_el0, %0" : : "r" (tls));
  return 0;
}

INLINE void pip_pause( void ) {
  asm volatile("sevl" :::"memory");
  asm volatile("wfe" :::"memory");
}
#define PIP_PAUSE

INLINE void pip_write_barrier(void) {
  asm volatile("dmb ishst" :::"memory");
}
#define PIP_WRITE_BARRIER

INLINE void pip_memory_barrier(void) {
  asm volatile("dmb ish" :::"memory");
}
#define PIP_MEMORY_BARRIER

#endif /* DOXYGEN */

#endif
