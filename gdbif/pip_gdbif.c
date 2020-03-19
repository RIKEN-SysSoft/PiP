#include <stddef.h>
#include <sys/wait.h>
#include <pip_gdbif.h>

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
const int PIP_GDBIF_ROOT_OFFSET_BEFOER_MAIN	= offsetof(struct pip_gdbif_root,hook_before_main);
const int PIP_GDBIF_ROOT_OFFSET_AFTER_MAIN	= offsetof(struct pip_gdbif_root,hook_after_main);
