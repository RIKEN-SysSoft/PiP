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

static void pip_do_swap_context( pip_task_internal_t *taski, 
				 pip_task_internal_t *nexti,
				 int flag_tls ) {
  struct {
    pip_ctx_t *ctxp_new;
    pip_ctx_t ctx_old;		/* must be the last member */
  } lvars;

  DBGF( "PIPID:%d ==>> PIPID:%d", taski->pipid, nexti->pipid );

#ifdef PIP_USE_FCONTEXT
#else
  lvars.ctxp_new = nexti->ctx_suspend;
  ASSERTD( lvars.ctxp_new == NULL );
  nexti->ctx_suspend = NULL;
  pip_stack_wait( nexti );

  DBG;
  taski->ctx_suspend = &lvars.ctx_old;
  if( flag_tls ) {
    ASSERT( pip_load_tls( nexti->tls ) );
  }
  ASSERT( pip_swap_ctx( &lvars.ctx_old, lvars.ctxp_new ) );
  DBG;
  pip_stack_unprotect( taski );
#endif
}

void pip_swap_context( pip_task_internal_t *taski,
		       pip_task_internal_t *nexti ) {
  pip_do_swap_context( taski, nexti, 0 );
}

void pip_swap_tls_and_context( pip_task_internal_t *taski,
			       pip_task_internal_t *nexti ) {
  pip_do_swap_context( taski, nexti, 1 );
}

static void pip_do_jump_context( pip_task_internal_t *taski, 
				 pip_task_internal_t *nexti,
				 int flag_tls ) {
  struct {
    pip_ctx_t *ctxp_new;
  } lvars;

  DBGF( "PIPID:%d ==>> PIPID:%d", taski->pipid, nexti->pipid );

#ifdef PIP_USE_FCONTEXT
#else
  lvars.ctxp_new = nexti->ctx_suspend;
  ASSERTD( lvars.ctxp_new == NULL );
  nexti->ctx_suspend = NULL;
  pip_stack_wait( nexti );

  if( flag_tls ) {
    ASSERT( pip_load_tls( nexti->tls ) );
  }
  pip_load_ctx( lvars.ctxp_new );
#endif
  NEVER_REACH_HERE;
}

void pip_jump_context( pip_task_internal_t *taski,
		       pip_task_internal_t *nexti ) {
  pip_do_jump_context( taski, nexti, 0 );
}

void pip_jump_tls_and_context( pip_task_internal_t *taski,
			       pip_task_internal_t *nexti ) {
  pip_do_jump_context( taski, nexti, 1 );
}

void pip_switch_to_sleep_context( pip_task_internal_t *taski,
				  pip_task_internal_t *schedi ) {
  DBGF( "PIPID:%d ==>> PIPID:%d", taski->pipid, schedi->pipid );

#ifdef PIP_USE_FCONTEXT
#else
  struct {
    int 	args_H, args_L;
    stack_t	*stk;
    pip_ctx_t	ctx_old, ctx_new; /* must be the last member */
  } lvars;
  
  /* creating a new context to sleep */
  lvars.stk = &lvars.ctx_new.ctx.uc_stack;
  lvars.stk->ss_sp    = schedi->annex->sleep_stack;
  lvars.stk->ss_size  = pip_root->stack_size_sleep;
  lvars.stk->ss_flags = 0;
  lvars.args_H = ( ((intptr_t) schedi) >> 32 ) & PIP_MASK32;
  lvars.args_L = (  (intptr_t) schedi)         & PIP_MASK32;

  ASSERT( pip_save_ctx( &lvars.ctx_new ) );
  pip_make_ctx( &lvars.ctx_new, 
		pip_sleep, 
		2, 
		lvars.args_H, 
		lvars.args_L );
  taski->ctx_suspend = &lvars.ctx_old;
  pip_swap_ctx( &lvars.ctx_old, &lvars.ctx_new );
  /* resumed */
  pip_stack_unprotect( taski );
#endif
}

void pip_jump_to_sleep_context( pip_task_internal_t *taski,
				pip_task_internal_t *schedi ) {
  DBGF( "PIPID:%d ==>> PIPID:%d", taski->pipid, schedi->pipid );

#ifdef PIP_USE_FCONTEXT
#else
  pip_switch_to_sleep_context( taski, schedi );
  pip_task_finalize( taski );
#endif
  NEVER_REACH_HERE;
}

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

void pip_stack_unprotect( pip_task_internal_t *taski ) {
  /* this function must be called everytime a context is switched */
  if( taski->flag_stackpp != NULL ) {
#ifdef DEBUG
    pip_task_internal_t *prev = (pip_task_internal_t*)
      ( taski->flag_stackpp - offsetof(pip_task_internal_t,flag_stackp) );
    DBGF( "PIPID:%d UN-protect pipid:%d", taski->pipid, prev->pipid );
    if( *taski->flag_stackpp == 0 ) DBGF( "ALREADY UN-protected" );
#endif
    *taski->flag_stackpp = 0;
    taski->flag_stackpp  = NULL;
    pip_memory_barrier();
  } else {
    DBGF( "UNABLE to UN-protect PIPID:%d", taski->pipid );
  }
}

void pip_stack_wait( pip_task_internal_t *taski ) {
  int i = 0;

  //DBGF( "PIPID:%d wait for unprotect", taski->pipid );
  if( taski->flag_stackp ) {
    for( i=0; i<pip_root->yield_iters; i++ ) {
      pip_pause();
      if( !taski->flag_stackp ) goto done;
    }
    for( i=0; taski->flag_stackp; i++ ) {
      pip_system_yield();
      DBGF( "WAITING  pipid:%d (count=%d*%d) ...",
	    taski->pipid, i, PIP_BUSYWAIT_COUNT );
      if( i > 1000 ) {
	DBGF( "WAIT-FAILED  pipid:%d (count=%d*%d)",
	      taski->pipid, i, PIP_BUSYWAIT_COUNT );
	ASSERT( 1 );
	break;
      }
    }
  }
 done:
  DBGF( "WAIT-DONE  pipid:%d (count=%d*%d)",
	taski->pipid, i, PIP_BUSYWAIT_COUNT );
}
