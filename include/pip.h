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
  * Written by Atsushi HORI <ahori@riken.jp>, 2016-2018
*/

#ifndef _pip_h_
#define _pip_h_

/**
 * @addtogroup pip-overview pip-overview
 * \brief the PiP library
 * @{
 *
 * \section overview Overview
 *
 * PiP is a user-level library which allows a process to
 * create sub-processes into the same virtual address space where the
 * parent process runs. The parent process and sub-processes share the
 * same address space, however, each process has its own
 * variables. So, each process runs independently from the other
 * process. If some or all processes agreed, then data own by a
 * process can be accessed by the other processes.
 *
 * Those processes share the same address space, just like pthreads,
 * and each process has its own variables like a process. The parent
 * process is called \e PiP \e process and sub-processes are called
 * \e PiP \e task since it has the best of the both worlds of
 * processes and pthreads.
 *
 * PiP root can spawn one or more number of PiP tasks. The PiP root
 * and PiP tasks shared the
 * same address space. The executable of the PiP task must be
 * compiled (with the "-fpie" compile option) and linked (with the
 * "-pie" linker option) as PIE (Position Independent Executable).
 *
 * When a PiP root or PiP task wants to be accessed the its own data
 * by the other(s), firstly a memory region where the data to be
 * accessed are located must be \e exported. Then the exported memory
 * region is \e imported so that the exported and imported data can be
 * accessed. The PiP library supports the functions to export and
 * import the memory region to be accessible.
 *
 * \section PiP User-Level Process (ULP)
 *
 * PiP also supports ULPs which are explicitly scheduled by PiP
 * tasks. Unlike the other user-level thread libraries, PiP ULPs can
 * run with different programs. Due to the GLIBC constraints, ULPs can
 * be created by the PiP root process. The created ULPs are associated
 * with a PiP task by specifying ULPs when the task is created by
 * calling \e pip_spawn. A ULP can yield, suspend, and resume its
 * execution by calling PiP ULP functions. A PiP task will be
 * terminated only when all ULPs scheduled by the task terminate.
 *
 * \section execution-mode Execution mode
 *
 * There are several PiP implementation modes which can be selected at
 * the runtime. These implementations can be categorized into two
 * according to the behavior of PiP tasks,
 *
 * - \c Pthread, and
 * - \c Process.
 *
 * In the pthread mode, although each PiP task has its own variables
 * unlike thread, PiP task behaves more like Pthread, having no PID,
 * having the same file descriptor space, having the same signal
 * delivery semantics as Pthread does, and so on.
 *
 * In the process mode, PiP task behaves more like a process, having
 * a PID, having an independent file descriptor space, having the same
 * signal delivery semantics as Linux process does, and so on.
 *
 * When the \c PIP_MODE environment variable set to \"thread\" then
 * the PiP library runs based on the pthread mode, and it is set to
 * \"process\" then it runs with the process mode. There are also two
 * implementations in the \b process mode; \"process:preload\" and
 * \"process:pipclone\" The former one must be with the \b LD_PRELOAD
 * environment variable setting so that the \b clone() system call
 * wrapper can work with. The latter one can only be specified with
 * the PIP-patched glibc library (see below: \b GLIBC issues).
 *
 * There several function provided by the PiP library to absorb the
 * difference due to the execution mode
 *
 * \section limitation Limitation
 *
 * PiP allows PiP root and PiP tasks to share the data, so the
 * function pointer can be passed to the others. However, jumping into
 * the code owned by the other may not work properly for some
 * reasons.
 *
 * \section compile-and-link Compile and Link User programs
 *
 * The PiP root ust be linked with the PiP library and libpthread. The
 * programs able to run as a PiP task must be compiled with the
 * \"-fpie\" compile option and the \"-pie -rdynamic\" linker
 * options.
 *
 * \section glibc-issues GLIBC issues
 *
 * The PiP library is implemented at the user-level, i.e. no need of
 * kernel patches nor kernel modules. Due to the novel usage of
 * combining \c dlmopn() GLIBC function and \c clone() systemcall,
 * there are some issues found in the GLIBC. To avoid this issues,
 * PiP users may have the patched GLIBC provided by the PiP
 * development team.
 *
 * \section gdb-issue GDB issue
 *
 * Currently gdb debugger only works with the PiP root. PiP-aware GDB
 * will be provided soon.
 *
 * \section author Author
 *  Atsushi Hori (RIKEN, Japan) ahori@riken.jp
 *
 * @}
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#define PIP_OPTS_NONE			(0x0)

#define PIP_MODE_PTHREAD		(0x1000)
#define PIP_MODE_PROCESS		(0x2000)
/* the following two modes are a submode of PIP_MODE_PROCESS */
#define PIP_MODE_PROCESS_PRELOAD	(0x2100)
#define PIP_MODE_PROCESS_PIPCLONE	(0x2200)
#define PIP_MODE_MASK			(0xFF00)

