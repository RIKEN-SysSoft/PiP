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
 * $PIP_VERSION: Version 2.0.0$
 *
 * $Author: Atsushi Hori (R-CCS)
 * Query:   procinproc-info+noreply@googlegroups.com
 * User ML: procinproc-users+noreply@googlegroups.com
 * $
 */

#include <pip/pip_internal.h>
#include <pip/pip_gdbif.h>

static int pipid_to_gdbif( int pipid ) {
  DBGF( "PIPID:%d", pipid );
  switch( pipid ) {
  case PIP_PIPID_ROOT:
    return( PIP_GDBIF_PIPID_ROOT );
  case PIP_PIPID_ANY:
    return( PIP_GDBIF_PIPID_ANY );
  default:
    return( pipid );
  }
}

/*
 * NOTE: pip_gdbif_load() won't be called for PiP root tasks.
 * thus, load_address and realpathname are both NULL for them.
 * handle is available, though.
 *
 * because this function is only for PIP-gdb,
 * this does not return any error, but warn.
 */

void pip_gdbif_load( pip_task_t *task ) {
  struct pip_gdbif_task *gdbif_task = task->gdbif_task;
  Dl_info dli;

  ENTER;
  if( gdbif_task != NULL ) {
    void *faddr = NULL;

    gdbif_task->handle = task->loaded;

    if( task->symbols.main != NULL ) {
      faddr = task->symbols.main;
    } else if( task->symbols.start != NULL ) {
      faddr = task->symbols.start;
    }
    gdbif_task->load_address = NULL;
    if( faddr != NULL ) {
      if( !dladdr( faddr, &dli ) ) {
	pip_warn_mesg( "dladdr(%s) failure"
		       " - PiP-gdb won't work with this PiP task %d",
		       task->args.prog, task->pipid );
      } else {
	gdbif_task->load_address = dli.dli_fbase;
      }
    }
    /* dli.dli_fname is same with task->args.prog and may be a relative path */
    DBGF( "[%d] task:%p gdbif_task:%p", task->pipid, task, gdbif_task );
    gdbif_task->realpathname = task->args.prog_full;
  }
  RETURNV;
}

void pip_gdbif_exit( pip_task_t *task, int extval ) {
  struct pip_gdbif_task *gdbif_task = task->gdbif_task;

  if( gdbif_task != NULL ) {
    gdbif_task->pid       = -1;
    gdbif_task->status    = PIP_GDBIF_STATUS_TERMINATED;
    gdbif_task->exit_code = extval;
    pip_memory_barrier();
  }
}

void pip_gdbif_task_commit( pip_task_t *task ) {
  struct pip_gdbif_task *gdbif_task = task->gdbif_task;

  gdbif_task->pid = task->tid;
  pip_memory_barrier();
  gdbif_task->status = PIP_GDBIF_STATUS_CREATED;
}

/*
 * The following functions must be called at root process
 */

static void pip_gdbif_init_task_struct( struct pip_gdbif_task *gdbif_task,
					pip_task_t *task ) {
  ENTER;
  /* members from task->args are unavailable if PIP_GDBIF_STATUS_TERMINATED */
  task->gdbif_task = gdbif_task;
  gdbif_task->pathname     = task->args.prog;
  gdbif_task->realpathname = NULL; /* filled by pip_gdbif_load() later */
  gdbif_task->argc         = task->args.argc;
  gdbif_task->argv         = task->args.argvec.vec;
  gdbif_task->envv         = task->args.envvec.vec;
  gdbif_task->handle       = NULL; /* filled by pip_gdbif_load() later*/
  gdbif_task->load_address = NULL; /* filled by pip_gdbif_load() later */
  gdbif_task->exit_code    = -1;
  gdbif_task->pipid        = pipid_to_gdbif( task->pipid );
  gdbif_task->exec_mode =
    (pip_root->opts & PIP_MODE_PROCESS) ? PIP_GDBIF_EXMODE_PROCESS :
    (pip_root->opts & PIP_MODE_PTHREAD) ? PIP_GDBIF_EXMODE_THREAD :
    PIP_GDBIF_EXMODE_NULL;
  gdbif_task->status     = PIP_GDBIF_STATUS_NULL;
  gdbif_task->gdb_status = PIP_GDBIF_GDB_DETACHED;
  RETURNV;
}

static void pip_gdbif_link_task_struct( struct pip_gdbif_task *gdbif_task ) {
  struct pip_gdbif_root *gdbif_root = pip_root->gdbif_root;
  gdbif_task->root = &gdbif_root->task_root;
  pip_spin_lock( &gdbif_root->lock_root );
  PIP_HCIRCLEQ_INSERT_TAIL( gdbif_root->task_root, gdbif_task, task_list );
  pip_spin_unlock( &gdbif_root->lock_root );
}

