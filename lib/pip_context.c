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

#include <pip_internal.h>
#include <pip_blt.h>
#include <pip_context.h>

/* STACK PROTECT */

void pip_stack_protect( pip_task_internal_t *taski,
			pip_task_internal_t *nexti ) {
  /* so that new task can reset the flag */
  DBGF( "protect PIPID:%d released by PIPID:%d",
	taski->pipid, nexti->pipid );
  ASSERTD( nexti->flag_stackpp != NULL );
  taski->flag_stackp  = 1;
  nexti->flag_stackpp = &taski->flag_stackp;
  pip_memory_barrier();
}

static void pip_stack_unprotect( pip_task_internal_t *taski ) {
  /* this function must be called everytime a context is switched */
  if( taski->flag_stackpp == NULL ) {
    DBGF( "UNABLE to UN-protect PIPID:%d", taski->pipid );
  } else {
#ifdef DEBUG
    pip_task_internal_t *prev = (pip_task_internal_t*)
      ( taski->flag_stackpp - offsetof(pip_task_internal_t,flag_stackp) );
    DBGF( "PIPID:%d UN-protect pipid:%d", taski->pipid, prev->pipid );
    if( *taski->flag_stackpp == 0 ) DBGF( "ALREADY UN-protected" );
#endif
    *taski->flag_stackpp = 0;
    taski->flag_stackpp  = NULL;
    pip_memory_barrier();
  }
}

void pip_stack_wait( pip_task_internal_t *taski ) {
  int i = 0, j;

  DBGF( "PIPID:%d wait for unprotect", taski->pipid );
  if( taski->flag_stackp ) {
    for( i=0; i<pip_root->yield_iters; i++ ) {
      pip_pause();
      if( !taski->flag_stackp ) goto done;
    }
    for( i=0; taski->flag_stackp; i++ ) {
      for( j=0; j<PIP_BUSYWAIT_COUNT; j++ ) {
	pip_pause();
	if( !taski->flag_stackp ) goto done;
      }
      sleep( 1 );
      pip_system_yield();
      DBGF( "WAITING  pipid:%d (count=%d*%d) ...",
	    taski->pipid, i, PIP_BUSYWAIT_COUNT );
      if( i > 10000 ) {
	DBGF( "WAIT-FAILED  pipid:%d (count=%d*%d)",
	      taski->pipid, i, PIP_BUSYWAIT_COUNT );
	ASSERT( "TIMED OUT !!!!" != NULL );
	break;
      }
    }
  }
 done:
  DBGF( "WAIT-DONE  pipid:%d (count=%d*%d)",
	taski->pipid, i, PIP_BUSYWAIT_COUNT );
}

/* context switch functions */

void pip_swap_context( pip_task_internal_t *taski, 
		       pip_task_internal_t *nexti ) {
  DBGF( "PIPID:%d ==>> PIPID:%d", taski->pipid, nexti->pipid );
  ASSERT( pip_load_tls( nexti->tls ) );

#ifdef PIP_USE_FCONTEXT
#else
  struct {
    pip_ctx_t *ctxp_new;
    pip_ctx_t ctx_old;		/* must be the last member */
  } lvars;

  lvars.ctxp_new = nexti->ctx_suspend;
  ASSERTD( lvars.ctxp_new == NULL );
  nexti->ctx_suspend = NULL;
  pip_stack_wait( nexti );
  taski->ctx_suspend = &lvars.ctx_old;
  ASSERT( pip_swap_ctx( &lvars.ctx_old, lvars.ctxp_new ) );
#endif
  pip_stack_unprotect( taski );
  ASSERTD( (pid_t) taski->task_sched->annex->tid != pip_gettid() );
}

#ifdef PIP_USE_FCONTEXT
static void pip_call_sleep( pip_transfer_t trans ) {
  pip_task_internal_t	*taski = (pip_task_intyernal_t*) trans.data;
  pip_stack_unprotect( taski );
  pip_sleep( taski );
}
#else
static void pip_call_sleep( intptr_t task_H, intptr_t task_L ) {
  pip_task_internal_t	*taski = (pip_task_internal_t*)
    ( ( ((intptr_t) task_H) << 32 ) | ( ((intptr_t) task_L) & PIP_MASK32 ) );
  pip_stack_unprotect( taski );
  pip_sleep( taski );
}
#endif

void pip_decouple_context( pip_task_internal_t *taski,
			   pip_task_internal_t *schedi ) {
  DBGF( "task PIPID:%d   sched PIPID:%d", taski->pipid, schedi->pipid );

#ifdef PIP_USE_FCONTEXT
#else
  struct {
    int 	args_H, args_L;
    stack_t	*stk;
    pip_ctx_t	ctx_old, ctx_new; /* must be the last member */
  } lvars;

  if( schedi->annex->ctx_sleep == NULL ) {
    /* creating a new context to sleep */
    lvars.stk = &lvars.ctx_new.ctx.uc_stack;
    lvars.stk->ss_sp    = schedi->annex->stack_sleep;
    lvars.stk->ss_size  = pip_root->stack_size_sleep;
    lvars.stk->ss_flags = 0;
    lvars.args_H = ( ((intptr_t) schedi) >> 32 ) & PIP_MASK32;
    lvars.args_L = (  (intptr_t) schedi)         & PIP_MASK32;
    
    ASSERT( pip_save_ctx( &lvars.ctx_new ) );
    pip_make_uctx( &lvars.ctx_new, 
		   pip_call_sleep, 
		   2, 
		   lvars.args_H, 
		   lvars.args_L );
    schedi->annex->ctx_sleep = &lvars.ctx_new;
  }
  taski->ctx_suspend = &lvars.ctx_old;
  if( taski != schedi ) {
    ASSERT( pip_load_tls( taski->tls ) );
  }
  pip_swap_ctx( taski->ctx_suspend, schedi->annex->ctx_sleep );
#endif
  pip_stack_unprotect( taski );
  /* resumed */
}

void pip_couple_context( pip_task_internal_t *taski,
			 pip_task_internal_t *schedi ) {
  DBGF( "task PIPID:%d   sched PIPID:%d", 
	taski->pipid, schedi->pipid );

#ifdef PIP_USE_FCONTEXT
#else
  struct {
    pip_ctx_t	ctx; /* must be the last member */
  } lvars;

  ASSERTD( taski->ctx_suspend == NULL );
  schedi->annex->ctx_sleep = &lvars.ctx;
  pip_stack_wait( taski );
  if( taski != schedi ) {
    ASSERT( pip_load_tls( taski->tls ) );
  }
  pip_swap_ctx( &lvars.ctx, taski->ctx_suspend );
#endif
  pip_stack_unprotect( schedi );
  /* resumed */
}