#define PIP_ENV_MODE			"PIP_MODE"
#define PIP_ENV_MODE_THREAD		"thread"
#define PIP_ENV_MODE_PTHREAD		"pthread"
#define PIP_ENV_MODE_PROCESS		"process"
#define PIP_ENV_MODE_PROCESS_PRELOAD	"process:preload"
#define PIP_ENV_MODE_PROCESS_PIPCLONE	"process:pipclone"

#define PIP_OPT_MASK			(0xFF)
#define PIP_OPT_FORCEEXIT		(0x01)
#define PIP_OPT_PGRP			(0x02)

#define PIP_ENV_OPTS			"PIP_OPTS"
#define PIP_ENV_OPTS_FORCEEXIT		"forceexit"
#define PIP_ENV_OPTS_PGRP		"pgrp"

#define PIP_VALID_OPTS	\
  ( PIP_MODE_PTHREAD | PIP_MODE_PROCESS_PRELOAD | PIP_MODE_PROCESS_PIPCLONE | \
    PIP_OPT_FORCEEXIT | PIP_OPT_PGRP )

#define PIP_ENV_STACKSZ		"PIP_STACKSZ"

#define PIP_PIPID_ROOT		(-1)
#define PIP_PIPID_MYSELF	(-2)
#define PIP_PIPID_SELF		(-2)
#define PIP_PIPID_ANY		(-3)
#define PIP_PIPID_NULL		(-4)

#define PIP_NTASKS_MAX		(300)

#define PIP_CPUCORE_ASIS 	(-1)

#define PIP_ULP_SCHED_FIFO	(0x0)
#define PIP_ULP_SCHED_LIFO	(0x1)

#define PIP_BARRIER_INIT(N)		{(N),(N),0}
#define PIP_TASK_BARRIER_INIT(N)	{(N),(N),0}

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <pthread.h>
#include <link.h>
#include <dlfcn.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <pip_machdep.h>

typedef struct pip_barrier {
  union {
    struct {
      int		count_init;
      pip_atomic_t	count;
      volatile int	gsense;
    };
    char		__gap__[PIP_CACHEBLK_SZ];
  };
} pip_barrier_t;

typedef struct pip_ulp_queue {
  struct pip_ulp_queue		*next;
  struct pip_ulp_queue		*prev;
} pip_ulp_queue_t;

typedef pip_ulp_queue_t		pip_list_t;
typedef pip_ulp_queue_t		pip_ulp_t;

typedef struct {
  char		*prog;
  char		**argv;
  char		**envv;
  char		*funcname;
  void		*arg;
  void 		*reserved[3];
} pip_spawn_program_t;

typedef int (*pip_spawnhook_t)( void* );
typedef int (*pip_startfunc_t)( void* );

typedef struct {
  pip_spawnhook_t	before;
  pip_spawnhook_t	after;
  void 			*hookarg;
} pip_spawn_hook_t;

