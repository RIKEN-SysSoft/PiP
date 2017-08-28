/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2016, 2017
*/

#ifndef _pip_h_
#define _pip_h_

/** \mainpage pip Overview of Process-in-Process (PiP)
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

#define PIP_OPT_MASK			(0XFF)
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
#define PIP_PIPID_ANY		(-2)
#define PIP_PIPID_MYSELF	(-3)

#define PIP_NTASKS_MAX		(260)

#define PIP_CPUCORE_ASIS 	(-1)

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

typedef int  (*pip_spawnhook_t)      ( void* );

typedef struct pip_barrier {
  int			count_init;
  volatile uint32_t	count;
  volatile int		gsense;
} pip_barrier_t;

#define PIP_BARRIER_INIT(N)	{(N),(N),0}

#ifdef __cplusplus
extern "C" {
#endif

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

/**
 * @addtogroup libpip libpip
 * \brief the PiP library
 * @{
 * @file
 * @{
 */

  /**
   *
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
   * \return Return 0 on success. Return an error code on error.
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
   * \return Return 0 on success. Return an error code on error.
   *
   * This function finalize the PiP library.
   *
   * \sa pip_init(3)
   */
  int pip_fin( void );
  /** @}*/

  /**
   * \brief spawn a PiP task
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
   * \brief terminate PiP task
   *  @{
   * \param[in] retval Terminate PiP task with the exit number
   * specified with this parameter.
   *
   * \note This function can be used regardless to the PiP execution
   * mode.
   *
   * \return Return 0 on success. Return an error code on error.
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
   * \note This function never returns when it succeeds.
   * \note This function can be used regardless to the PiP execution
   * mode. However, only the least significant 2 bytes are
   * effective. This is because of the compatibility with the
   * \c exit glibc function.
   *
   * \return Return 0 on success. Return an error code on error.
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
   */
  int pip_trywait( int pipid, int *retval );
  /** @}*/

  /**
   * \brief deliver a signal to a PiP task
   *  @{
   * \param[out] pipid PIPID of a target PiP task
   * \param[out] signal signal number to be delivered
   *
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
   * \param[in] n number of participants of this barrier synchronization
   *
   */
  void pip_barrier_init( pip_barrier_t *barrp, int n );
  /** @}*/

  /**
   * \brief wait on barrier synchronization in a busy-wait way
   *  @{
   *
   * \param[in] barrp pointer to a PiP barrier structure
   *
   * \note This barrier synchronization never blocks.
   *
   */
  void pip_barrier_wait( pip_barrier_t *barrp );
  /** @}*/

#ifndef DOXYGEN_SHOULD_SKIP_THIS

  int  pip_idstr( char *buf, size_t sz );

#ifdef __cplusplus
}
#endif

#ifdef PIP_INTERNAL_FUNCS
#include <pip_internal.h>
#endif /* PIP_INTERNAL_FUNCS */

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

/**
 * @}
 * @}
 */

#endif	/* _pip_h_ */
