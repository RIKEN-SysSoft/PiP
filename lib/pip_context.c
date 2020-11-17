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
 * System Software Development Team, 2016-2020
 * $
 * $PIP_VERSION: Version 3.0.0$
 *
 * $Author: Atsushi Hori (R-CCS) mailto: ahori@riken.jp or ahori@me.com
 * $
 */

#include <pip_internal.h>

/* STACK PROTECT */

/* context switch functions */

#ifdef PIP_USE_FCONTEXT

typedef struct pip_ctx_data {
  pip_task_internal_t	*old;
  pip_task_internal_t	*new;
} pip_ctx_data_t;

INLINE void pip_preproc_fctxsw( pip_ctx_data_t *dp,
				pip_task_internal_t *old,
				pip_task_internal_t *new,
				pip_ctx_p *ctxp ) {
  TA(old)->ctx_savep = ctxp;
  dp->old = old;
  dp->new = new;
  if( new != old ) {
    /* GCC: warning: suggest explicit braces
       to avoid ambiguous ‘else’ [-Wparentheses] */
    ASSERTS( pip_load_tls( TA(new)->tls ) );
  }
}

INLINE void pip_postproc_fctxsw( pip_transfer_t tr ) {
  pip_ctx_data_t	*dp = (pip_ctx_data_t*) tr.data;

  DBGF( "PIPID:%d <<== PIPID:%d", TA(dp->new)->pipid, TA(dp->old)->pipid );
  ASSERTD( TA(dp->old)->ctx_savep == NULL );
  *TA(dp->old)->ctx_savep = tr.ctx;
  /* before unprotect stack, old ctx must be saved */
}

#endif	/* PIP_USE_FCONTEXT */

INLINE void pip_deffered_proc( pip_task_internal_t *taski ) {
  pip_deffered_proc_t	def_proc = TA(taski)->deffered_proc;
  void			*def_arg = TA(taski)->deffered_arg;

  TA(taski)->deffered_proc = NULL;
  TA(taski)->deffered_arg  = NULL;
  if( def_proc != NULL ) def_proc( def_arg );
  RETURNV;
}

INLINE void pip_post_ctxsw( pip_task_internal_t *taski ) {
  pip_deffered_proc( taski );
}

void pip_swap_context( pip_task_internal_t *taski,
		       pip_task_internal_t *nexti ) {
#ifdef PIP_USE_FCONTEXT
  pip_ctx_data_t	data;
  pip_ctx_p		ctx;
  pip_transfer_t 	tr;

  ENTERF( "PIPID:%d ==>> PIPID:%d", TA(taski)->pipid, TA(nexti)->pipid );

  /* wait for stack and ctx ready */
  ctx = TA(nexti)->ctx_suspend;
  ASSERTD( ctx == NULL );
  TA(nexti)->ctx_suspend = NULL;
  pip_preproc_fctxsw( &data, taski, nexti, &TA(taski)->ctx_suspend );
  tr = pip_jump_fctx( ctx, (void*) &data );
  pip_postproc_fctxsw( tr );

#else  /* ucontext */
  struct {
    pip_ctx_p ctxp_new;
    pip_ctx_t ctx_old;		/* must be the last member */
  } lvars;

  DBGF( "PIPID:%d ==>> PIPID:%d", TA(taski)->pipid, TA(nexti)->pipid );
  ASSERTD( TA(nexti)->ctx_suspend == NULL );

  lvars.ctxp_new = TA(nexti)->ctx_suspend;
  TA(nexti)->ctx_suspend = NULL;
  if( taski != nexti ) {
    ASSERTS( pip_load_tls( TA(nexti)->tls ) );
  }
  TA(taski)->ctx_suspend = &lvars.ctx_old;
  ASSERTS( pip_swap_ctx( &lvars.ctx_old, lvars.ctxp_new ) );
  SETCURR( taski->task_sched, taski );

#endif
  pip_post_ctxsw( taski );
}

void pip_call_sleep2( pip_task_internal_t *schedi ) {
  pip_deffered_proc( schedi );
  pip_sleep( schedi );
}

