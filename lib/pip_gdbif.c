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

#define _GNU_SOURCE

#include <dlfcn.h>
#include <elf.h>

#include <pip_internal.h>
#include <pip_machdep.h>
#include <pip_util.h>

#include <pip_gdbif.h>

struct pip_gdbif_root	*pip_gdbif_root = NULL;

extern int  pip_count_vec_( char** );
extern void pip_page_alloc_( size_t, void** );

static int pipid_to_gdbif( int pipid ) {
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

void pip_gdbif_load_( pip_task_internal_t *task ) {
  struct pip_gdbif_task *gdbif_task = task->annex->gdbif_task;
  Dl_info dli;
  char buf[PATH_MAX];
  ENTER;

  if( gdbif_task != NULL ) {
    gdbif_task->handle = task->annex->loaded;

    if( !dladdr( task->annex->symbols.main, &dli ) ) {
      pip_warn_mesg( "dladdr(%s) failure"
		     " - PIP-gdb won't work with this PiP task %d",
		     task->annex->args.prog, task->pipid );
      gdbif_task->load_address = NULL;
    } else {
      gdbif_task->load_address = dli.dli_fbase;
    }

    /* dli.dli_fname is same with task->args.prog and may be a relative path */
    DBGF( "[%d] task:%p gdbif_task:%p", task->pipid, task, gdbif_task );
    if( realpath( task->annex->args.prog, buf ) == NULL ) {
      gdbif_task->realpathname = NULL; /* give up */
      pip_warn_mesg( "realpath(%s): %s"
		     " - PIP-gdb won't work with this PiP task %d",
		     task->annex->args.prog, strerror( errno ), task->pipid );
    } else {
      gdbif_task->realpathname = strdup( buf );
      DBGF( "gdbif_task->realpathname:%p", gdbif_task->realpathname );
      if( gdbif_task->realpathname == NULL ) { /* give up */
	pip_warn_mesg( "strdup(%s) failure"
		       " - PIP-gdb won't work with this PiP task %d",
		       task->annex->args.prog, task->pipid );
      }
    }
  }
  RETURNV;
}

void pip_gdbif_exit_( pip_task_internal_t *task, int extval ) {
  if( task->annex->gdbif_task != NULL ) {
    task->annex->gdbif_task->status    = PIP_GDBIF_STATUS_TERMINATED;
    task->annex->gdbif_task->exit_code = extval;
  }
}

void pip_gdbif_task_commit_( pip_task_internal_t *task ) {
  struct pip_gdbif_task *gdbif_task =
    &pip_gdbif_root->tasks[task->pipid];

  gdbif_task->pid = task->annex->pid;
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
  gdbif_task->pathname     = task->annex->args.prog;
  gdbif_task->realpathname = NULL; /* filled by pip_load_gdbif() later */
  if ( task->annex->args.argv == NULL ) {
    gdbif_task->argc = 0;
  } else {
    gdbif_task->argc = pip_count_vec_( task->annex->args.argv );
  }
  gdbif_task->argv   = task->annex->args.argv;
  gdbif_task->envv   = task->annex->args.envv;
  gdbif_task->handle = task->annex->loaded; /* filled by pip_load_gdbif later*/
  gdbif_task->load_address = NULL; /* filled by pip_load_gdbif() later */
  gdbif_task->exit_code    = -1;
  gdbif_task->pid          = task->annex->pid;
  gdbif_task->pipid        = pipid_to_gdbif( task->pipid );
  gdbif_task->exec_mode =
    (pip_root_->opts & PIP_MODE_PROCESS) ? PIP_GDBIF_EXMODE_PROCESS :
    (pip_root_->opts & PIP_MODE_PTHREAD) ? PIP_GDBIF_EXMODE_THREAD :
    PIP_GDBIF_EXMODE_NULL;
  gdbif_task->status     = PIP_GDBIF_STATUS_NULL;
  gdbif_task->gdb_status = PIP_GDBIF_GDB_DETACHED;
  RETURNV;
}

static void pip_gdbif_init_root_task_link( struct pip_gdbif_task *gdbif_task ){
  PIP_HCIRCLEQ_INIT(*gdbif_task, task_list);
}

static void pip_gdbif_link_task_struct( struct pip_gdbif_task *gdbif_task ) {
  gdbif_task->root = &pip_gdbif_root->task_root;
  pip_spin_lock( &pip_gdbif_root->lock_root );
  PIP_HCIRCLEQ_INSERT_TAIL(pip_gdbif_root->task_root, gdbif_task, task_list);
  pip_spin_unlock( &pip_gdbif_root->lock_root );
}

void pip_gdbif_task_new_( pip_task_internal_t *task ) {
  struct pip_gdbif_task *gdbif_task =
    &pip_gdbif_root->tasks[task->pipid];
  pip_gdbif_init_task_struct( gdbif_task, task );
  pip_gdbif_link_task_struct( gdbif_task );
  task->annex->gdbif_task = gdbif_task;
}

void pip_gdbif_initialize_root_( int ntasks ) {
  struct pip_gdbif_root *gdbif_root;
  size_t sz;

  ENTER;
  sz = sizeof( *gdbif_root ) + sizeof( gdbif_root->tasks[0] ) * ntasks;
  pip_page_alloc_( sz, (void**) &gdbif_root );
  gdbif_root->hook_before_main = NULL; /* XXX */
  gdbif_root->hook_after_main  = NULL; /* XXX */
  pip_spin_init( &gdbif_root->lock_free );
  PIP_SLIST_INIT(&gdbif_root->task_free);
  pip_spin_init( &gdbif_root->lock_root );
  pip_gdbif_init_task_struct( &gdbif_root->task_root, pip_root_->task_root );
  pip_gdbif_init_root_task_link( &gdbif_root->task_root );
  pip_memory_barrier();
  gdbif_root->task_root.status = PIP_GDBIF_STATUS_CREATED;
  pip_gdbif_root = gdbif_root; /* assign after initialization completed */
  RETURNV;
}

static void pip_gdbif_finalize_tasks( void ) {
  struct pip_gdbif_task *gdbif_task, **prev, *next;

  ENTER;
  if( pip_gdbif_root == NULL ) {
    DBGF( "pip_gdbif_root=NULL, pip_init() hasn't called?" );
    return;
  }
  pip_spin_lock( &pip_gdbif_root->lock_root );
  prev = &PIP_SLIST_FIRST(&pip_gdbif_root->task_free);
  PIP_SLIST_FOREACH_SAFE(gdbif_task, &pip_gdbif_root->task_free, free_list,
			 next) {
    if( gdbif_task->gdb_status != PIP_GDBIF_GDB_DETACHED ) {
      prev = &PIP_SLIST_NEXT(gdbif_task, free_list);
    } else {
      *prev = next;
      PIP_HCIRCLEQ_REMOVE(gdbif_task, task_list);
    }
  }
  pip_spin_unlock( &pip_gdbif_root->lock_root );
  RETURNV;
}

void pip_gdbif_finalize_task_( pip_task_internal_t *task ) {
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
    pip_spin_lock( &pip_gdbif_root->lock_free );
    {
      PIP_SLIST_INSERT_HEAD(&pip_gdbif_root->task_free, gdbif_task, free_list);
      pip_gdbif_finalize_tasks();
    }
    pip_spin_unlock( &pip_gdbif_root->lock_free );
  }
  RETURNV;
}