void pip_gdbif_task_new( pip_task_t *task ) {
  struct pip_gdbif_root *gdbif_root = pip_root->gdbif_root;
  struct pip_gdbif_task *gdbif_task = &gdbif_root->tasks[task->pipid];
  pip_gdbif_init_task_struct( gdbif_task, task );
  pip_gdbif_link_task_struct( gdbif_task );
  task->gdbif_task = gdbif_task;
}

static void pip_gdb_hook_before( struct pip_gdbif_task *gdbif_task ) {
  return;
}

static void pip_gdb_hook_after( struct pip_gdbif_task *gdbif_task ) {
  return;
}

static void pip_gdbif_init_root_task_link( struct pip_gdbif_task *gdbif_task ){
  PIP_HCIRCLEQ_INIT( *gdbif_task, task_list );
}

void pip_gdbif_initialize_root( int ntasks ) {
  struct pip_gdbif_root *gdbif_root;
  size_t sz;

  ENTER;
  sz = sizeof( *gdbif_root ) + sizeof( gdbif_root->tasks[0] ) * ( ntasks );
  pip_page_alloc( sz, (void**) &gdbif_root );
  gdbif_root->hook_before_main = pip_gdb_hook_before;
  gdbif_root->hook_after_main  = pip_gdb_hook_after;
  PIP_SLIST_INIT( &gdbif_root->task_free );
  pip_spin_init(  &gdbif_root->lock_free );
  pip_spin_init(  &gdbif_root->lock_root );
  pip_root->gdbif_root = gdbif_root;
  pip_gdbif_init_task_struct( &gdbif_root->task_root, pip_root->task_root );
  pip_gdbif_init_root_task_link( &gdbif_root->task_root );
  gdbif_root->task_root.status = PIP_GDBIF_STATUS_CREATED;
  pip_memory_barrier();
  /* assign after initialization is completed */
  pip_gdbif_root = gdbif_root;
  RETURNV;
}

static void pip_gdbif_finalize_tasks( void ) {
  struct pip_gdbif_root *gdbif_root;
  struct pip_gdbif_task *gdbif_task, **prev, *next;

  ENTER;
  if( pip_root == NULL ) {
    DBGF( " pip_init() hasn't called?" );
    return;
  }
  gdbif_root = pip_root->gdbif_root;
  pip_spin_lock( &gdbif_root->lock_root );
  prev = &PIP_SLIST_FIRST(&gdbif_root->task_free);
  PIP_SLIST_FOREACH_SAFE(gdbif_task, &gdbif_root->task_free, free_list,
			 next) {
    if( gdbif_task->gdb_status != PIP_GDBIF_GDB_DETACHED ) {
      prev = &PIP_SLIST_NEXT(gdbif_task, free_list);
    } else {
      *prev = next;
      PIP_HCIRCLEQ_REMOVE(gdbif_task, task_list);
    }
  }
  pip_spin_unlock( &gdbif_root->lock_root );
  RETURNV;
}

void pip_gdbif_finalize_task( pip_task_t *task ) {
  struct pip_gdbif_root *gdbif_root = pip_root->gdbif_root;
  struct pip_gdbif_task *gdbif_task = task->gdbif_task;

  ENTER;
  if( gdbif_task != NULL ) {
    gdbif_task->status   = PIP_GDBIF_STATUS_TERMINATED;
    pip_memory_barrier();
    gdbif_task->pathname     = NULL;
    gdbif_task->argc         = 0;
    gdbif_task->argv         = NULL;
    gdbif_task->envv         = NULL;
    gdbif_task->realpathname = NULL;

    pip_spin_lock( &gdbif_root->lock_free );
    {
      PIP_SLIST_INSERT_HEAD(&gdbif_root->task_free, gdbif_task, free_list);
      pip_gdbif_finalize_tasks();
    }
    pip_spin_unlock( &gdbif_root->lock_free );
  }
  RETURNV;
}

void pip_gdbif_hook_before( pip_task_t *taski ) {
  struct pip_gdbif_root *gdbif_root = pip_root->gdbif_root;
  struct pip_gdbif_task *gdbif_task = taski->gdbif_task;

  if( gdbif_root->hook_before_main != NULL ) {
    gdbif_root->hook_before_main( gdbif_task );
  }
}

void pip_gdbif_hook_after( pip_task_t *taski ) {
  struct pip_gdbif_root *gdbif_root = pip_root->gdbif_root;
  struct pip_gdbif_task *gdbif_task = taski->gdbif_task;

  if( gdbif_root->hook_after_main != NULL ) {
    gdbif_root->hook_after_main( gdbif_task );
  }
}
