/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019$
 * $PIP_VERSION: Version 1.0.0$
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

#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

#include "pip_machdep.h"
#include "pip_gdbif.h"

/*
 * utility program to write PIP-gdb/gdb/linux-tdep.c
 */

#define MEMBER(type, member) \
	printf("%s::%s offset:%zd size:%zd\n", \
	       #type, #member, offsetof(type, member), \
	       sizeof(((type *)0)->member));

int main() {

  printf("sizeof(struct pip_gdbif_task): %zd\n",
	 sizeof(struct pip_gdbif_task));

  MEMBER(struct pip_gdbif_task, task_list.hcqe_next);
  MEMBER(struct pip_gdbif_task, task_list.hcqe_prev);
  MEMBER(struct pip_gdbif_task, root);
  MEMBER(struct pip_gdbif_task, pathname);
  MEMBER(struct pip_gdbif_task, realpathname);
  MEMBER(struct pip_gdbif_task, argc);
  MEMBER(struct pip_gdbif_task, argv);
  MEMBER(struct pip_gdbif_task, envv);
  MEMBER(struct pip_gdbif_task, handle);
  MEMBER(struct pip_gdbif_task, load_address);
  MEMBER(struct pip_gdbif_task, pid);
  MEMBER(struct pip_gdbif_task, pipid);
  MEMBER(struct pip_gdbif_task, exit_code);
  MEMBER(struct pip_gdbif_task, exec_mode);
  MEMBER(struct pip_gdbif_task, status);
  MEMBER(struct pip_gdbif_task, gdb_status);

  putchar('\n');

  printf("sizeof(struct pip_gdbif_root): %zd\n",
	 sizeof(struct pip_gdbif_root));

  MEMBER(struct pip_gdbif_root, hook_before_main);
  MEMBER(struct pip_gdbif_root, hook_after_main);
  MEMBER(struct pip_gdbif_root, task_root);

  return 0;
}
