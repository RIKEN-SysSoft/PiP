/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2016, 2017
*/

#ifndef _pip_ulp_h_
#define _pip_ulp_h_

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <pip_machdep.h>

typedef struct pip_ulp_locked_queue {
  pip_spinlock_t	lock;
  pip_ulp_queue_t	queue;
} pip_ulp_locked_queue_t;

typedef struct pip_ulp_mutex {
  void			*sched;
  void			*holder;
  pip_ulp_queue_t	waiting;
} pip_ulp_mutex_t;

typedef struct pip_ulp_barrier {
  void			*sched;
  pip_ulp_t		waiting;
  int			count_init;
  int			count;
} pip_ulp_barrier_t;

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#ifdef __cplusplus
extern "C" {
#endif

  /**
   * \brief Create a PiP ULP
   *  @{
   * \param[in] progp Pointer to the \t pip_spawn_program_t
   * \param[in,out] pipidp Specify PIPID of the new ULP. If
   *  \c PIP_PIPID_ANY is specified, then the PIPID of the new ULP
   *  is up to the PiP library and the assigned PIPID will be
   *  returned.
   * \param[in,out] sisters List of ULPs to be scheduled by the same
   *  PiP task. Or, \t NULL to specify no list.
   * \param[out} newp Created ULP is returned.
   *
   *
   * \sa pip_task_spawn(3), pip_spawn_from_main(3)
   */
  int pip_ulp_new( pip_spawn_program_t *progp,
		   int *pipidp,
		   pip_ulp_queue_t *sisters,
		   pip_ulp_t **newp );
  /** @}*/

  /**
   * \brief Yield
   *  @{
   *
   * \return Return 0 on success. Return an error code on error.
   *
   * \sa pip_ulp_suspend(3), pip_ulp_resume(3)
   */
  int pip_ulp_yield( void );
  /** @}*/

  /**
   * \brief Suspend the current ULP and schedule the next ULP eligible
   *  to run.
   *  @{
   *
   * The suspended ULP can be eligible to run when \b pip_ulp_resume()
   * is called. If there is no ULPs eligible to run as the result of
   * calling this function, it returns \a EDEADLK.
   *
   * \return Return 0 on success. Return an error code on error.
   *
   * \sa pip_ulp_resume(3), pip_ulp_yield(3)
   */
  int pip_ulp_suspend( void );
  /** @}*/

  /**
   * \brief
   *  @{
   *
   */
  int pip_ulp_resume( pip_ulp_t *ulp, int flags );
  /** @}*/

  /**
   * \brief
   *  @{
   *
   */
  int pip_ulp_yield_to( pip_ulp_t *ulp );
  /** @}*/

  /**
   * \brief
   *  @{
   *
   */
  int pip_ulp_myself( pip_ulp_t **ulpp );
  /** @}*/

  /**
   * \brief
   *  @{
   *
   */
  int pip_ulp_get( int pipid, pip_ulp_t **ulpp );
  /** @}*/

  /**
   * \brief
   *  @{
   *
   */
  int pip_ulp_get_pipid( pip_ulp_t *ulp, int *pipidp );
  /** @}*/

  /**
   * \brief
   *  @{
   *
   */
  int pip_get_ulp_sched( int *pipidp );
  /** @}*/

  /**
   * \brief
   *  @{
   *
   */
  int pip_ulp_mutex_init( pip_ulp_mutex_t *mutex );
  /** @}*/

  /**
   * \brief
   *  @{
   *
   */
  int pip_ulp_mutex_lock( pip_ulp_mutex_t *mutex );
  /** @}*/

  /**
   * \brief
   *  @{
   *
   */
  int pip_ulp_mutex_unlock( pip_ulp_mutex_t *mutex );
  /** @}*/

  /**
   * \brief
   *  @{
   *
   */
  void pip_ulp_barrier_init( pip_ulp_barrier_t *barrp, int n );
  /** @}*/

  /**
   * \brief
   *  @{
   *
   */
  int pip_ulp_barrier_wait( pip_ulp_barrier_t *barrp );
  /** @}*/

  int pip_ulp_locked_queue_init(   pip_ulp_locked_queue_t *queue );
  int pip_ulp_enqueue_and_suspend( pip_ulp_locked_queue_t *queue,
				   int flags );
  int pip_ulp_dequeue_and_migrate( pip_ulp_locked_queue_t *queue,
				   pip_ulp_t **ulpp,
				   int flag );
  int pip_ulp_enqueue_locked_queue( pip_ulp_locked_queue_t *queue,
				    pip_ulp_t *ulp,
				    int flags );
  int pip_ulp_dequeue_locked_queue( pip_ulp_locked_queue_t *queue,
				    pip_ulp_t **ulpp );

#ifdef __cplusplus
}
#endif

#endif /* _pip_ulp_h_ */