#ifdef __cplusplus
extern "C" {
#endif

#endif /* DOXYGEN_SHOULD_SKIP_THIS */
/**
 * @addtogroup libpip libpip
 * \brief the PiP library
 * @{
 */

  /**
   * \brief Setting information to invoke aa PiP task
   *  @{
   * \param[in,out] progp Pointer to the \t pip_spawn_program_t
   *  structure in which the program invokation information is set
   * \param[in] prog Filename of the program
   * \param[in] argv Argument vector
   * \param[in] envv Environment variables
   *
   * This function sets the required information to invoke a program,
   * starting from the \t main() function.
   *
   * \sa pip_task_spawn(3), pip_spawn_from_func(3)
   *
   */
static inline void pip_spawn_from_main( pip_spawn_program_t *progp,
					char *prog, char **argv, char **envv ) {
  if( prog != NULL ) {
    progp->prog   = prog;
  } else {
    progp->prog   = argv[0];
  }
  progp->argv     = argv;
  progp->envv     = envv;
  progp->funcname = NULL;
  progp->arg      = NULL;
}
  /** @}*/

  /**
   * \brief Setting information to invoke a PiP task
   *  @{
   * \param[in,out] progp Pointer to the \c pip_spawn_program_t
   *  structure in which the program invokation information is set
   * \param[in] prog Filename of the program
   * \param[in] funcname Function name to be started
   * \param[in] envv Environment variables
   *
   * This function sets the required information to invoke a program,
   * starting from the \c main() function.
   *
   * \sa pip_task_spawn(3), pip_spawn_from_main(3)
   *
   */
static inline void
pip_spawn_from_func( pip_spawn_program_t *progp,
		     char *prog, char *funcname, void *arg, char **envv ) {
  progp->prog     = prog;
  progp->argv     = NULL;
  progp->envv     = envv;
  progp->funcname = funcname;
  progp->arg      = arg;
}
  /** @}*/

  /**
   * \brief Setting invokation hook information
   *  @{
   * \param[in] before Just before the executing of the spawned PiP
   *  task, this function is called so that file descriptors inherited
   *  from the PiP root, for example, can deal with. This is only
   *  effective with the PiP process mode. This function is called
   *  with the argument \a hookarg described below.
   * \param[in] after This function is called when the PiP task
   *  terminates for the cleanup purpose. This function is called
   *  with the argument \a hookarg described below.
   * \param[in] hookarg The argument for the \a before and \a after
   *  function call.
   *
   * The \a before and \a after functions are introduced to follow the
   * programming style of conventional \c fork and \c
   * exec. \a before function does the prologue found between the
   * \c fork and \c exec. \a after function is to free the argument if
   * it is \c malloc()ed. Note that the \a before and \a after
   * functions are called in the different \e context from the spawned
   * PiP task. More specifically, any variables defined in the spawned
   * PiP task cannot be accessible from the \a before and \a after
   * functions.
   *
   * \sa pip_task_spawn(3)
   *
   */
static inline void pip_spawn_hook( pip_spawn_hook_t *hook,
				   pip_spawnhook_t before,
				   pip_spawnhook_t after,
				   void *hookarg ) {
  hook->before  = before;
  hook->after   = after;
  hook->hookarg = hookarg;
}
  /** @}*/

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif

  /**
   * \brief Initialize the PiP library.
   *  @{
   * \param[out] pipidp When this is called by the PiP root
   *  process, then this returns PIP_PIPID_ROOT, otherwise it returns
   *  the PIPID of the calling PiP task.
   * \param[in,out] ntasks When called by the PiP root, it specifies
   *  the maximum number of PiP tasks. When called by a PiP task, then
   *  it returns the number specified by the PiP root.
   * \param[in,out] root_expp If the root PiP is ready to export a
   *  memory region to any PiP task(s), then this parameter points to
   *  the variable holding the exporting address of the root PiP. If
   *  the PiP root is not ready to export or has nothing to export
   *  then this variable can be NULL. When called by a PiP task, it
   *  returns the exporting address of the PiP root, if any.
   * \param[in] opts This must be zero at the point of this writing.
   *
   * \return zero is returned if this function succeeds. On error, error number is returned.
   * \retval EINVAL \e notasks is a negative number, or the option combination is ivalid
   * \retval EOVERFLOW \c notasks is too latrge
   * \retval ENOMEM unable to allocate memory
   *
   * This function initializes the PiP library. The PiP root process
   * must call this. A PiP task is not required to call this function
   * unless the PiP task calls any PiP functions.
   *
   * Is is NOT guaranteed that users can spawn tasks up to the number
   * specified by the \a ntasks argument. There are some limitations
   * come from outside of the PiP library (GLIBC).
   *
   * \sa pip_export(3), pip_fin(3)
   */
  int pip_init( int *pipidp, int *ntasks, void **root_expp, int opts );
  /** @}*/

  /**
   * \brief finalize the PiP library.
   *  @{
   * \return zero is returned if this function succeeds. On error, error number is returned.
   * \retval EBUSY \c one or more PiP tasks is yet running
   *
   * This function finalize the PiP library.
   *
   * \sa pip_init(3)
   */
  int pip_fin( void );
  /** @}*/


  /**
   * \brief spawn a PiP task (ULP API Version 2)
   *  @{
   * \param[in] progp Program information to spawn as a PiP task
   * \param[in] coreno Core number for the PiP task to be bound to. If
   *  \c PIP_CPUCORE_ASIS is specified, then the core binding will not
   *  take place.
   * \param[in,out] pipidp Specify PIPID of the spawned PiP task. If
   *  \c PIP_PIPID_ANY is specified, then the PIPID of the spawned PiP
   *  task is up to the PiP library and the assigned PIPID will be
   *  returned.
   * \param[in] hookp Hook information to be invoked before and after
   *  the program invokation.
   * \param[in] sisters List of ULPs to be scheduled by this task
   *
   * \return zero is returned if this function succeeds. On error, error number is returned.
   * \retval EPERM PiP task tries to spawn child task
   * \retval EBUSY Specified PIPID is alredy occupied
   *
   * \note In theory, there is no reason to restrict for a PiP task to
   * spawn another PiP task. However, the current implementation fails
   * to do so. If the root process is multithreaded, only the main
   * thread can call this function.
   *
   * \sa pip_task_spawn(3), pip_spawn_from_main(3), pip_ulp_new(3)
   *
   */
int pip_task_spawn( pip_spawn_program_t *progp,
		    int coreno,
		    int *pipidp,
		    pip_spawn_hook_t *hookp,
		    pip_ulp_t *sisters );
  /** @}*/


  /**
   * \brief export an address of the calling PiP root or a PiP task to
   * the others.
   *  @{
   * \param[in] exp Starting address of a memory region of the calling
   *  process or task so that the other tasks can access.
   * \param[in] format a \c printf format to give the exported address a name
   *
   * \return Return 0 on success. Return an error code on error.
   *
   * The PiP root or a PiP task can export a memory region only
   * once.
   *
   * \note The exported address can only be retrieved by \c pip_named_import(3).
   * \note There is no size parameter to specify the length of the
   * exported region because there is no way to restrict the access
   * outside of the exported region.
   *
   * \sa pip_named_import(3)
   */
  int pip_named_export( void *exp, const char *format, ... )
    __attribute__ ((format (printf, 2, 3)));
  /** @}*/

  /**
   * \brief import the exposed memory region of the other.
   *  @{
   * \param[in] pipid The PIPID to import the exposed address
   * \param[out] expp The starting address of the exposed region of
   *  the PiP task specified by the \a pipid.
   * \param[in] format a \c printf format to give the exported address a name
   *
   * \return Return 0 on success. Return an error code on error.
   *
   * \note To avoid deadlock, the corresponding \c pip_named_export(3)
   * must be called beofre calling \c pip_named_import(3);
   * \note Unlike \c pip_import(3), this function might be blocked until the
   * target address is exported by the target task. Once a name is
   * associated by an address, the address associated with the name
   * cannot be changed.
   * \note If this function is called by a ULP or a task having
   * ULP(s), then this call may result in context switching to the
   * other ULP.
   *
   * \sa pip_named_export(3)
   */
  int pip_named_import( int pipid, void **expp, const char *format, ... )
    __attribute__ ((format (printf, 3, 4)));
  /** @}*/

  /**
   * \brief non-blocking version of \c pip_named_import
   *  @{
   * \param[in] pipid The PIPID to import the exposed address
   * \param[out] expp The starting address of the exposed region of
   *  the PiP task specified by the \a pipid.
   * \param[in] format a \c printf format to give the exported address a name
   *
   * \return Return 0 on success. Return an error code on error.
   *
   * \note The imported address must be exported by \c pip_named_export(3).
   * \note When the named export cannot be found at the specified
   * task, then this function returns immediately. It is guaranteed
   * that the will be no ULP context switching take place in this
   * function call.
   *
   * \sa pip_named_export(3)
   */
  int pip_named_tryimport( int pipid, void **expp, const char *format, ... )
    __attribute__ ((format (printf, 3, 4)));
  /** @}*/

  /**
   * \brief export a memory region of the calling PiP root or a PiP task to
   * the others.
   *  @{
   * \param[in] exp Starting address of a memory region of the calling
   *  process or task to the others.
   *  function call.
   *
   * \return Return 0 on success. Return an error code on error.
   *
   * The PiP root or a PiP task can export a memory region only
   * once.
   *
   * \note There is no size parameter to specify the length of the
   * exported region because there is no way to restrict the access
   * outside of the exported region.
   *
   * \sa pip_import(3)
   */
  int pip_export( void *exp );
  /** @}*/

  /**
   * \brief import the exposed memory region of the other.
   *  @{
   * \param[in] pipid The PIPID to import the exposed address
   * \param[out] expp The starting address of the exposed region of
   *  the PiP task specified by the \a pipid.
   *
   * \return Return 0 on success. Return an error code on error.
   *
   * \note It is the users' responsibility to synchronize. When the
   * target region is not exported yet , then this function returns
   * NULL. If the root exports its region by the \c pip_init function
   * call, then it is guaranteed to be imported by PiP tasks at any
   * time.
   *
   * \sa pip_export(3)
   */
  int pip_import( int pipid, void **expp );
  /** @}*/

  /**
   * \brief get PIPID
   *  @{
   * \param[out] pipidp This parameter points to the variable which
   *  will be set to the PIPID of the calling process.
   *
   * \return Return 0 on success. Return an error code on error.
   *
   */
  int pip_get_pipid( int *pipidp );
  /** @}*/

  /**
   * \brief get the maximum number of the PiP tasks
   *  @{
   * \param[out] ntasksp This parameter points to the variable which
   *  will be set to the maximum number of the PiP tasks.
   *
   * \return Return 0 on success. Return an error code on error.
   *
   */
  int pip_get_ntasks( int *ntasksp );
  /** @}*/

  /**
   * \brief check if the calling task is a PiP task or not
   *  @{
   *
   * \return Return 0 on success. Return an error code on error.
   *
   * \note Unlike most of the other PiP functions, this can be called
   * BEFORE calling the \c pip_init() function.
   */
  int pip_isa_piptask( void );
  /** @}*/

  /**
   * \brief get the PiP execution mode
   *  @{
   * \param[out] modep This parameter points to the variable which
   *  will be set to the PiP execution mode
   *
   * \return Return 0 on success. Return an error code on error.
   *
   */
  int pip_get_mode( int *modep );
  /** @}*/

  /**
   * \brief terminate PiP task or ULP
   *  @{
   * \param[in] retval Terminate PiP task or ULP with the exit number
   * specified with this parameter.
   *
   * \note This function can be used regardless to the PiP execution
   * mode.
   * \note If this function is called by a PiP task having one or more
   * ULPs then the actual termination of the PiP task is postponed
   * until all the associated (scheduling) ULP(s) terminate(s).
   *
   * \return Return 0 on success. Return an error code on error.
   *
   * \sa pip_wait(3), pip_trywait(3), pip_wait_any(3), pip_trywait_any(3)
   *
   */
  int pip_exit( int retval );
  /** @}*/

  /**
   * \brief wait for the termination of a PiP task
   *  @{
   * \param[in] pipid PIPID to wait for.
   * \param[out] retval Exit value of the terminated PiP task
   *
   * \note This function blocks until the specified PiP task or ULP
   * terminates.
   * \note This function can be used regardless to the PiP execution
   * mode. However, only the least significant 2 bytes of the exit value are
   * effective. This is because of the compatibility with the
   * \c exit glibc function.
   *
   * \return Return 0 on success. Return an error code on error.
   *
   * \sa pip_exit(3), pip_trywait(3), pip_wait_any(3), pip_trywait_any(3)
   *
   */
  int pip_wait( int pipid, int *retval );
  /** @}*/

  /**
   * \brief wait for the termination of a PiP task in a non-blocking way
   *  @{
   * \param[in] pipid PIPID to wait for.
   * \param[out] retval Exit value of the terminated PiP task
   *
   * \note This function can be used regardless to the PiP execution
   * mode.
   *
   * \return Return 0 on success. Return an error code on error.
   *
   * \sa pip_exit(3), pip_wait(3), pip_wait_any(3), pip_trywait_any(3)
   *
   */
  int pip_trywait( int pipid, int *retval );
  /** @}*/

  /**
   * \brief wait for the termination of any PiP task
   *  @{
   * \param[out] pipid PIPID of terminated PiP task.
   * \param[out] retval Exit value of the terminated PiP task
   *
   * \note This function blocks until one of PiP tasks or ULPs
   * terminates.
   * \note This function can be used regardless to the PiP execution
   * mode. However, only the least significant 2 bytes are
   * effective. This is because of the compatibility with the
   * \c exit glibc function.
   *
   * \return Return 0 on success. Return an error code on error.
   *
   * \sa pip_exit(3), pip_wait(3), pip_trywait(3), pip_trywait_any(3)
   *
   */
  int pip_wait_any( int *pipid, int *retval );
  /** @}*/

  /**
   * \brief wait for the termination of any PiP task
   *  @{
   * \param[out] pipid PIPID of terminated PiP task.
   * \param[out] retval Exit value of the terminated PiP task
   *
   * \note This function never blocks.
   * \note This function can be used regardless to the PiP execution
   * mode. However, only the least significant 2 bytes are
   * effective. This is because of the compatibility with the
   * \c exit glibc function.
   *
   * \return Return 0 on success. Return an error code on error.
   *
   * \sa pip_exit(3), pip_wait(3), pip_trywait(3), pip_wait_any(3)
   *
   */
  int pip_trywait_any( int *pipid, int *retval );
  /** @}*/

  /**
   * \brief deliver a signal to a PiP task
   *  @{
   * \param[out] pipid PIPID of a target PiP task
   * \param[out] signal signal number to be delivered
   *
   * \note Only the PiP task can be the target of the signal delivery.
   * \note This function can be used regardless to the PiP execution
   * mode.
   *
   * \return Return 0 on success. Return an error code on error.
   *
   */
  int pip_kill( int pipid, int signal );
  /** @}*/

  /**
   * \brief deliver a process or thread ID
   *  @{
   * \param[out] pipid PIPID of a target PiP task
   * \param[out] idp a pointer to store the ID value
   *
   * \note This function can be used regardless to the PiP execution
   * mode.
   *
   * \return Return 0 on success. Return an error code on error.
   *
   */
  int pip_get_id( int pipid, intptr_t *idp );
  /** @}*/

  /**
   * \brief get a string of the current execution mode
   *  @{
   *
   * \note This function can be used regardless to the PiP execution
   * mode.
   *
   * \return Return the name string of the current execution mode
   *
   */
  const char *pip_get_mode_str( void );
  /** @}*/

  /**
   * \brief initialize barrier synchronization structure
   *  @{
   *
   * \param[in] barrp pointer to a PiP barrier structure
   * \param[in] n number of participants of this barrier
   * synchronization
   *
   * \sa pip_task_barrier_wait(3), pip_universal_barrier_init(3), pip_universal_barrier_wait(3)
   */
  int pip_task_barrier_init( pip_barrier_t *barrp, int n );
  /** @}*/

  /**
   * \brief wait on barrier synchronization in a busy-wait way
   *  @{
   *
   * \param[in] barrp pointer to a PiP barrier structure
   *
   * \sa pip_task_barrier_init(3), pip_universal_barrier_init(3), pip_universal_barrier_wait(3)
   *
   */
  int pip_task_barrier_wait( pip_barrier_t *barrp );
  /** @}*/

  /**
   * \brief check if calling PiP task is PiP root or not
   *  @{
   *
   */
  int  pip_is_root( void );
  /** @}*/

  /**
   * \brief check if calling PiP task is a PiP task or not
   *  @{
   *
   */
  int  pip_is_task( void );
  /** @}*/

  /**
   * \brief check if calling PiP task is a PiP ULP or not
   *  @{
   *
   */
  int  pip_is_ulp(  void );
  /** @}*/

  /**
   * \brief check if the specified PiP task is alive or not
   *  @{
   *
   */
  int pip_is_alive( int pipid );
  /** @}*/


#ifdef PIP_EXPERIMENTAL
  void *pip_malloc( size_t size );
  void  pip_free( void *ptr );
#endif

#ifdef __cplusplus
}
#endif

