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

#ifndef _pip_context_h_
#define _pip_context_h_

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <pip_machdep.h>

#ifdef PIP_USE_FCONTEXT

#include <config.h>

#ifdef PIP_BOOST_COMPILE
#if PIP_USE_CONTEXT > 1
#define BOOST_USE_TSX
#endif

#else  /* !PIP_BOOST_COMPILE */

typedef void* 		pip_fcontext;
typedef pip_transfer {
  pip_fcontext		ctx;
  void			*data;
} pip_transfer_t;

pip_fcontext_t make_fcontext( void *sp, size_t size, void (*fn)(pip_transfer_t*) );

#define pip_make_fctx(SP,SZ,FUNC)	make_fcontext((SP),(SZ),(FUNC))

#endif	/* PIP_BOOST_COMPILE */

#else  /* !PIP_USE_FCONTEXT */

#include <ucontext.h>
typedef struct {
  ucontext_t		ctx;
} pip_ctx_t;

#define pip_make_uctx(CTX,F,C,...)	 \
  do { makecontext(&(CTX)->ctx,(void(*)(void))(F),(C),__VA_ARGS__); } while(0)

/* I cannot call getcontext() inside of a function but */
/* gcc (4.8.5 20150623) complains if it is attributed  */
/* as always_inline. So I make it as a mcro            */
#define pip_save_ctx(ctxp)	 ( getcontext(&(ctxp)->ctx) ? errno : 0 )

inline static int pip_load_ctx( const pip_ctx_t *ctxp ) {
  return ( setcontext(&ctxp->ctx) ? errno : 0 );
}

inline static int pip_swap_ctx( pip_ctx_t *oldctx, pip_ctx_t *newctx )
  __attribute__((always_inline));
inline static int pip_swap_ctx( pip_ctx_t *oldctx, pip_ctx_t *newctx ) {
  if( oldctx == NULL ) {
    return ( setcontext(&newctx->ctx) ? errno : 0 );
  } else {
    return ( swapcontext( &oldctx->ctx, &newctx->ctx ) ? errno : 0 );
  }
}

#endif	/* PIP_USE_FCONTEXT */

#endif	/* DOXYGEN_SHOULD_SKIP_THIS */

#endif /* _pip_context_h_ */
