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
  * Written by Atsushi HORI <ahori@riken.jp>
*/

#ifndef _pip_h_
#define _pip_h_

#ifndef DOXYGEN_INPROGRESS

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>

#define PIP_OPTS_NONE			(0x0)

#define PIP_MODE_PTHREAD		(0x1000)
#define PIP_MODE_PROCESS		(0x2000)
/* the following two modes are a submode of PIP_MODE_PROCESS */
#define PIP_MODE_PROCESS_PRELOAD	(0x2100)
#define PIP_MODE_PROCESS_PIPCLONE	(0x2200)
#define PIP_MODE_PROCESS_GOT		(0x2400)
#define PIP_MODE_MASK			(0xFF00)

#define PIP_ENV_MODE			"PIP_MODE"
#define PIP_ENV_MODE_THREAD		"thread"
#define PIP_ENV_MODE_PTHREAD		"pthread"
#define PIP_ENV_MODE_PROCESS		"process"
#define PIP_ENV_MODE_PROCESS_PRELOAD	"process:preload"
#define PIP_ENV_MODE_PROCESS_PIPCLONE	"process:pipclone"
#define PIP_ENV_MODE_PROCESS_GOT	"process:got"

#define PIP_ENV_STOP_ON_START		"PIP_STOP_ON_START"

#define PIP_ENV_GDB_PATH		"PIP_GDB_PATH"
#define PIP_ENV_GDB_COMMAND		"PIP_GDB_COMMAND"
#define PIP_ENV_GDB_SIGNALS		"PIP_GDB_SIGNALS"
#define PIP_ENV_SHOW_MAPS		"PIP_SHOW_MAPS"
#define PIP_ENV_SHOW_PIPS		"PIP_SHOW_PIPS"

#define PIP_VALID_OPTS	\
  ( PIP_MODE_PTHREAD | PIP_MODE_PROCESS_PRELOAD | \
    PIP_MODE_PROCESS_PIPCLONE | PIP_MODE_PROCESS_GOT )

#define PIP_ENV_STACKSZ			"PIP_STACKSZ"

#define PIP_MAGIC_NUM			(-747) /* "PIP" P=7, I=4 */

#define PIP_PIPID_ROOT			(PIP_MAGIC_NUM-1)
#define PIP_PIPID_MYSELF		(PIP_MAGIC_NUM-2)
#define PIP_PIPID_ANY			(PIP_MAGIC_NUM-3)
#define PIP_PIPID_NULL			(PIP_MAGIC_NUM-4)
#define PIP_PIPID_SELF			PIP_PIPID_MYSELF

#define PIP_NTASKS_MAX			(300)

#define PIP_CPUCORE_FLAG_SHIFT		(24)
#define PIP_CPUCORE_CORENO_MASK		((1<<PIP_CPUCORE_FLAG_SHIFT)-1)
#define PIP_CPUCORE_CORENO_MAX		PIP_CPUCORE_CORENO_MASK
#define PIP_CPUCORE_ASIS 		(0x01)
#define PIP_CPUCORE_ABS 		(0x02)
#define PIP_CPUCORE_CORENO_MASK		((1<<PIP_CPUCORE_FLAG_SHIFT)-1)

typedef struct {
  char		*prog;
  char		**argv;
  char		**envv;
  char		*funcname;
  void		*arg;
  void		*exp;
  void		*reserved[2];
} pip_spawn_program_t;

typedef int (*pip_spawnhook_t)( void* );

typedef struct {
  pip_spawnhook_t	before;
  pip_spawnhook_t	after;
  void 			*hookarg;
  void 			*reserved[5];
} pip_spawn_hook_t;

typedef uintptr_t	pip_id_t;

