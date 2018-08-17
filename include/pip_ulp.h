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

#ifndef _pip_ulp_h_
#define _pip_ulp_h_

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <pip_machdep.h>

#define PIP_ULP_INIT(L)					\
  do { PIP_ULP_NEXT(L) = (L); PIP_ULP_PREV(L) = (L); } while(0)

#define PIP_ULP_NEXT(L)		((L)->next)
#define PIP_ULP_PREV(L)		((L)->prev)
#define PIP_ULP_PREV_NEXT(L)	((L)->prev->next)
#define PIP_ULP_NEXT_PREV(L)	((L)->next->prev)

#define PIP_ULP_ENQ_FIRST(L,E)				\
  do { PIP_ULP_NEXT(E)   = PIP_ULP_NEXT(L);		\
    PIP_ULP_PREV(E)      = (L);				\
    PIP_ULP_NEXT_PREV(L) = (E);				\
    PIP_ULP_NEXT(L)      = (E); } while(0)

#define PIP_ULP_ENQ_LAST(L,E)				\
  do { PIP_ULP_NEXT(E)   = (L);				\
    PIP_ULP_PREV(E)      = PIP_ULP_PREV(L);		\
    PIP_ULP_PREV_NEXT(L) = (E);				\
    PIP_ULP_PREV(L)      = (E); } while(0)

#define PIP_ULP_DEQ(L)					\
  do { PIP_ULP_NEXT_PREV(L) = PIP_ULP_PREV(L);		\
    PIP_ULP_PREV_NEXT(L) = PIP_ULP_NEXT(L); 		\
    PIP_ULP_INIT(L); } while(0)

#define PIP_ULP_MOVE_QUEUE(P,Q)				\
  do { if( !PIP_ULP_ISEMPTY(Q) ) {			\
    PIP_ULP_NEXT_PREV(Q) = (P);				\
    PIP_ULP_PREV_NEXT(Q) = (P);				\
    PIP_ULP_NEXT(P) = PIP_ULP_NEXT(Q);			\
    PIP_ULP_PREV(P) = PIP_ULP_PREV(Q);			\
    PIP_ULP_INIT(Q); } } while(0)

#define PIP_ULP_ISEMPTY(L)				\
  ( PIP_ULP_NEXT(L) == (L) && PIP_ULP_PREV(L) == (L) )

#define PIP_ULP_FOREACH(L,E)				\
  for( (E)=(L)->next; (L)!=(E); (E)=PIP_ULP_NEXT(E) )

#define PIP_ULP_FOREACH_SAFE(L,E,TV)				\
  for( (E)=(L)->next, (TV)=PIP_ULP_NEXT(E);			\
       (L)!=(E);						\
       (E)=(TV), (TV)=PIP_ULP_NEXT(TV) )

#define PIP_ULP_FOREACH_SAFE_XXX(L,E,TV)			\
  for( (E)=(L), (TV)=PIP_ULP_NEXT(E); (L)!=(E); (E)=(TV) )

#define PIP_LIST_INIT(L)	PIP_ULP_INIT(L)
#define PIP_LIST_ISEMPTY(L)	PIP_ULP_ISEMPTY(L)
#define PIP_LIST_ADD(L,E)	PIP_ULP_ENQ_LAST(L,E)
#define PIP_LIST_DEL(E)		PIP_ULP_DEQ(E)
#define PIP_LIST_MOVE(P,Q)	PIP_ULP_MOVE_QUEUE(P,Q)
#define PIP_LIST_FOREACH(L,E)	PIP_ULP_FOREACH(L,E)
#define PIP_LIST_FOREACH_SAFE(L,E,F)	PIP_ULP_FOREACH_SAFE(L,E,F)

