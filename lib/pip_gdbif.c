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
 * Written by Noriyuki Soda (soda@sra.co.jp) and
 * Atsushi HORI <ahori@riken.jp>, 2018, 2019
 */

#ifdef DEBUG
//#undef DEBUG
#endif

#include <pip_gdb.c>

#include <pip_gdbif_func.h>
#include <pip_dlfcn.h>

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

void pip_gdbif_load( pip_task_internal_t *task ) {
  struct pip_gdbif_task *gdbif_task = task->annex->gdbif_task;
  Dl_info dli;
  char buf[PATH_MAX];

  ENTER;
  if( gdbif_task != NULL ) {
    void *faddr = NULL;

    gdbif_task->handle = task->annex->loaded;

    if( task->annex->symbols.main != NULL ) {
      faddr = task->annex->symbols.main;
    } else if( task->annex->symbols.start != NULL ) {
      faddr = task->annex->symbols.start;
    }
    gdbif_task->load_address = NULL;
    if( faddr != NULL ) {
      if( !pip_dladdr( faddr, &dli ) ) {
	pip_warn_mesg( "dladdr(%s) failure"
		       " - PiP-gdb won't work with this PiP task %d",
		       task->annex->args.prog, task->pipid );
      } else {
	gdbif_task->load_address = dli.dli_fbase;
      }
    }
    /* dli.dli_fname is same with task->args.prog and may be a relative path */
    DBGF( "[%d] task:%p gdbif_task:%p", task->pipid, task, gdbif_task );
    if( realpath( task->annex->args.prog, buf ) == NULL ) {
      gdbif_task->realpathname = NULL; /* give up */
      pip_warn_mesg( "realpath(%s): %s"
		     " - PiP-gdb won't work with this PiP task %d",
		     task->annex->args.prog, strerror( errno ), task->pipid );
    } else {
      gdbif_task->realpathname = strdup( buf );
      DBGF( "gdbif_task->realpathname:%p", gdbif_task->realpathname );
      if( gdbif_task->realpathname == NULL ) { /* give up */
	pip_warn_mesg( "strdup(%s) failure"
		       " - PiP-gdb won't work with this PiP task %d",
		       task->annex->args.prog, task->pipid );
      }
    }
  }
  RETURNV;
}

void pip_gdbif_exit( pip_task_internal_t *task, int extval ) {
  struct pip_gdbif_task *gdbif_task = task->annex->gdbif_task;
  DBG;
  if( gdbif_task != NULL ) {
    gdbif_task->status    = PIP_GDBIF_STATUS_TERMINATED;
    gdbif_task->exit_code = extval;
  }
}

void pip_gdbif_task_commit( pip_task_internal_t *task ) {
  struct pip_gdbif_task *gdbif_task = task->annex->gdbif_task;
  DBG;
  gdbif_task->pid = task->annex->tid;
  pip_memory_barrier();
  gdbif_task->status = PIP_GDBIF_STATUS_CREATED;
}

/*
 * The following functions must be called at root process
 */

static void pip_gdbif_init_task_struct( struct pip_gdbif_task *gdbif_task,
					pip_task_internal_t *task ) {
  ENTER;
  /* members from task->args are unavailable if PIP_GDBIF_STATUS_TERMINATED */
  task->annex->gdbif_task = gdbif_task;
  gdbif_task->pathname     = task->annex->args.prog;
  gdbif_task->realpathname = NULL; /* filled by pip_gdbif_load() later */
  if ( task->annex->args.argv == NULL ) {
    gdbif_task->argc = 0;
  } else {
    gdbif_task->argc = pip_count_vec( task->annex->args.argv );
  }
  gdbif_task->argv         = task->annex->args.argv;
  gdbif_task->envv         = task->annex->args.envv;
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

void pip_gdbif_task_new( pip_task_internal_t *task ) {
  struct pip_gdbif_root *gdbif_root = pip_root->gdbif_root;
  struct pip_gdbif_task *gdbif_task = &gdbif_root->tasks[task->pipid];
  pip_gdbif_init_task_struct( gdbif_task, task );
  pip_gdbif_link_task_struct( gdbif_task );
  task->annex->gdbif_task = gdbif_task;
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

void pip_gdbif_finalize_task( pip_task_internal_t *task ) {
  struct pip_gdbif_root *gdbif_root = pip_root->gdbif_root;
  struct pip_gdbif_task *gdbif_task = task->annex->gdbif_task;

  ENTER;
  if( gdbif_task != NULL ) {
    gdbif_task->status   = PIP_GDBIF_STATUS_TERMINATED;
    pip_memory_barrier();
    gdbif_task->pathname = NULL;
    gdbif_task->argc     = 0;
    gdbif_task->argv     = NULL;
    gdbif_task->envv     = NULL;
    if( gdbif_task->realpathname  != NULL ) {
      char *p = gdbif_task->realpathname;
      DBGF( "p:%p", p );
      gdbif_task->realpathname = NULL; /* do this before free() for PIP-gdb */
      pip_memory_barrier();
      free( p );
    }
    pip_spin_lock( &gdbif_root->lock_free );
    {
      PIP_SLIST_INSERT_HEAD(&gdbif_root->task_free, gdbif_task, free_list);
      pip_gdbif_finalize_tasks();
    }
    pip_spin_unlock( &gdbif_root->lock_free );
  }
  RETURNV;
}

void pip_gdbif_hook_before( pip_task_internal_t *taski ) {
  struct pip_gdbif_root *gdbif_root = pip_root->gdbif_root;
  struct pip_gdbif_task *gdbif_task = taski->annex->gdbif_task;

  if( gdbif_root->hook_before_main != NULL ) {
    gdbif_root->hook_before_main( gdbif_task );
  }
}

void pip_gdbif_hook_after( pip_task_internal_t *taski ) {
  struct pip_gdbif_root *gdbif_root = pip_root->gdbif_root;
  struct pip_gdbif_task *gdbif_task = taski->annex->gdbif_task;

  if( gdbif_root->hook_after_main != NULL ) {
    gdbif_root->hook_after_main( gdbif_task );
  }
}