/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2018
*/

#ifndef _pip_gdbif_h_
#define _pip_gdbif_h_

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

struct pip_gdbif_task {
  /* double linked list */
  /* assuming GDB only dereferencing the next pointer */
  struct pip_gdbif_task *next;
  struct pip_gdbif_task *prev;
  struct pip_gdbif_task *root;
  /* pathname of the program */
  char	*pathname;
  /* argc, argv and env */
  int	argc;
  char 	**argv;
  char 	**envv;
  /* handle of the loaded link map */
  void	*handle;
  /* PID or TID of the PiP task, the value of zero means nothing */
  pid_t	pid;
  /* exit code, this value is set when the PiP task */
  /* gets PIP_GDBIF_STATUS_TERMINATED */
  int	exit_code;
  /* pip task exec mode */
  enum pip_task_exec_mode	exec_mode;
  /* task status, this is set by PiP lib */
  enum pip_task_status		status;
  /* the variable(s) below are set/reset by GDB */
  enum pip_gdb_status		gdb_status;
};

typedef void(*pip_gdbif_hook_t)(struct pip_gdbif_task*);

struct pip_gdbif_root {
  pip_spinlock_t	lock_root; /* lock for task_root */
  struct pip_gdbif_task	task_root;

  pip_spinlock_t	lock_free; /* lock for task_free */
  struct pip_gdbif_task	*task_free;

  /* hook function address, these addresses are set */
  /* when the PiP task gets PIP_GDBIF_STATUS_LOADED */
  pip_gdbif_hook_t	hook_before_main;
  pip_gdbif_hook_t	hook_after_main;
};

#endif /* _pip_gdbif_h_ */
