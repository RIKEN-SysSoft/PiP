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

#ifndef _pip_machdep_h_
#define _pip_machdep_h_

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#if defined(__x86_64__)
#include <pip_machdep_x86_64.h>
#elif defined(__aarch64__)
#include <pip_machdep_aarch64.h>
#endif

#ifndef PIP_CACHEBLK_SZ
#define PIP_CACHEBLK_SZ		(64)
#endif

#ifndef PIP_LOCK_TYPE
typedef volatile uint32_t	pip_spinlock_t;
#endif

#ifndef PIP_ATOMIC_TYPE
typedef volatile intptr_t	pip_atomic_t;

#ifndef PIP_PAUSE
inline static void pip_pause( void ) {
  /* nop */
}
#endif

#ifndef PIP_WRITE_BARRIER
inline static void pip_write_barrier( void )
  __attribute__((always_inline)); /* this function must be inlined ALWAYS!! */
inline static void pip_write_barrier( void ) {
  __sync_synchronize ();
}
#endif

#ifndef PIP_MEMORY_BARRIER
inline static void pip_memory_barrier( void )
  __attribute__((always_inline)); /* this function must be inlined ALWAYS!! */
inline static void pip_memory_barrier( void ) {
  __sync_synchronize ();
}
#endif

#ifndef PIP_SPIN_TRYLOCK_WV
inline static int
pip_spin_trylock_wv( pip_spinlock_t *lock, pip_spinlock_t lv ) {
  return __sync_val_compare_and_swap( lock, 0, lv );
}
#endif

#ifndef PIP_SPIN_LOCK_WV
inline static void
pip_spin_lock_wv( pip_spinlock_t *lock, pip_spinlock_t lv ) {
  while( pip_spin_trylock_wv( lock, lv ) != 0 ) pip_pause();
}
#endif

#ifndef PIP_SPIN_TRYLOCK
inline static int pip_spin_trylock( pip_spinlock_t *lock ) {
  if( *lock != 0 ) return 0;
  int oldval = __sync_val_compare_and_swap( lock, 0, 1 );
  return oldval == 0;
}
#endif

#ifndef PIP_SPIN_LOCK
inline static void pip_spin_lock( pip_spinlock_t *lock ) {
  while( !pip_spin_trylock( lock ) ) pip_pause();
}
#endif

#ifndef PIP_SPIN_UNLOCK
inline static void pip_spin_unlock( pip_spinlock_t *lock ) {
  __sync_lock_release( lock );	/* *lock <= 0 */
}
#endif

#ifndef PIP_SPIN_INIT
inline static void pip_spin_init( pip_spinlock_t *lock ) {
  pip_spin_unlock( lock );
}
#endif

#ifndef PIP_SPIN_DESTROY
inline static void pip_spin_destroy( pip_spinlock_t *lock ) {
  /* Nothing to do.  */
}
#endif

#ifndef PIP_COMP_AND_SWAP
inline static int
pip_comp_and_swap( pip_atomic_t *lock, pip_atomic_t oldv, pip_atomic_t newv ) {
  return (int) __sync_bool_compare_and_swap( lock, oldv, newv );
}
#endif

#ifndef PIP_COMP2_AND_SWAP
inline static int
pip_comp2_and_swap( pip_atomic_t *lock, pip_atomic_t oldv, pip_atomic_t newv ){
  return ( ( *lock ) != oldv ) ? 0 :
     __sync_bool_compare_and_swap( lock, oldv, newv );
}
#endif

#ifndef PIP_ATOMIC_FETCH_AND_ADD
inline static pip_atomic_t
pip_atomic_fetch_and_add( pip_atomic_t *p,  pip_atomic_t v ) {
  return __sync_fetch_and_add( p, v );
}
#endif

#ifndef PIP_ATOMIC_SUB_AND_FETCH
inline static pip_atomic_t
pip_atomic_sub_and_fetch( pip_atomic_t *p, pip_atomic_t v ) {
  return __sync_sub_and_fetch( p, v );
}
#endif
#endif

#ifndef PIP_PRINT_FSREG
inline static void pip_print_fs_segreg( void ) {}
#endif

#ifndef PIP_CTX_T
#include <ucontext.h>
typedef struct {
  ucontext_t		ctx;
} pip_ctx_t;
#endif

#ifndef PIP_MAKE_CONTEXT
#define pip_make_context(CTX,F,C,...)	 \
  do { makecontext(&(CTX)->ctx,(void(*)(void))(F),(C),__VA_ARGS__); } while(0)
#endif

/* I cannot call getcontext() in a function but       */
/* gcc (4.8.5 20150623) complains it is attributed */
/* as always_inline. So I make it as a mcro            */
#ifndef PIP_SAVE_CONTEXT
#define pip_save_context(ctxp)	 ( getcontext(&(ctxp)->ctx) ? errno : 0 )
#endif

#ifndef PIP_SWAP_CONTEXT
inline static int pip_swap_context( pip_ctx_t *oldctx, pip_ctx_t *newctx )
  __attribute__((always_inline));
inline static int pip_swap_context( pip_ctx_t *oldctx, pip_ctx_t *newctx ) {
    return ( swapcontext( &oldctx->ctx, &newctx->ctx ) ? errno : 0 );
}
#endif

#ifndef PIP_LOAD_CONTEXT
inline static int pip_load_context( const pip_ctx_t *ctxp ) {
    return ( setcontext(&ctxp->ctx) ? errno : 0 );
}
#endif

#endif	/* DOXYGEN_SHOULD_SKIP_THIS */

#endif	/* _pip_machdep_h_ */
