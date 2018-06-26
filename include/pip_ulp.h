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
  pip_ulp_queue_t	waiting;
  int			count_init;
  int			count;
} pip_ulp_barrier_t;

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup libpip libpip
 * \brief the PiP library
 * @{
 * @file
 * @{
 */

  /**
   * \brief Create a PiP ULP
   *  @{
   * \param[in] progp Pointer to the \c pip_spawn_program_t
   * \param[in,out] pipidp Specify PIPID of the new ULP. If
   *  \c PIP_PIPID_ANY is specified, then the PIPID of the new ULP
   *  is up to the PiP library and the assigned PIPID will be
   *  returned.
   * \param[in,out] sisters List of ULPs to be scheduled by the same
   *  PiP task. Or, \c NULL to specify no list.
   * \param[out] newp Created ULP is returned.
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
   * \brief Resume PiP ULP to be eligible to run
   *  @{
   * \param[in] ulp ULP to resume
   * \param[in] flags Specifying scheduling policy
   *
   */
  int pip_ulp_resume( pip_ulp_t *ulp, int flags );
  /** @}*/

  /**
   * \brief Yield to the specified PiP ULP
   *  @{
   * \param[in] ulp Target PiP ULP to be scheduled
   *
   */
  int pip_ulp_yield_to( pip_ulp_t *ulp );
  /** @}*/

  /**
   * \brief Get the current PiP ULP
   *  @{
   * \param[out] ulpp The current PiP ULP
   *
   */
  int pip_ulp_myself( pip_ulp_t **ulpp );
  /** @}*/

  /**
   * \brief Get the PiP ULP having the specified PiP ID
   *  @{
   * \param[in] pipid PiP ID
   * \param[out] ulpp Returned PiP ULP
   *
   */
  int pip_ulp_get( int pipid, pip_ulp_t **ulpp );
  /** @}*/

  /**
   * \brief Get the PiP ID of the specified PiP ULP
   *  @{
   * \param[in] ulp PiP ULP
   * \param[out] pipidp PiP ID pointer
   *
   */
  int pip_ulp_get_pipid( pip_ulp_t *ulp, int *pipidp );
  /** @}*/

  /**
   * \brief Get the PiP ID of the scheduling PiP task of the current ULP
   *  @{
   * \param[out] pipidp PiP ID pointer of the scheduling task
   *
   */
  int pip_get_ulp_sched( int *pipidp );
  /** @}*/

  /**
   * \brief Initialize PiP ULP mutex
   *  @{
   * \param[in,out] mutex pointer to the PiP ULP mutex
   *
   * \note This PiP ULP mutex can only be used to lock ULPs and a PiP
   * task having the same scheduling domain.
   *
   * \sa pip_ulp_mutex_lock(3), pip_ulp_mutex_unlock(3)
   */
  int pip_ulp_mutex_init( pip_ulp_mutex_t *mutex );
  /** @}*/

  /**
   * \brief Lock PiP ULP mutex
   *  @{
   * \param[in] mutex pointer to the PiP ULP mutex
   *
   * \note This PiP ULP mutex can only be used to lock ULPs and a PiP
   * task having the same scheduling domain.
   *
   * \sa pip_ulp_mutex_init(3), pip_ulp_mutex_unlock(3)
   */
  int pip_ulp_mutex_lock( pip_ulp_mutex_t *mutex );
  /** @}*/

  /**
   * \brief Unlock PiP ULP mutex
   *  @{
   * \param[in] mutex pointer to the PiP ULP mutex
   *
   * \note This PiP ULP mutex can only be used to lock ULPs and a PiP
   * task having the same scheduling domain.
   *
   * \sa pip_ulp_mutex_init(3), pip_ulp_mutex_lock(3)
   */
  int pip_ulp_mutex_unlock( pip_ulp_mutex_t *mutex );
  /** @}*/

  /**
   * \brief Initialize PiP ULP barrier
   *  @{
   * \param[in] barrp pointer to the PiP ULP barrier
   * \param[in] n Number of participants of the barrier
   *
   * \note This PiP ULP barrier can only be used to synchronize ULPs
   * and a PiP task having the same scheduling domain.
   *
   * \sa pip_ulp_mutex_wait(3)
   */
  void pip_ulp_barrier_init( pip_ulp_barrier_t *barrp, int n );
  /** @}*/

  /**
   * \brief Wait on a PiP ULP barrier
   *  @{
   * \param[in] barrp pointer to the PiP ULP barrier
   *
   * \note This PiP ULP barrier can only be used to synchronize ULPs
   * and a PiP task having the same scheduling domain.
   *
   * \sa pip_ulp_mutex_init(3)
   */
  int pip_ulp_barrier_wait( pip_ulp_barrier_t *barrp );
  /** @}*/

  /**
   * \brief Initialize alocked ULP queue
   *  @{
   * \param[in] queue pointer to the locked queue to be initialized
   *
   * \sa pip_ulp_suspend_and_enqueue(3),
   * pip_ulp_dequeue_and_migrate(3), pip_ulp_enqueue_with_lock(3),
   * pip_ulp_dequeue_with_lock(3)
   */
  int pip_ulp_locked_queue_init( pip_ulp_locked_queue_t *queue );
  /** @}*/

  /**
   * \brief Suspend the current PiP ULP and enqueue it to the
   *  specified locked queue
   *  @{
   * \param[in] queue pointer to a locked ULP queue
   * \param[in] flag Specifying scheduling policy
   *
   * \sa pip_ulp_locked_queue_init(3),
   * pip_ulp_dequeue_and_migrate(3), pip_ulp_enqueue_with_lock(3),
   * pip_ulp_dequeue_with_lock(3)
   */
  int pip_ulp_suspend_and_enqueue( pip_ulp_locked_queue_t *queue,
				   int flag );
  /** @}*/

  /**
   * \brief Dequeue a PiP ULP from the locked queue and make it
   *  eligible to run
   *  @{
   * \param[in] queue pointer to a locked ULP queue
   * \param[out] ulpp Dequeued PiP ULP
   * \param[in] flag Specifying scheduling policy
   *
   * \sa pip_ulp_locked_queue_init(3),
   * pip_ulp_suspend_and_enqueue(3), pip_ulp_enqueue_with_lock(3),
   * pip_ulp_dequeue_with_lock(3)
   */
  int pip_ulp_dequeue_and_migrate( pip_ulp_locked_queue_t *queue,
				   pip_ulp_t **ulpp,
				   int flag );
  /** @}*/

  /**
   * \brief Put a PiP ULP into the specified locked queue
   *  @{
   * \param[in] queue pointer to a locked ULP queue
   * \param[out] ulp PiP ULP to be enqueued
   * \param[in] flag Specifying scheduling policy
   *
   * \sa pip_ulp_locked_queue_init(3), pip_ulp_dequeue_and_migrate(3),
   * pip_ulp_suspend_and_enqueue(3),
   * pip_ulp_dequeue_with_lock(3)
   */
  int pip_ulp_enqueue_with_lock( pip_ulp_locked_queue_t *queue,
				 pip_ulp_t *ulp,
				 int flag );
  /** @}*/

  /**
   * \brief Dequeue the first PiP ULP in the locked queue
   *  @{
   * \param[in] queue pointer to a locked ULP queue
   * \param[out] ulpp pointer to the dequeued PiP ULP
   *
   * \note This PiP ULP barrier can only be used to synchronize ULPs
   * and a PiP task having the same scheduling domain.
   *
   * \sa pip_ulp_locked_queue_init(3), pip_ulp_dequeue_and_migrate(3),
   * pip_ulp_suspend_and_enqueue(3), pip_ulp_enqueue_with_lock(3)
   */
  int pip_ulp_dequeue_with_lock( pip_ulp_locked_queue_t *queue,
				 pip_ulp_t **ulpp );
  /** @}*/

#ifdef __cplusplus
}
#endif

/**
 * @}
 * @}
 */

#endif /* _pip_ulp_h_ */
