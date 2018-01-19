/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2018
*/

#ifndef _pip_gdb_if_h_
#define _pip_gdb_if_h_

enum pip_task_status_e {
  PIP_TASK_GDBIF_NULL		= 0,
  PIP_TASK_GDBIF_CREATED	= 1,
  PIP_TASK_GDBIF_LOADED		= 2,
  PIP_TASK_GDBIF_TERMINATED	= 3
};

enum pip_task_exec_mode_e {
  PIP_TASK_GDBIF_MODE_NULL	= 0,
  PIP_TASK_GDBIF_PROCESS	= 1,
  PIP_TASK_GDBIF_THREAD		= 2
};

struct pip_task_gdbif {
  /* double linked list */
  struct pip_task_gdbif *next;
  struct pip_task_gdbif *prev;
  /* PID or TID of PiP task */
  pid_t	pid;
  /* pathname of the program */
  char	*pathname;
  /* argc, argv and env */
  int	argc;
  char 	**argv;
  char 	**envv;
  /* handle of the link map */
  void	*handle;
  /* exit code */
  int	exit_code;
  /* hook function address */
  void	*hook_before_main;
  void	*hook_after_main;
  /* pip task exec mode */
  enum pip_task_exec_mode_e	exec_mode;
  /* task status, this is set by PiP lib */
  enum pip_task_status_e	status;
};

extern struct pip_task_gdbif	pip_task_gdbif_root;
extern struct pip_task_gdbif	pip_task_gdbif_freelist;

#endif /* _pip__gdb_if_h_ */