#ifdef PIP_USE_FCONTEXT
static void pip_call_sleep( pip_transfer_t tr ) {
  pip_ctx_data_t *dp = (pip_ctx_data_t*) tr.data;
  ASSERTD( TA(dp->old)->ctx_savep == NULL );
  *TA(dp->old)->ctx_savep = tr.ctx;
  pip_call_sleep2( dp->new );
}
#else  /* ucontext */
static void pip_call_sleep( intptr_t task_H, intptr_t task_L ) {
  pip_task_internal_t	*schedi = (pip_task_internal_t*)
    ( ( ((intptr_t) task_H) << 32 ) | ( ((intptr_t) task_L) & PIP_MASK32 ) );
  pip_call_sleep2( schedi );
}
#endif

void pip_decouple_context( pip_task_internal_t *taski,
			   pip_task_internal_t *schedi ) {
  ENTERF( "task PIPID:%d  sched PIPID:%d", TA(taski)->pipid, TA(schedi)->pipid );
  AA(taski)->flag_wakeup = 0;
  pip_memory_barrier();

#ifdef PIP_USE_FCONTEXT
  pip_ctx_data_t	data;
  pip_ctx_p		ctx = AA(schedi)->ctx_trampoline;
  pip_transfer_t 	tr;

  IF_UNLIKELY( ctx == NULL ) {
    /* creating a new context to sleep for the first time */
    ctx = pip_make_fctx( AA(schedi)->stack_trampoline
#ifdef PIP_STACK_DESCENDING
			 + pip_root->stack_size_trampoline
#endif
			 , 0, 	/* NOT USED !!!! */
			 /*  pip_root->stack_size_trampoline,  */
			 pip_call_sleep );
    AA(schedi)->ctx_trampoline = ctx;
  }
  pip_preproc_fctxsw( &data, taski, schedi, &TA(taski)->ctx_suspend );
  tr = pip_jump_fctx( ctx, (void*) &data );
  pip_postproc_fctxsw( tr );

#else  /* ucontext */
  struct {
    int 	args_H, args_L;
    stack_t	*stk;
    pip_ctx_t	ctx_old, ctx_new; /* must be the last member */
  } lvars;

  IF_UNLIKELY( AA(schedi)->ctx_trampoline == NULL ) {
    /* creating a new context to sleep for the first time */
    lvars.stk = &lvars.ctx_new.ctx.uc_stack;
    lvars.stk->ss_sp    = AA(schedi)->stack_trampoline;
    lvars.stk->ss_size  = pip_root->stack_size_trampoline;
    lvars.stk->ss_flags = 0;
    lvars.args_H = ( ((intptr_t) schedi) >> 32 ) & PIP_MASK32;
    lvars.args_L = (  (intptr_t) schedi)         & PIP_MASK32;

    ASSERTS( pip_save_ctx( &lvars.ctx_new ) );
    pip_make_uctx( &lvars.ctx_new,
		   pip_call_sleep,
		   2,
		   lvars.args_H,
		   lvars.args_L );
    AA(schedi)->ctx_trampoline = &lvars.ctx_new;
  }
  TA(taski)->ctx_suspend = &lvars.ctx_old;
  if( taski != schedi ) {
    ASSERTS( pip_load_tls( TA(schedi)->tls ) );
  }
  pip_swap_ctx( TA(taski)->ctx_suspend, AA(schedi)->ctx_trampoline );
#endif
  pip_post_ctxsw( taski );
}

void pip_couple_context( pip_task_internal_t *schedi,
			 pip_task_internal_t *taski ) {
  /* this must be called from the trampoline context */
  DBGF( "task PIPID:%d   sched PIPID:%d",
	TA(taski)->pipid, TA(schedi)->pipid );

#ifdef PIP_USE_FCONTEXT
  pip_ctx_data_t	data;
  pip_ctx_p		ctx;
  pip_transfer_t 	tr;

  ctx = TA(taski)->ctx_suspend;
  ASSERTD( ctx == NULL );
  TA(taski)->ctx_suspend = NULL;
  pip_preproc_fctxsw( &data, schedi, taski, &AA(schedi)->ctx_trampoline );
  tr = pip_jump_fctx( ctx, (void*) &data );
  pip_postproc_fctxsw( tr );

#else  /* ucontext */
  struct {
    pip_ctx_t	ctx; /* must be the last member */
  } lvars;

  ASSERTD( TA(taski)->ctx_suspend == NULL );
  AA(schedi)->ctx_trampoline = &lvars.ctx;
  if( taski != schedi ) {
    ASSERTS( pip_load_tls( TA(taski)->tls ) );
  }
  pip_swap_ctx( &lvars.ctx, TA(taski)->ctx_suspend );
#endif
  pip_post_ctxsw( schedi );
}
