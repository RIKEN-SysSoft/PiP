/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2016
*/

#ifndef _pip_h_
#define _pip_h_

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#define PIP_OPTS_ANY			(0x0)
#define PIP_MODEL_PTHREAD		(0x100)
#define PIP_MODEL_PROCESS		(0x200)
/* the following two modes are a submode of PIP_MODEL_PROCESS */
#define PIP_MODEL_PROCESS_PRELOAD	(0x210)
#define PIP_MODEL_PROCESS_PIPCLONE	(0x220)

#define PIP_ENV_MODEL			"PIP_MODEL"
#define PIP_ENV_MODEL_THREAD		"thread"
#define PIP_ENV_MODEL_PTHREAD		"pthread"
#define PIP_ENV_MODEL_PROCESS		"process"
#define PIP_ENV_MODEL_PROCESS_PRELOAD	"process:preload"
#define PIP_ENV_MODEL_PROCESS_PIPCLONE	"process:pipclone"

#define PIP_ENV_STACKSZ		"PIP_STACKSZ"

#define PIP_PIPID_ROOT		(-1)
#define PIP_PIPID_ANY		(-2)
#define PIP_PIPID_MYSELF	(-3)

#define PIP_NTASKS_MAX		(100)

#define PIP_CPUCORE_ASIS 	(-1)

typedef int(*pip_spawnhook_t)(void*);