#ifdef __cplusplus
extern "C" {
#endif

#endif /* DOXYGEN */

#ifndef INLINE
#define INLINE			inline static
#endif

  /**
   * \defgroup pip-0-init-fin PiP Initialization/Finalization
   * @{
   * \page pip-intialize-finalize PiP Initialization/Finalization
   * \description PiP initialization/finalization functions
   */
  /**
   * \page pip_init pip_init
   *
   * \name
   * Initialize the PiP library
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_init( int *pipidp, int *ntasks, void **root_expp, uint32_t opts );
   *
   * \description
   * This function initializes the PiP library. The PiP root process
   * must call this. A PiP task is not required to call this function
   * unless the PiP task calls any PiP functions.
   * \par
   * When this function is called by a PiP root, \c ntasks, and \c root_expp
   * are input parameters. If this is called by a PiP task, then those parameters are output
   * returning the same values input by the root.
   * \par
   * A PiP task may or may not call this function. If \c pip_init is not called by a PiP task
   * explicitly, then \c pip_init is called magically and implicitly even if the PiP task program
   * is NOT linked with the PiP library.
   *
   * \param[out] pipidp When this is called by the PiP root
   *  process, then this returns \c PIP_PIPID_ROOT, otherwise it returns
   *  the PiP ID of the calling PiP task.
   * \param[in,out] ntasks When called by the PiP root, it specifies
   *  the maximum number of PiP tasks. When called by a PiP task, then
   *  it returns the number specified by the PiP root.
   * \param[in,out] root_expp If the root PiP is ready to export a
   *  memory region to any PiP task(s), then this parameter is to pass
   *  the exporting address. If
   *  the PiP root is not ready to export or has nothing to export
   *  then this variable can be NULL. When called by a PiP task, it
   *  returns the exported address by the PiP root, if any.
   * \param[in] opts Specifying the PiP execution mode and See below.
   *
   * \notes
   * The \c opts may have one of the defined values \c PIP_MODE_PTHREAD,
   * \c PIP_MODE_PROCESS, \c PIP_MODE_PROCESS_PRELOAD,
   * \c PIP_MODE_PROCESS_PIPCLONE and \c PIP_MODE_PROCESS_GOT, or
   * any combination (bit-wise or) of them. If combined or \c opts is zero,
   * then an
   * appropriate one is chosen by the library. This PiP execution mode
   * can be specified by an environment variable described below.
   *
   * \return Zero is returned if this function succeeds. Otherwise an
   * error number is returned.
   *
   * \retval EINVAL \a ntasks is negative
   * \retval EBUSY PiP root called this function twice or more without calling \c
   * pip_fin(1).
   * \retval EPERM \a opts is invalid or unacceptable
   * \retval EOVERFLOW \a ntasks is too large
   * \retval ELIBSCN verssion miss-match between PiP root and PiP task
   *
   * \environment
   * \arg \b PIP_MODE Specifying the PiP execution mmode. Its value can be
   * either \c thread, \c pthread, \c process, \c process:preload,
   * \c process:pipclone, or \c process:got.
   * \arg \b LD_PRELOAD This is required to set appropriately to hold the path
   * to \c pip_preload.so file, if the PiP execution mode is
   * \c PIP_MODE_PROCESS_PRELOAD (the \c opts in \c pip_init) and/or
   * the PIP_MODE ennvironment is set to \c process:preload. See also
   * the pip_mode(1) command to set the environment variable appropriately and
   * easily.
   * \arg \b PIP_GDB_PATH If thisenvironment is set to the path pointing to the PiP-gdb
   * executable file, then PiP-gdb is automatically attached when an
   * excetion signal (SIGSEGV and SIGHUP by default) is delivered. The signals which
   * triggers the PiP-gdb invokation can be specified the
   * \c PIP_GDB_SIGNALS environment described below.
   * \arg \b PIP_GDB_COMMAND If this PIP_GDB_COMMAND is set to a filename
   * containing some GDB commands, then those GDB commands will be executed by the GDB
   * in batch mode, instead of backtrace.
   * \arg \b PIP_GDB_SIGNALS Specifying the signal(s) resulting automatic PiP-gdb attach.
   * Signal names (case insensitive) can be concatenated by the '+' or '-' symbol.
   * 'all' is reserved to specify most of the signals. For example, 'ALL-TERM'
   * means all signals excepting \c SIGTERM, another example, 'PIPE+INT' means \c SIGPIPE
   * and \c SIGINT. Some signals such as SIGKILL and SIGCONT cannot be specified.
   * \arg \b PIP_SHOW_MAPS If the value is 'on' and one of the above exection signals is delivered,
   * then the memory map will be shown.
   * \arg \b PIP_SHOW_PIPS If the value is 'on' and one of the above exection signals is delivered,
   * then the process status by using the \c pips command (see also pips(1)) will be shown.
   *
   * \bugs
   * Is is NOT guaranteed that users can spawn tasks up to the number
   * specified by the \a ntasks argument. There are some limitations
   * come from outside of the PiP library (from GLIBC).
   * \n\n
   * \sa pip_named_export(3), pip_export(3), pip_fin(3), pip-mode(1), pips(1)
   * \n\n
   */
  int pip_init( int *pipidp, int *ntasks, void **root_expp, uint32_t opts );

  /**
   * \page pip_fin pip_fin
   *
   * \name
   * Finalize the PiP library
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_fin( void );
   *
   * \description
   * This function finalizes the PiP library. After calling this, most
   * of the PiP functions will return the error code \c EPERM.
   *
   * \return zero is returned if this function succeeds. On error, error number is returned.
   * \retval EPERM \c pip_init is not yet called
   * \retval EBUSY \c one or more PiP tasks are not yet terminated
   *
   * \notes
   * The behavior of calling \c pip_init after calling this \c pip_fin
   * is note defined and recommended to do so.
   *
   * \n\n
   * \sa pip_init(3)
   */
  int pip_fin( void );

  /**
   * @}
   */

  /**
   * \defgroup pip-1-spawn Spawning PiP task
   * @{
   * \page pip-spawn PiP Spawnig PiP (ULP/BLT) task
   * \description Spawning PiP task or ULP/BLT task
   */

  /**
   * \page pip_spawn_from_main pip_spawn_from_main
   *
   * \brief Setting information to invoke a PiP task starting from the
   *  main function
   *
   * \synopsis
   * \#include <pip.h> \n
   * void pip_spawn_from_main( pip_spawn_program_t *progp,
   *			       char *prog, char **argv, char **envv,
   *		               void *exp )
   *
   * \description
   * This function sets up the \c pip_spawn_program_t structure for
   * spawning a PiP task, starting from the mmain function.
   *
   * \param[out] progp Pointer to the \c pip_spawn_program_t
   *  structure in which the program invokation information will be set
   * \param[in] prog Path to the executiable file.
   * \param[in] argv Argument vector.
   * \param[in] envv Environment variables. If this is \c NULL, then
   * the \c environ variable is used for the spawning PiP task.
   * \param[in] exp Export value to the spawning PiP task
   *
   * \sa pip_task_spawn(3), pip_spawn_from_func(3)
   *
   */
#ifndef DOXYGEN_INPROGRESS
INLINE
#endif
void pip_spawn_from_main( pip_spawn_program_t *progp,
			  char *prog, char **argv, char **envv,
			  void *exp ) {
  memset( progp, 0, sizeof(pip_spawn_program_t) );
  if( prog != NULL ) {
    progp->prog   = prog;
  } else {
    progp->prog   = argv[0];
  }
  progp->argv     = argv;
  if( envv == NULL ) {
    extern char **environ;
    progp->envv = environ;
  } else {
    progp->envv = envv;
  }
  progp->exp = exp;
}

  /**
   * \page pip_spawn_from_func pip_spawn_from_func
   *
   * \brief Setting information to invoke a PiP task starting from a
   * function defined in a program
   *
   * \synopsis
   * \#include <pip.h> \n
   * pip_spawn_from_func( pip_spawn_program_t *progp,
   *		     char *prog, char *funcname, void *arg, char **envv,
   *		     void *exp );
   *
   * \description
   * This function sets the required information to invoke a program,
   * starting from the \c main() function. The function should have
   * the function prototype as shown below;
     \code
     int start_func( void *arg )
     \endcode
   * This start function must be globally defined in the program..
   * The returned integer of the start function will be treated in the
   * same way as the \c main function. This implies that the
   * \c pip_wait function family called from the PiP root can retrieve
   * the return code.
   *
   * \param[out] progp Pointer to the \c pip_spawn_program_t
   *  structure in which the program invokation information will be set
   * \param[in] prog Path to the executiable file.
   * \param[in] funcname Function name to be started
   * \param[in] arg Argument which will be passed to the start function
   * \param[in] envv Environment variables. If this is \c NULL, then
   * the \c environ variable is used for the spawning PiP task.
   * \param[in] exp Export value to the spawning PiP task
   *
   * \sa pip_task_spawn(3), pip_spawn_from_main(3)
   *
   */
#ifndef DOXYGEN_INPROGRESS
INLINE
#endif
void pip_spawn_from_func( pip_spawn_program_t *progp,
			  char *prog, char *funcname, void *arg, char **envv,
			  void *exp ) {
  memset( progp, 0, sizeof(pip_spawn_program_t) );
  progp->prog     = prog;
  progp->funcname = funcname;
  progp->arg      = arg;
  if( envv == NULL ) {
    extern char **environ;
    progp->envv = environ;
  } else {
    progp->envv = envv;
  }
  progp->exp = exp;
}

  /**
   * \page pip_spawn_hook pip_spawn_hook
   *
   * \brief Setting invocation hook information
   *
   * \synopsis
   * \#include <pip.h> \n
   * void pip_spawn_hook( pip_spawn_hook_t *hook,
   *                      pip_spawnhook_t before,
   *			  pip_spawnhook_t after,
   *			  void *hookarg );
   *
   * \description
   *
   * The \a before and \a after functions are introduced to follow the
   * programming model of the \c fork and \c exec.
   * \a before function does the prologue found between the
   * \c fork and \c exec. \a after function is to free the argument if
   * it is \c malloc()ed, for example.
   * \pre
   * It should be noted that the \a before and \a after
   * functions are called in the \e context of PiP root, although they
   * are running as a part of PiP task (i.e., having PID of the
   * spawning PiP task). Conversely
   * speaking, those functions cannot access the variables defined in
   * the spawning PiP task.
   * \pre
   * The before and after hook functions  should have
   * the function prototype as shown below;
     \code
     int hook_func( void *hookarg )
     \endcode
   *
   * \param[out] hook Pointer to the \c pip_spawn_hook_t
   *  structure in which the invocation hook information will be set
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
   * \note
   * Note that the file descriptors and signal
   * handlers are shared between PiP root and PiP tasks in the pthread
   * execution mode.
   *
   * \sa pip_task_spawn(3)
   *
   */
#ifndef DOXYGEN_INPROGRESS
INLINE
#endif
void pip_spawn_hook( pip_spawn_hook_t *hook,
		     pip_spawnhook_t before,
		     pip_spawnhook_t after,
		     void *hookarg ) {
  hook->before  = before;
  hook->after   = after;
  hook->hookarg = hookarg;
}

  /**
   * \page pip_task_spawn pip_task_spawn
   *
   * \brief Spawning a PiP task
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_task_spawn( pip_spawn_program_t *progp,
   * 		    uint32_t coreno,
   *		    uint32_t opts,
   *		    int *pipidp,
   *		    pip_spawn_hook_t *hookp );
   *
   * \description
   * This function spawns a PiP task specified by \c progp.
   * \par
   * In the process execution mode, the file descriptors having the
   * \c FD_CLOEXEC flag is closed and will not be passed to the spawned
   * PiP task. This simulated close-on-exec will not take place in the
   * pthread execution mode.
   *
   * \param[out] hook Pointer to the \p pip_spawn_hook_t
   *  structure in which the invocation hook information is set
   * \param[in] coreno CPU core number for the PiP task to be bound to. By
   *  default, \p coreno is set to zero, for example, then the calling
   *  task will be bound to the first core available. This is in mind
   *  that the available core numbers are not contiguous. To specify
   *  an absolute core number, \p coreno must be bitwise-ORed with
   *  \p PIP_CPUCORE_ABS. If
   *  \p PIP_CPUCORE_ASIS is specified, then the core binding will not
   *  take place.
   * \param[in] opts option flags
   * \param[in,out] pipidp Specify PiP ID of the spawned PiP task. If
   *  \p PIP_PIPID_ANY is specified, then the PiP ID of the spawned PiP
   *  task is up to the PiP library and the assigned PiP ID will be
   *  returned.
   * \param[in] hookp Hook information to be invoked before and after
   *  the program invokation.
   *
   * \return Zero is returned if this function succeeds. On error, an
   * error number is returned.
   * \retval EPERM PiP library is not yet initialized
   * \retval EPERM PiP task tries to spawn child task
   * \retval EINVAL \c progp is \c NULL
   * \retval EINVAL \c opts is invalid and/or unacceptable
   * \retval EINVAL the value off \c pipidp is invalid
   * \retval EINVAL the coreno is larger than or equal to \p
   * PIP_CPUCORE_CORENO_MAX
   * \retval EBUSY specified PiP ID is alredy occupied
   * \retval ENOMEM not enough memory
   * \retval ENXIO \c dlmopen failss
   *
   * \note
   * In the process execution mode, each PiP task may have its own
   * file descriptors, signal handlers, and so on, just like a
   * process. Contrastingly, in the pthread executionn mode, file
   * descriptors and signal handlers are shared among PiP root and PiP
   * tasks while maintaining the privatized variables.
   *
   * \environment
   * \arg \b PIP_STOP_ON_START Specifying the PIP ID to stop on start
   * to debug the specified PiP task from the beginning. If the
   * before hook is specified, then the PiP task will be stopped just
   * before calling the before hook.
   *
   * \bugs
   * In theory, there is no reason to restrict for a PiP task to
   * spawn another PiP task. However, the current glibc implementation
   * does not allow to do so.
   * \par
   * If the root process is multithreaded, only the main
   * thread can call this function.
   *
   * \sa pip_task_spawn(3), pip_spawn_from_main(3),
   * pip_spawn_from_func(3), pip_spawn_hook(3), pip_spawn(3), pip_blt_spawn(3)
   */
int pip_task_spawn( pip_spawn_program_t *progp,
		    uint32_t coreno,
		    uint32_t opts,
		    int *pipidp,
		    pip_spawn_hook_t *hookp );

  /**
   * \page pip_spawn pip_spawn
   *
   * \brief spawn a PiP task (PiP v1 API and deprecated)
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_spawn( char *filename, char **argv, char **envv,
   *		 uint32_t coreno, int *pipidp,
   *		 pip_spawnhook_t before, pip_spawnhook_t after, void *hookarg);
   *
   * \description
   * This function spawns a PiP task.
   * \par
   * In the process execution mode, the file descriptors having the
   * \c FD_CLOEXEC flag is closed and will not be passed to the spawned
   * PiP task. This simulated close-on-exec will not take place in the
   * pthread execution mode.
   *
   * \param[in] filename The executable to run as a PiP task
   * \param[in] argv Argument(s) for the spawned PiP task
   * \param[in] envv Environment variables for the spawned PiP task
   * \param[in] coreno CPU core number for the PiP task to be bound to. By
   *  default, \p coreno is set to zero, for example, then the calling
   *  task will be bound to the first core available. This is in mind
   *  that the available core numbers are not contiguous. To specify
   *  an absolute core number, \p coreno must be bitwise-ORed with
   *  \p PIP_CPUCORE_ABS. If
   *  \p PIP_CPUCORE_ASIS is specified, then the core binding will not
   *  take place.
   * \param[in,out] pipidp Specify PiP ID of the spawned PiP task. If
   *  \c PIP_PIPID_ANY is specified, then the PiP ID of the spawned PiP
   *  task is up to the PiP library and the assigned PiP ID will be
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
   * \retval EPERM PiP library is not yet initialized
   * \retval EPERM PiP task tries to spawn child task
   * \retval EINVAL \c progp is \c NULL
   * \retval EINVAL \c opts is invalid and/or unacceptable
   * \retval EINVAL the value off \c pipidp is invalid
   * \retval EINVAL the coreno is larger than or equal to \p
   * PIP_CPUCORE_CORENO_MAX
   * \retval EBUSY specified PiP ID is alredy occupied
   * \retval ENOMEM not enough memory
   * \retval ENXIO \c dlmopen failss
   *
   * \environment
   * \arg \b PIP_STOP_ON_START Specifying the PIP ID to stop on start
   * PiP task program to debug from the beginning. If the
   * before hook is specified, then the PiP task will be stopped just
   * before calling the before hook.
   *
   * \bugs
   * In theory, there is no reason to restrict for a PiP task to
   * spawn another PiP task. However, the current glibc implementation
   * does not allow to do so.
   * \par
   * If the root process is multithreaded, only the main
   * thread can call this function.
   *
   * \sa pip_task_spawn(3), pip_spawn_from_main(3),
   * pip_spawn_from_func(3), pip_spawn_hook(3), pip_task_spawn(3), pip_blt_spawn(3)
   */
  int pip_spawn( char *filename, char **argv, char **envv,
		 uint32_t coreno, int *pipidp,
		 pip_spawnhook_t before, pip_spawnhook_t after, void *hookarg);

  /**
   * @}
   */

  /**
   * \defgroup pip-4-export Export/Import Functions
   * @{
   * \page pip-export PiP Export and Import
   * \description Export and import functions to exchange addresses among tasks
   */

  /**
   * \page pip_named_export pip_named_export
   *
   * \brief export an address of the calling PiP root or a PiP task to
   * the others.
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_named_export( void *exp, const char *format, ... )
   *
   * \description
   * Pass an address of a memory region to the other PiP task. Unlike
   * the simmple \p pip_export and \p pip_import
   * functions which can only export one address per task,
   * \p pip_named_export and \p pip_named_import can associate a name
   * with an address so that PiP root or PiP task can exchange
   * arbitrary number of addressess.
   *
   * \param[in] exp an address to be passed to the other PiP task
   * \param[in] format a \c printf format to give the exported address
   * a name. If this is \p NULL, then the name is assumed to be "".
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM \p pip_init is not yet called.
   * \retval EBUSY The name is already registered.
   * \retval ENOMEM Not enough memory
   *
   * \note
   * The addresses exported by \p pip_named_export cannot be imported
   * by calling \p pip_import, and vice versa.
   *
   * \sa pip_named_import(3)
   */
  int pip_named_export( void *exp, const char *format, ... )
    __attribute__ ((format (printf, 2, 3)));

  /**
   * \page pip_named_import pip_named_import
   *
   * \brief import the named exported address
   *
   * \synopsis
   * \#include <pip.h> \n
   *  int pip_named_import( int pipid, void **expp, const char
   * *format, ... )
   *
   * \description
   * Import an address exported by the specified PiP task and having
   * the specified name. If it is not exported yet, the calling task
   * will be blocked. The
   *
   * \param[in] pipid The PiP ID to import the exposed address
   * \param[out] expp The starting address of the exposed region of
   *  the PiP task specified by the \a pipid.
   * \param[in] format a \c printf format to give the exported address a name
   *
   * \note
   * There is possibility of deadlock when two or more tasks are
   * mutually waiting for exported addresses.
   * \par
   * The addresses exported by \p pip_export cannot be imported
   * by calling \p pip_named_import, and vice versa.
   *
   * \return zero is returned if this function succeeds. On error, an
   * error number is returned.
   * \retval EPERM \p pip_init is not yet called.
   * \retval EINVAL The specified \p pipid is invalid
   * \retval ENOMEM Not enough memory
   * \retval ECANCELED The target task is terminated
   * \retval EDEADLK \p pipid is the calling task and tries to block
   * itself
   *
   * \sa pip_named_export(3), pip_named_tryimport(3), pip_export(3), pip_import(3)
   */
  int pip_named_import( int pipid, void **expp, const char *format, ... )
    __attribute__ ((format (printf, 3, 4)));

  /**
   * \page pip_named_tryimport pip_named_tryimport
   *
   * \brief import the named exported address (non-blocking)
   *
   * \synopsis
   * \#include <pip.h> \n
   *  int pip_named_tryimport( int pipid, void **expp, const char
   * *format, ... )
   *
   * \description
   * Import an address exported by the specified PiP task and having
   * the specified name. If it is not exported yet, this returns \p EAGAIN.
   *
   * \param[in] pipid The PiP ID to import the exposed address
   * \param[out] expp The starting address of the exposed region of
   *  the PiP task specified by the \a pipid.
   * \param[in] format a \c printf format to give the exported address a name
   *
   * \note
   * The addresses exported by \p pip_export cannot be imported
   * by calling \p pip_named_import, and vice versa.
   *
   * \return Zero is returned if this function succeeds. On error, an
   * error number is returned.
   * \retval EPERM \p pip_init is not yet called.
   * \retval EINVAL The specified \p pipid is invalid
   * \retval ENOMEM Not enough memory
   * \retval ECANCELED The target task is terminated
   * \retval EAGAIN Target is not exported yet
   *
   * \sa pip_named_export(3), pip_named_import(3), pip_export(3), pip_import(3)
   */
  int pip_named_tryimport( int pipid, void **expp, const char *format, ... )
    __attribute__ ((format (printf, 3, 4)));

  /**
   * \page pip_export pip_export
   *
   * \brief export an address
   *
   * \synopsis
   * \#include <pip.h> \n
   *  int pip_export( void *exp );
   *
   * \description
   * Pass an address of a memory region to the other PiP task. This is
   * a very naive implementation in PiP v1 and deprecated. Once a task
   * export an address, there is no way to change the exported address
   * or undo export.
   *
   * \param[in] exp An addresss
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM PiP library is not initialized yet
   *
   * \sa pip_import(3), pip_named_export(3), pip_named_import(3), pip_named_tryimport(3)
   */
  int pip_export( void *exp );

  /**
   * \page pip_import pip_import
   *
   * \brief import exported address of a PiP task
   *
   * \synopsis
   * \#include <pip.h> \n
   *  int pip_export( void **expp );
   *
   * \description
   * Get an address exported by the specified PiP task. This is
   * a very naive implementation in PiP v1 and deprecated. If the
   * address is not yet exported at the time of calling this function,
   * then \p NULL is returned.
   *
   * \param[in] pipid The PiP ID to import the exportedaddress
   * \param[out] expp The exported address
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM PiP library is not initialized yet
   *
   * \sa pip_export(3), pip_named_export(3), pip_named_import(3), pip_named_tryimport(3)
   */
  int pip_import( int pipid, void **expp );

  /**
   * @}
   */

  /**
   * \defgroup pip-3-wait Waiting for PiP task termination
   * @{
   * \page pip-wait Waiting for PiP task termination
   * \description Functions to wait for PiP task termination.
   * All functions listed here must only be called from PiP root.
   */

  /**
   * \page pip_wait pip_wait
   *
   * \brief wait for the termination of a PiP task
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_wait( int pipid, int *status );
   *
   * \description
   * This function can be used regardless to the PiP execution mode.
   * This function blocks until the specified PiP task terminates.
   * The
   * macros such as \p WIFEXITED and so on defined in Glibc can be
   * applied to the returned \p status value.
   *
   * \param[in] pipid PiP ID to wait for.
   * \param[out] status Status value of the terminated PiP task
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM PiP library is not initialized yet
   * \retval EPERM This function is called other than PiP root
   * \retval EDEADLK The specified \c pipid is the one of PiP root
   * \retval ECHILD The target PiP task does not exist or it was already
   * terminated and waited for
   *
   * \sa pip_exit(3), pip_trywait(3), pip_wait_any(3), pip_trywait_any(3)
   */
  int pip_wait( int pipid, int *status );

  /**
   * \page pip_trywait pip_trywait
   *
   * \brief wait for the termination of a PiP task in a non-blocking way
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_trywait( int pipid, int *status );
   *
   * \description
   * This function can be used regardless to the PiP execution mode.
   * This function behaves like the \p wait function of glibc and the
   * macros such as \p WIFEXITED and so on can be applied to the
   * returned \p status value.
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_trywait( int pipid, int *status );
   *
   * \param[in] pipid PiP ID to wait for.
   * \param[out] status Status value of the terminated PiP task
   *
   * \note This function can be used regardless to the PiP execution
   * mode.
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM The PiP library is not initialized yet
   * \retval EPERM This function is called other than PiP root
   * \retval EDEADLK The specified \c pipid is the one of PiP root
   * \retval ECHILD The target PiP task does not exist or it was already
   * terminated and waited for
   *
   * \sa pip_exit(3), pip_wait(3), pip_wait_any(3), pip_trywait_any(3)
   */
  int pip_trywait( int pipid, int *status );

  /**
   * \page pip_wait_any pip_wait_any
   *
   * \brief Wait for the termination of any PiP task
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_wait_any( int *pipid, int *status );
   *
   * \description
   * This function can be used regardless to the PiP execution mode.
   * This function blocks until any of PiP tasks terminates.
   * The macros such as \p WIFEXITED and so on defined in Glibc can be
   * applied to the returned \p status value.
   *
   * \param[out] pipid PiP ID of terminated PiP task.
   * \param[out] status Exit value of the terminated PiP task
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM The PiP library is not initialized yet
   * \retval EPERM This function is called other than PiP root
   * \retval ECHILD The target PiP task does not exist or it was already
   * terminated and waited for
   *
   * \sa pip_exit(3), pip_wait(3), pip_trywait(3), pip_trywait_any(3)
   *
   */
  int pip_wait_any( int *pipid, int *status );

  /**
   * \page pip_trywait_any pip_trywait_any
   *
   * \brief non-blocking version of \p pip_wait_any
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_trywait_any( int *pipid, int *status );
   *
   * \description
   * This function can be used regardless to the PiP execution mode.
   * This function blocks until any of PiP tasks terminates.
   * The macros such as \p WIFEXITED and so on defined in Glibc can be
   * applied to the returned \p status value.
   *
   * \param[out] pipid PiP ID of terminated PiP task.
   * \param[out] status Exit value of the terminated PiP task
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM The PiP library is not initialized yet
   * \retval EPERM This function is called other than PiP root
   * \retval ECHILD There is no PiP task to wait for
   *
   * \sa pip_exit(3), pip_wait(3), pip_trywait(3), pip_wait_any(3)
   */
  int pip_trywait_any( int *pipid, int *retval );

  /**
   * @}
   */

  /**
   * \defgroup pip-5-misc PiP Miscellaneous Functions
   * @{
   * \page pip-misc PiP miscellaneous functions
   * \description Miscellaneous functions for PiP task (not BLT/ULP)
   */

  /**
   * \page pip_get_pipid pip_get_pipid
   *
   * \brief get PiP ID of the calling task
   *
   * \synopsis
   * \#include <pip.h> \n
   *  int pip_get_pipid( int *pipidp );
   *
   * \param[out] pipidp This parameter points to the variable which
   *  will be set to the PiP ID of the calling task
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM PiP library is not initialized yet
   */
  int pip_get_pipid( int *pipidp );

  /**
   * \page pip_is_initialized pip_is_initialized
   *
   * \brief Query is PiP library is already initialized
   *
   * \synopsis
   * \#include <pip.h> \n
   *  int pip_is_initialized( void );
   *
   * \return Return a non-zero value if PiP is already
   * initialized. Otherwise this returns zero.
   */
  int pip_is_initialized( void );

  /**
   * \page pip_get_ntasks pip_get_ntasks
   *
   * \brief get the maximum number of the PiP tasks
   *
   * \synopsis
   * \#include <pip.h> \n
   *  int pip_get_ntasks( int *ntasksp );
   *
   * \param[out] ntasksp Maximum number of PiP tasks is returned
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM PiP library is not yet initialized
   *
   */
  int pip_get_ntasks( int *ntasksp );

  /**
   * \page pip_get_mode pip_get_mode
   *
   * \brief get the PiP execution mode
   *
   * \synopsis
   * \#include <pip.h> \n
   *  int pip_get_mode( int *modep );
   *
   * \param[out] modep Returned PiP execution mode
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM PiP library is not yet initialized
   *
   * \sa pip_get_mode_str(3)
   */
  int pip_get_mode( int *modep );

  /**
   * \page pip_get_mode_str pip_get_mode_str
   *
   * \brief get a character string of the current execution mode
   *
   * \synopsis
   * \#include <pip.h> \n
   *  char *pip_get_mode_str( void );
   *
   * \return Return the name string of the current execution mode. If
   * PiP library is note initialized yet, then thiss return \p NULL.
   *
   */
  const char *pip_get_mode_str( void );

  /**
   * \page pip_get_system_id pip_get_system_id
   *
   * \brief deliver a process or thread ID defined by the system
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_get_system_id( int *pipid, uintptr_t *idp );
   *
   * \description
   * The returned object depends on the PiP execution mode. In the process
   * mode it returns TID (Thread ID, not PID) and in the thread mode
   * it returns thread (\p pthread_t) associated with the PiP task
   * This function can be used regardless to the PiP execution
   * mode.
   *
   * \param[out] pipid PiP ID of a target PiP task
   * \param[out] idp a pointer to store the ID value
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM The PiP library is not initialized yet
   */
  int pip_get_system_id( int pipid, pip_id_t *idp );

  /**
   * \page pip_isa_root pip_isa_root
   *
   * \brief check if calling PiP task is a PiP root or not
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_isa_root( void );
   *
   * \return Return a non-zero value if the caller is the PiP
   * root. Otherwise this returns zero.
   */
  int  pip_isa_root( void );

  /**
   * \page pip_isa_task pip_isa_task
   *
   * \brief check if calling PiP task is a PiP task or not
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_isa_task( void );
   *
   * \return Return a non-zero value if the caller is the PiP
   * task. Otherwise this returns zero.
   */
  int  pip_isa_task( void );

  /**
   * \page pip_is_threaded pip_is_threaded
   *
   * \brief check if PiP execution mode is pthread or not
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_is_threaded( int *flagp );
   *
   * \param[out] set to a non-zero value if PiP execution mode is
   * Pthread
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM The PiP library is not initialized yet
   */
  int pip_is_threaded( int *flagp );

  /**
   * \page pip_is_shared_fd pip_is_shared_fd
   *
   * \brief check if file descriptors are shared or not.
   * This is equivalent with the \p pip_is_threaded function.
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_is_shared_fd( int *flagp );
   *
   * \param[out] set to a non-zero value if FDs are shared
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM The PiP library is not initialized yet
   */
  int pip_is_shared_fd( int *flagp );

  /**
   * @}
   */

  /**
   * \defgroup pip-2-exit Terminating PiP Task
   * @{
   * \page pip-exit Terminating PiP task
   * \description Function to ternminate PiP task normally or
   * abnormally (abort).
   */

  /**
   * \page pip_exit pip_exit
   *
   * \brief terminate the calling PiP task
   *
   * \synopsis
   * \#include <pip.h> \n
   *  void pip_exit( int status );
   *
   * \description
   * When the main function or the start function of a PiP task
   * returns with an integer value, then it has the same effect of
   * calling \p pip_exit with the returned value.
   *
   * \param[in] status This status is returned to PiP root.
   *
   * \note
   * This function can be used regardless to the PiP execution mode.
   * \p exit(3) is called in the process mode and \p pthread_exit(3)
   * is called in the pthread mode.
   *
   * \sa pip_wait(3), pip_trywait(3), pip_wait_any(3), pip_trywait_any(3)
   *
   */
  void pip_exit( int status );

  /**
   * \page pip_kill_all_tasks pip_kill_all_tasks
   *
   * \brief kill all PiP tasks
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_kill_all_tasks( void );
   *
   * \note
   * This function must be called from PiP root.
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM The PiP library is not initialized yet
   * \retval EPERM Not called from root
   */
  int pip_kill_all_tasks( void );

  /**
   * \page pip_abort pip_abort
   * \brief Kill all PiP tasks and then kill PiP root
   *
   * \synopsis
   * \#include <pip.h> \n
   * void pip_abort( void );
   */
  void pip_abort(void);

  /**
   * @}
   */

#ifndef DOXYGEN_INPROGRESS

  pid_t  pip_gettid( void );
  void   pip_glibc_lock( void );
  void   pip_glibc_unlock( void );
  void   pip_debug_info( void );
  size_t pip_idstr( char*, size_t );
  int    pip_check_pie( const char*, int );

#ifdef __cplusplus
}
#endif

#include <pip_blt.h>
#include <pip_dlfcn.h>
#include <pip_signal.h>

#endif /* DOXYGEN */

#endif	/* _pip_h_ */
