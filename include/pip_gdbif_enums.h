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
 * $PIP_VERSION: Version 2.0.0$
 *
 * $Author: Atsushi Hori (R-CCS) mailto: ahori@riken.jp or ahori@me.com
 * $
 */

#ifndef _pip_gdbif_enums_h_
#define _pip_gdbif_enums_h_

/* make this independent from <pip.h> */
enum pip_gdbif_pipid {
  PIP_GDBIF_PIPID_ROOT		= -1,
  PIP_GDBIF_PIPID_ANY		= -2
};

enum pip_task_status {
  PIP_GDBIF_STATUS_NULL		= 0, /* just to make sure, there is nothing in this struct and invalid to access */
  PIP_GDBIF_STATUS_CREATED	= 1, /* just after pip_spawn() is called and this structure is created */
  /* Note: the order of state transition of the next two depends on implementation (option) */
  /* do to rely on the order of the next two. */
  PIP_GDBIF_STATUS_LOADED	= 2, /* just after calling dlmopen */
  PIP_GDBIF_STATUS_SPAWNED	= 3, /* just after calling pthread_create() or clone() */
  PIP_GDBIF_STATUS_TERMINATED	= 4  /* when the task is about to die (killed) */
};

/* gdb -> libpip */
enum pip_gdb_status {
  PIP_GDBIF_GDB_DETACHED	= 0, /* this structure can be freed */
  PIP_GDBIF_GDB_ATTACHED	= 1  /* gdb is using this, cannot be freed */
};

enum pip_task_exec_mode {	/* One of the value (except NULL) is set when this structure is created */
  /* and the value is left unchanged until the structure is put on the free list */
  PIP_GDBIF_EXMODE_NULL		= 0,
  PIP_GDBIF_EXMODE_PROCESS	= 1,
  PIP_GDBIF_EXMODE_THREAD	= 2
};

#define PIP_GDBIF_ROOT_VARNAME		"pip_gdbif_root"

#endif