#ifndef DOXYGEN_SHOULD_SKIP_THIS


#ifdef __cplusplus
extern "C" {
#endif

  int pip_barrier_init( pip_barrier_t *barrp, int n );
  int pip_barrier_wait( pip_barrier_t *barrp );

  /** DEPRECATED
   * \brief spawn a PiP task (ULP API Version 1)
   *  @{
   * \param[in] filename The executable to run as a PiP task
   * \param[in] argv Argument(s) for the spawned PiP task
   * \param[in] envv Environment variables for the spawned PiP task
   * \param[in] coreno Core number for the PiP task to be bound to. If
   *  \c PIP_CPUCORE_ASIS is specified, then the core binding will not
   *  take place.
   * \param[in,out] pipidp Specify PIPID of the spawned PiP task. If
   *  \c PIP_PIPID_ANY is specified, then the PIPID of the spawned PiP
   *  task is up to the PiP library and the assigned PIPID will be
   *  returned.
   * \param[in] before Just before the executing of the spawned PiP
   *  task, this function is called so that file descriptors inherited
   *  from the PiP root, for example, can deal with. This is only
   *  effective with the PiP process mode. This function is called
   *  with the argument \a hookarg described below.
   * \param[in] after This function is called when the PiP task
   *  terminates for the cleanup purpose. This function is called
   *  with the argument \a hookarg described below.
   * \param[in] hookarg The argument for the \a before and \a after
   *  function call.
   *
   * \return Return 0 on success. Return an error code on error.
   *
   * This function is to spawn
   * a PiP task. These functions are introduced to follow the
   * programming style of conventional \c fork and \c
   * exec. \a before function does the prologue found between the
   * \c fork and \c exec. \a after function is to free the argument if
   * it is \c malloc()ed. Note that the \a before and \a after
   * functions are called in the different \e context from the spawned
   * PiP task. More specifically, any variables defined in the spawned
   * PiP task cannot be accessible from the \a before and \a after
   * functions.
   *
   * \note In theory, there is no reason to restrict for a PiP task to
   * spawn another PiP task. However, the current implementation fails
   * to do so.
   */
  int pip_spawn( char *filename, char **argv, char **envv,
		 int coreno, int *pipidp,
		 pip_spawnhook_t before, pip_spawnhook_t after, void *hookarg);
  /** @}*/

  /**  DEPRECATED
   * \brief import the exposed memory region of the other.
   *  @{
   * \param[in] pipid The PIPID to import the exposed address
   * \param[in] symnam The name of a symbol existing in the specified PiP task
   * \param[out] addrp The address of the variable of
   *  the PiP task specified by the \a pipid.
   *
   * \return Return 0 on success. Return an error code on error.
   *
   * \note pip_get_addr() function is unable to get proper addresses
   * for local (static) or TLS variables.
   *
   * \note Although the pip_get_addr() fucntion can be used to get a
   * function address, calling the function of the other PiP task by
   * its address is very tricky and it may result in an unexpected
   * bahavior.
   *
   * \note By definition of the dlsym() Glibc function, this may
   * return NULL even if the variable having the specified name exists.
   */
  int pip_get_addr( int pipid, const char *symnam, void **addrp );
  /** @}*/

  int    pip_idstr( char *buf, size_t sz );

#ifdef __cplusplus
}
#endif

#ifdef PIP_INTERNAL_FUNCS

#include <pip_internal.h>

#ifdef __cplusplus
extern "C" {
#endif
  /* the following functions deeply depends on PiP execution mode */
  char  *pip_type_str( void );
  int    pip_get_thread( int pipid, pthread_t *threadp );
  int    pip_is_pthread( int *flagp );
  int    pip_is_shared_fd( int *flagp );
  int    pip_is_shared_sighand( int *flagp );
#ifdef __cplusplus
}
#endif
#endif /* PIP_INTERNAL_FUNCS */

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

/**
 * @}
 */

#endif	/* _pip_h_ */
