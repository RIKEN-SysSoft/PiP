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

#include <stddef.h>
#include <sys/wait.h>
#include <pip/pip_gdbif.h>

/* struct pip_gdbif_task */

const int PIP_GDBIF_TASK_SIZE		= sizeof(struct pip_gdbif_task);
const int PIP_GDBIF_TASK_OFFSET_NEXT	= (offsetof(struct pip_gdbif_task,task_list));
const int PIP_GDBIF_TASK_OFFSET_PREV	= (offsetof(struct pip_gdbif_task,task_list)+sizeof(void*));
const int PIP_GDBIF_TASK_ROOT		= (offsetof(struct pip_gdbif_task,root));
const int PIP_GDBIF_TASK_PATHNAME	= (offsetof(struct pip_gdbif_task,pathname));
const int PIP_GDBIF_TASK_REALPATHNAME	= (offsetof(struct pip_gdbif_task,realpathname));
const int PIP_GDBIF_TASK_ARGC		= (offsetof(struct pip_gdbif_task,argc));
const int PIP_GDBIF_TASK_ARGV		= (offsetof(struct pip_gdbif_task,argv));
const int PIP_GDBIF_TASK_ENVV		= (offsetof(struct pip_gdbif_task,envv));
const int PIP_GDBIF_TASK_HANDLE		= (offsetof(struct pip_gdbif_task,handle));
const int PIP_GDBIF_TASK_LOAD_ADDRESS	= (offsetof(struct pip_gdbif_task,load_address));
const int PIP_GDBIF_TASK_PID		= (offsetof(struct pip_gdbif_task,pid));
const int PIP_GDBIF_TASK_PIPID		= (offsetof(struct pip_gdbif_task,pipid));
const int PIP_GDBIF_TASK_EXIT_CODE	= (offsetof(struct pip_gdbif_task,exit_code));
const int PIP_GDBIF_TASK_EXEC_MODE	= (offsetof(struct pip_gdbif_task,exec_mode));
const int PIP_GDBIF_TASK_STATUS		= (offsetof(struct pip_gdbif_task,status));
const int PIP_GDBIF_TASK_GDB_STATUS	= (offsetof(struct pip_gdbif_task,gdb_status));

/* struct pip_gdbif_root */

const int PIP_GDBIF_ROOT_SIZE			= sizeof(struct pip_gdbif_root);
const int PIP_GDBIF_ROOT_OFFSET_TASK_ROOT	= offsetof(struct pip_gdbif_root,task_root);
const int PIP_GDBIF_ROOT_OFFSET_BEFOER_MAIN	= offsetof(struct pip_gdbif_root,hook_before_main);
const int PIP_GDBIF_ROOT_OFFSET_AFTER_MAIN	= offsetof(struct pip_gdbif_root,hook_after_main);