#include <sys/types.h>
#include <pthread.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#endif

  /** \defgroup pip_init PIP_INIT pip_init
   * Initialize the PiP library.
   *  @{
   * \param[out] pipidp When this is called by the PiP root
   *  process, then this returns PIP_PIPID_ROOT, otherwise it returns
   *  the PIPID of the calling PiP task.
   * \param[in,out] ntasks When called by the PiP root, it specifies
   *  the maxmum number of PiP tasks. When called by a PiP task, then
   *  it returns the number specified by the PiP root.
   * \param[in,out] root_expp If the root PiP is ready to export a
   *  memory region to any PiP task(s), then this parameter points to
   *  the variable holding the exporting address of the root PiP. If
   *  the PiP root is not ready to export or has nothing to export
   *  then this variable can be NULL. When called by a PiP task, it
   *  returns the exporting address of the PiP root.
   * \param[in] opts This must be zero at this point of time.
   *
   * \return Return 0 on success. Return an error code on error.
   *
   * This function initialize the PiP library. The PiP root process
   * must call this. It is optional for a PiP task to call this.
   *
   * Is is NOT guaranteed that users can spawn tasks up to the number
   * specified by the \a ntasks argument. There are some limitations
   * come from outside of the PiP library.
   *
   * \sa pip_export, pip_fin
   */
  int pip_init( int *pipidp, int *ntasks, void **root_expp, int opts );
  /** @}*/

  /** \defgroup pip_fin PIP_FIN pip_fin
   * finalize the PiP library.
   *  @{
   * \return Return 0 on success. Return an error code on error.
   *
   * This function finalize the PiP library.
   *
   * \sa pip_init
   */
  int pip_fin( void );
  /** @}*/

  /** \defgroup pip_spawn PIP_SPAWN pip_spawn
   * spawn a PiP task
   *  @{
   * \param[in] filename The executable to run as a PiP task
   * \param[in] argv Argument(s) for the spawned PiP task
   * \param[in] envv Environment variables for the spawned PiP task
   * \param[in] coreno Core number for the PiP task to be bound to. If
   *  PIP_CPUCORE_ASIS is specified, then the core binding will not
   *  take place.
   * \param[in,out] pipidp Specify PIPID of the spanwed PiP task. If
   *  PIP_PIPID_ANY is specified, then the PIPID of the spawned PiP
   *  task is up to the PiP library and the assigned PIPID will be
   *  returned.
   * \param[in] before Just before the executing of the spanwed PiP
   *  task, this function is called so that file descriptors inherited
   *  from the PiP root, for example, can deal with. This is only
   *  effective with the PiP process model. This function is called
   *  with the argument \a hookarg described below.
   * \param[in] after This function is called when the PiP task
   *  terminates for the cleanup puropose. This function is called
   *  with the argument \a hookarg described below.
   * \param[in] hookarg The argument for the \a before and \a after
   *  function call.
   *
   * \return Return 0 on success. Return an error code on error.
   *
   * This function behave like the Linux \c execve() function to spanw
   * a PiP task. These functions are introduced to follow the
   * programming style of conventional \c fork and \c
   * exec. \a before function does the prologue found between the
   * \c fork and \c exec. \a after function is to free the argument if
   * it is \c malloc()ed. Note that the \a before and \a after
   * functions are called in the different \e context from the spawned
   * PiP task. More specifically, the variables defined in the spawned
   * PiP task cannot be accessible from the \a before and \a after
   * functions.
   *
   * \bug In the current implementation, the spawned PiP task cannot
   * be freed even if the spawned PiP task terminates. To fix this,
   * hack on GLIBC (ld-linud.so) is required.
   */
  int pip_spawn( char *filename, char **argv, char **envv,
		 int coreno, int *pipidp,
		 pip_spawnhook_t before, pip_spawnhook_t after, void *hookarg);
  /** @}*/

  /** \defgroup pip_export PIP_EXPORT pip_export
   * export a memory region of the calling PiP root or a PiP task to
   * the others.
   *  @{
   * \param[in] exp Starting address of a memory region of the calling
   *  process or task to the others.
   *  function call.
   *
   * \return Return 0 on success. Return an error code on error.
   *
   * The PiP root or a PiP task can call this function only once. If
   * the PiP task calls the \c pip_init() with the non-null value of
   * the \a expp parameter, then the function call by the root PiP
   * will fail.
   *
   * \bug In theory, there is no reason to restrict for a PiP task to
   * spawn another PiP task. However, the current implementation fails
   * to do so.
   *
   * \sa pip_import
   */
  int pip_export( void *exp );
  /** @}*/

  /** \defgroup pip_import PIP_IMPORT pip_import
   * import the exposed memory region of the other.
   *  @{
   * \param[in] pipid The PIPID to import the exposed address
   * \param[out] expp The starting address of the exposed region of
   *  the PiP task specified by the \a pipid.
   *
   * \return Return 0 on success. Return an error code on error.
   *
   * \note There is no size parameter to specify the length of the
   * exported region because there is no way to restrict the access
   * outside of the exported region.
   *
   * \sa pip_export
   */
  int pip_import( int pipid, void **expp );
  /** @}*/

  /** \defgroup pip_get_pipid PIP_GET_PIPID pip_get_pipid
   * get PIPID
   *  @{
   * \param[out] pipidp This parameter points to the variable which
   *  will be set to the PIPID of the calling process.
   */
  int pip_get_pipid( int *pipid );
  /** @}*/

  /** \defgroup pip_get_ntasks PIP_GET_NTASKS pip_get_ntasks
   * get the maxmum number of the PiP tasks
   *  @{
   * \param[out] ntaskp This parameter points to the variable which
   *  will be set to the maxmum number of the PiP tasks.
   */
  int pip_get_ntasks( int *ntasksp );
  /** @}*/

#ifndef DOXYGEN_SHOULD_SKIP_THIS

  int pip_exit( int  retval );
  int pip_wait( int pipid, int *retval );
  int pip_trywait( int pipid, int *retval );
  int pip_kill( int pipid, int signal );

#ifdef PIP_INTERNAL_FUNCS

  /* the following functions depends its implementation deeply */

  int pip_get_thread( int pipid, pthread_t *threadp );
  int pip_if_pthread( int *flagp );
  int pip_if_shared_fd( int *flagp );
  int pip_if_shared_sighand( int *flagp );

  int pip_join( int pipid );
  int pip_tryjoin( int pipid );
  int pip_timedjoin( int pipid, struct timespec *time );

  int pip_get_pid( int pipid, pid_t *pidp );

  int pip_print_loaded_solibs( FILE *file );
  char **pip_copy_vec( char *addition, char **vecsrc );

#endif

#ifdef __cplusplus
}
#endif

#endif

#endif
