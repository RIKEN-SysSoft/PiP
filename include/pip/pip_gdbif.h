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

#ifndef _pip_gdbif_h_
#define _pip_gdbif_h_

#include <pip/pip.h>
#include <pip/pip_machdep.h> /* for pip_spinlock_t */
#include <pip/pip_gdbif_queue.h>
#include <pip/pip_gdbif_enums.h>

struct pip_gdbif_task {
  /* double linked list */
  /* assuming GDB only dereferencing the next pointer */
  PIP_HCIRCLEQ_ENTRY(pip_gdbif_task) task_list;
  struct pip_gdbif_task *root;
  PIP_SLIST_ENTRY(pip_gdbif_task) free_list;
  /* pathname of the program */
  char	*pathname;
  char	*realpathname;
  /* argc, argv and env */
  int	argc;
  char 	**argv;
  char 	**envv;
  /* handle of the loaded link map */
  void	*handle;
  void	*load_address;
  /* PID or TID of the PiP task, the value of zero means nothing */
  pid_t	pid;
  int	pipid;
  /* exit code, this value is set when the PiP task */
  /* gets PIP_GDBIF_STATUS_TERMINATED */
  volatile int	exit_code;
  /* pip task exec mode */
  enum pip_task_exec_mode	exec_mode;
  /* task status, this is set by PiP lib */
  enum pip_task_status		status;
  /* the variable(s) below are set/reset by GDB */
  enum pip_gdb_status		gdb_status;
};

typedef void(*pip_gdbif_hook_t)(struct pip_gdbif_task*);

struct pip_gdbif_root {
  /* hook function address, these addresses are set */
  /* when the PiP task gets PIP_GDBIF_STATUS_LOADED */
  pip_gdbif_hook_t	hook_before_main;
  pip_gdbif_hook_t	hook_after_main;

  pip_spinlock_t	lock_free; /* lock for task_free */
  PIP_SLIST_HEAD(, pip_gdbif_task) task_free;

  pip_spinlock_t	lock_root; /* lock for task_root */
  PIP_HCIRCLEQ_HEAD(pip_gdbif_task) task_root;
  /* task_root == tasks[PIP_GDBIF_PIPID_ROOT], although it's not recommended */

  struct pip_gdbif_task	tasks[];
};

#endif /* _pip_gdbif_h_ */
