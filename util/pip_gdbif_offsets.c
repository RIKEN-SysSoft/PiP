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
  MEMBER(struct pip_gdbif_task, argc);
  MEMBER(struct pip_gdbif_task, argv);
  MEMBER(struct pip_gdbif_task, envv);
  MEMBER(struct pip_gdbif_task, handle);
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