typedef struct pip_locked_queue {
  union {
    struct {
      pip_spinlock_t	lock;
      pip_ulp_queue_t	queue;
    };
    char		__gap0__[PIP_CACHEBLK_SZ];
  };
} pip_locked_queue_t;

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
 * @addtogroup libpip_ulp libpip_ulp
 * \brief the PiP/ULP library
 * @{
 */

  /**
   * \brief sleep the curren PiP task and enqueue it as a ULP
   *  @{
   * \param[in] queue A locked queue
   * \param[in] flags Enqueue policy
   *
   * \return Return true if the specified PiP task or ULP is alive
   * (i.e., not yet terminated) and running
   */
  int pip_sleep_and_enqueue( pip_locked_queue_t *queue, int flags );
  /** @}*/

  /**
   * \brief dequeue a ULP from the queue and wakeup it as a PiP task
   *  @{
   * \param[in] queue A locked queue
   *
   * \return Return true if the specified PiP task or ULP is alive
   * (i.e., not yet terminated) and running
   */
  int pip_dequeue_and_wakeup( pip_locked_queue_t *queue );
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
   * \retval EDEADLK There is no other ULP eligible to run
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
   * \return Return 0 on success. Return an error code on error.
   * \retval EINAVL \c ulp is \c NULL
   * \retval EBUSY The specified ULP is already eligible to run
   *
   */
  int pip_ulp_resume( pip_ulp_t *ulp, int flags );
  /** @}*/

  /**
   * \brief Yield to the specified PiP ULP
   *  @{
   * \param[in] ulp Target PiP ULP to be scheduled
   *
   * Context-switch to the specified ULP. If \c ulp is \c NULL, then
   * this works the same as \c pip_ulp_yield() does.
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM The specified ULP is being scheduled by the other
   * PiP task
   *
   */
  int pip_ulp_yield_to( pip_ulp_t *ulp );
  /** @}*/

  /**
   * \brief Get the current PiP ULP
   *  @{
   * \param[out] ulpp The current PiP ULP
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EINAVL \c ulpp is \c NULL
   * \retval EPERM PiP library is not yet initialized
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
   * \return Return 0 on success. Return an error code on error.
   * \retval EINAVL Invalid PiP ID is specified
   * \retval EPERM PiP library is not yet initialized
   */
  int pip_ulp_get( int pipid, pip_ulp_t **ulpp );
  /** @}*/

  /**
   * \brief Get the PiP ID of the specified PiP ULP
   *  @{
   * \param[in] ulp PiP ULP
   * \param[out] pipidp PiP ID pointer
   *
   * \note If \c ulp is \c NULL, then the PiP id of the current ULP is
   * returned.
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM PiP library is not yet initialized
   */
  int pip_ulp_get_pipid( pip_ulp_t *ulp, int *pipidp );
  /** @}*/

  /**
   * \brief Get the PiP ID of the scheduling PiP task of the current ULP
   *  @{
   * \param[out] pipidp PiP ID pointer of the scheduling task
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EINAVL \c pipidp is \c NULL
   * \retval The target is not eligible to run
   */
  int pip_ulp_get_sched_task( int *pipidp );
  /** @}*/

  /**
   * \brief Initialize PiP ULP mutex
   *  @{
   * \param[in,out] mutex pointer to the PiP ULP mutex
   *
   * \note This PiP ULP mutex can only be used to lock ULPs and a PiP
   * task having the same scheduling domain.
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EINAVL \c mutex is \c NULL
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
   * \return Return 0 on success. Return an error code on error.
   * \retval EINAVL \c mutex is \c NULL
   * \retval EDEADLK Tries to lock again
   * \retval EPERM The lock is owned by the other PiP task
   * (i.e. different scheduling domain)
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
   * \return Return 0 on success. Return an error code on error.
   * \retval EINAVL \c mutex is \c NULL
   * \retval EPERM The lock is owned by the other PiP task
   * (i.e. different scheduling domain)
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
   * \return Return 0 on success. Return an error code on error.
   * \retval EINAVL \c barrier is \c NULL or \c n is invalid
   *
   * \sa pip_ulp_barrier_wait(3), pip_task_barrier_init(3),
   * pip_task_barrier_wait(3), pip_universal_barrier_init(3),
   * pip_universal_barrier_wait(3)
   */
  int pip_ulp_barrier_init( pip_ulp_barrier_t *barrp, int n );
  /** @}*/

  /**
   * \brief Wait on a PiP ULP barrier
   *  @{
   * \param[in] barrp pointer to the PiP ULP barrier
   *
   * \note This PiP ULP barrier can only be used to synchronize ULPs
   * and a PiP task having the same scheduling domain.
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EINAVL \c barrier is \c NULL
   * \retval EPERM The barrier has different scheduling domain
   * \retval EDEADLK There is no other PiP task or ULP eligible to run
   *
   * \sa pip_ulp_barrier_init(3), pip_task_barrier_init(3),
   * pip_task_barrier_wait(3), pip_universal_barrier_init(3),
   * pip_universal_barrier_wait(3)
   */
  int pip_ulp_barrier_wait( pip_ulp_barrier_t *barrp );
  /** @}*/

  /**
   * \brief Initialize alocked ULP queue
   *  @{
   * \param[in] queue pointer to the locked queue to be initialized
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EINAVL \c queue is \c NULL
   *
   * \sa pip_ulp_suspend_and_enqueue(3),
   * pip_ulp_dequeue_and_involve(3), pip_ulp_enqueue_with_lock(3),
   * pip_ulp_dequeue_with_lock(3)
   */
  int pip_locked_queue_init( pip_locked_queue_t *queue );
  /** @}*/

  /**
   * \brief Suspend the current PiP ULP and enqueue it to the
   *  specified locked queue for possible migration
   *  @{
   * \param[in] queue pointer to a locked ULP queue
   * \param[in] flag Specifying scheduling policy
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EINAVL \c queue is \c NULL
   * \retval EPERM a PiP task cannot be involved
   *
   * \sa pip_locked_queue_init(3),
   * pip_ulp_dequeue_and_involve(3), pip_ulp_enqueue_with_lock(3),
   * pip_ulp_dequeue_with_lock(3)
   */
  int pip_ulp_suspend_and_enqueue( pip_locked_queue_t *queue,
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
   * \return Return 0 on success. Return an error code on error.
   * \retval EINAVL \c queue is \c NULL
   * \retval ENOENT The specified queue is empty
   * \retval EPERM The ULP in the queue to be scheduled is a PiP task
   *
   * \sa pip_locked_queue_init(3),
   * pip_ulp_suspend_and_enqueue(3), pip_ulp_enqueue_with_lock(3),
   * pip_ulp_dequeue_with_lock(3)
   */
  int pip_ulp_dequeue_and_involve( pip_locked_queue_t *queue,
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
   * \return Return 0 on success. Return an error code on error.
   * \retval EINAVL \c queue is \c NULL
   *
   * \sa pip_locked_queue_init(3), pip_ulp_dequeue_and_involve(3),
   * pip_ulp_suspend_and_enqueue(3),
   * pip_ulp_dequeue_with_lock(3)
   */
  int pip_enqueue_with_lock( pip_locked_queue_t *queue,
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
   * \return Return 0 on success. Return an error code on error.
   * \retval EINAVL \c queue is \c NULL
   *
   * \sa pip_locked_queue_init(3), pip_ulp_dequeue_and_involve(3),
   * pip_ulp_suspend_and_enqueue(3), pip_ulp_enqueue_with_lock(3)
   */
  int pip_dequeue_with_lock( pip_locked_queue_t *queue,
			     pip_ulp_t **ulpp );
  /** @}*/

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _pip_ulp_h_ */
