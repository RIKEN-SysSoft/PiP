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

#define PIP_TASKQ_INIT(L)					\
  do { PIP_TASKQ_NEXT(L) = (L); PIP_TASKQ_PREV(L) = (L); } while(0)

#define PIP_TASKQ_NEXT(L)	((L)->next)
#define PIP_TASKQ_PREV(L)	((L)->prev)
#define PIP_TASKQ_PREV_NEXT(L)	((L)->prev->next)
#define PIP_TASKQ_NEXT_PREV(L)	((L)->next->prev)

#define PIP_TASKQ_ENQ_FIRST(L,E)				\
  do { PIP_TASKQ_NEXT(E)   = PIP_TASKQ_NEXT(L);			\
    PIP_TASKQ_PREV(E)      = (L);				\
    PIP_TASKQ_NEXT_PREV(L) = (E);				\
    PIP_TASKQ_NEXT(L)      = (E); } while(0)

#define PIP_TASKQ_ENQ_LAST(L,E)					\
  do { PIP_TASKQ_NEXT(E)   = (L);				\
    PIP_TASKQ_PREV(E)      = PIP_TASKQ_PREV(L);			\
    PIP_TASKQ_PREV_NEXT(L) = (E);				\
    PIP_TASKQ_PREV(L)      = (E); } while(0)

#define PIP_TASKQ_DEQ(L)					\
  do { PIP_TASKQ_NEXT_PREV(L) = PIP_TASKQ_PREV(L);		\
    PIP_TASKQ_PREV_NEXT(L) = PIP_TASKQ_NEXT(L); 		\
    PIP_TASKQ_INIT(L); } while(0)

#define PIP_TASKQ_MOVE_QUEUE(P,Q)				\
  do { if( !PIP_TASKQ_ISEMPTY(Q) ) {				\
      PIP_TASKQ_NEXT_PREV(Q) = (P);				\
      PIP_TASKQ_PREV_NEXT(Q) = (P);				\
      PIP_TASKQ_NEXT(P) = PIP_TASKQ_NEXT(Q);			\
      PIP_TASKQ_PREV(P) = PIP_TASKQ_PREV(Q);			\
      PIP_TASKQ_INIT(Q); } } while(0)

#define PIP_TASKQ_ISEMPTY(L)					\
  ( PIP_TASKQ_NEXT(L) == (L) && PIP_TASKQ_PREV(L) == (L) )

#define PIP_TASKQ_FOREACH(L,E)					\
  for( (E)=(L)->next; (L)!=(E); (E)=PIP_TASKQ_NEXT(E) )

#define PIP_TASKQ_FOREACH_SAFE(L,E,TV)				\
  for( (E)=(L)->next, (TV)=PIP_TASKQ_NEXT(E);			\
       (L)!=(E);						\
       (E)=(TV), (TV)=PIP_TASKQ_NEXT(TV) )

#define PIP_TASKQ_FOREACH_SAFE_XXX(L,E,TV)			\
  for( (E)=(L), (TV)=PIP_TASKQ_NEXT(E); (L)!=(E); (E)=(TV) )

#define PIP_LIST_INIT(L)		PIP_TASKQ_INIT(L)
#define PIP_LIST_ISEMPTY(L)		PIP_TASKQ_ISEMPTY(L)
#define PIP_LIST_ADD(L,E)		PIP_TASKQ_ENQ_LAST(L,E)
#define PIP_LIST_DEL(E)			PIP_TASKQ_DEQ(E)
#define PIP_LIST_MOVE(P,Q)		PIP_TASKQ_MOVE_QUEUE(P,Q)
#define PIP_LIST_FOREACH(L,E)		PIP_TASKQ_FOREACH(L,E)
#define PIP_LIST_FOREACH_SAFE(L,E,F)	PIP_TASKQ_FOREACH_SAFE(L,E,F)

typedef struct pip_mutex {
  void			*sched;
  void			*holder;
  pip_task_t		waiting;
} pip_mutex_t;

#ifdef AHAH
typedef struct pip_barrier {
  void			*sched;
  pip_task_t		waiting;
  int			count_init;
  int			count;
} pip_barrier_t;
#endif

struct pip_task;
typedef void (*pip_enqueuehook_t)(int);

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief the PiP library
 * @{
 */

  /**
   * \brief sleep the curren PiP task and enqueue it as a ULP
   *  @{
   * \param[in] queue A locked queue
   * \param[in] hook The callback function address to be called when
   *  the task is enqueued
   * \param[in] flags Enqueue policy
   *
   * \return Return true if the specified PiP task or ULP is alive
   * (i.e., not yet terminated) and running
   */
  int pip_sleep_and_enqueue( pip_locked_queue_t *queue,
			     pip_enqueuehook_t hook,
			     int flags );
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
   * \sa pip_suspend(3), pip_resume(3)
   */
  int pip_yield( void );
  /** @}*/

  /**
   * \brief Suspend the current PiP task and schedule the next taskeligible
   *  to run.
   *  @{
   *
   * The suspended task can be eligible to run when \b pip_resume()
   * is called. If there is no tasks eligible to run as the result of
   * calling this function, it returns \a EDEADLK.
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EDEADLK There is no other task eligible to run
   *
   * \sa pip_resume(3), pip_yield(3)
   */
  int pip_suspend( void );
  /** @}*/

  /**
   * \brief Resume PiP task to be eligible to run
   *  @{
   * \param[in] task passive taskto resume
   * \param[in] flags Specifying scheduling policy
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EINAVL \c task is \c NULL
   * \retval EBUSY The specified PiP task is already eligible to run
   *
   */
  int pip_resume( pip_task_t *task, int flags );
  /** @}*/

  /**
   * \brief Yield to the specified PiP task
   *  @{
   * \param[in] task Target PiP task to be scheduled
   *
   * Context-switch to the specified PiP task. If \c task is \c NULL, then
   * this works the same as \c pip_yield() does.
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM The specified ULP is being scheduled by the other
   * PiP task
   *
   */
  int pip_yield_to( pip_task_t *task );
  /** @}*/

  /**
   * \brief Get the current PiP task
   *  @{
   * \param[out] taskp The current PiP task
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EINAVL \c ulpp is \c NULL
   * \retval EPERM PiP library is not yet initialized
   *
   */
  int pip_myself( pip_task_t **taskp );
  /** @}*/

  /**
   * \brief Get the PiP task having the specified PiP ID
   *  @{
   * \param[in] pipid PiP ID
   * \param[out] taskp Returned PiP task
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EINAVL Invalid PiP ID is specified
   * \retval EPERM PiP library is not yet initialized
   */
  int pip_get( int pipid, pip_task_t **taskp );
  /** @}*/

  /**
   * \brief Get the PiP ID of the specified PiP task
   *  @{
   * \param[in] task a PiP task
   * \param[out] pipidp PiP ID pointer
   *
   * \note If \c task is \c NULL, then the PiP id of the current task is
   * returned.
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM PiP library is not yet initialized
   */
  int pip_get_pipid( pip_task_t *task, int *pipidp );
  /** @}*/

  /**
   * \brief Get the PiP ID of the scheduling PiP task of the current task
   *  @{
   * \param[out] pipidp PiP ID pointer of the scheduling task
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EINAVL \c pipidp is \c NULL
   * \retval The target is not eligible to run
   */
  int pip_get_sched_task( int *pipidp );
  /** @}*/

  /**
   * \brief Initialize PiP task mutex
   *  @{
   * \param[in,out] mutex pointer to the PiP task mutex
   *
   * \note This PiP task mutex can only be used to lock tasks and a PiP
   * task having the same scheduling domain.
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EINAVL \c mutex is \c NULL
   *
   * \sa pip_mutex_lock(3), pip_mutex_unlock(3)
   */
  int pip_mutex_init( pip_mutex_t *mutex );
  /** @}*/

  /**
   * \brief Lock PiP task mutex
   *  @{
   * \param[in] mutex pointer to the PiP task mutex
   *
   * \note This PiP task mutex can only be used to lock tasks and a PiP
   * task having the same scheduling domain.
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EINAVL \c mutex is \c NULL
   * \retval EDEADLK Tries to lock again
   * \retval EPERM The lock is owned by the other PiP task
   * (i.e. different scheduling domain)
   *
   * \sa pip_mutex_init(3), pip_mutex_unlock(3)
   */
  int pip_mutex_lock( pip_mutex_t *mutex );
  /** @}*/

  /**
   * \brief Unlock PiP task mutex
   *  @{
   * \param[in] mutex pointer to the PiP task mutex
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EINAVL \c mutex is \c NULL
   * \retval EPERM The lock is owned by the other PiP task
   * (i.e. different scheduling domain)
   *
   * \sa pip_mutex_init(3), pip_mutex_lock(3)
   */
  int pip_mutex_unlock( pip_mutex_t *mutex );
  /** @}*/

#ifdef AH
  /**
   * \brief Initialize PiP task barrier
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
   * \sa pip_barrier_wait(3), pip_task_barrier_init(3),
   * pip_task_barrier_wait(3), pip_universal_barrier_init(3),
   * pip_universal_barrier_wait(3)
   */
  int pip_barrier_init( pip_barrier_t *barrp, int n );
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
   * \sa pip_barrier_init(3), pip_task_barrier_init(3),
   * pip_task_barrier_wait(3), pip_universal_barrier_init(3),
   * pip_universal_barrier_wait(3)
   */
  int pip_barrier_wait( pip_barrier_t *barrp );
  /** @}*/
#endif

  /**
   * \brief Initialize alocked task queue
   *  @{
   * \param[in] queue pointer to the locked queue to be initialized
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EINAVL \c queue is \c NULL
   *
   * \sa pip_suspend_and_enqueue(3),
   * pip_dequeue_and_involve(3), pip_enqueue_with_lock(3),
   * pip_dequeue_with_lock(3)
   */
  int pip_locked_queue_init( pip_locked_queue_t *queue );
  /** @}*/

  /**
   * \brief Suspend the current PiP task and enqueue it to the
   *  specified locked queue for possible migration
   *  @{
   * \param[in] queue pointer to a locked task queue
   * \param[in] hook The callback function address to be called when
   *  the task is enqueued
   * \param[in] flag Specifying scheduling policy
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EINAVL \c queue is \c NULL
   * \retval EPERM a PiP task cannot be involved
   *
   * \sa pip_locked_queue_init(3),
   * pip_dequeue_and_involve(3), pip_enqueue_with_lock(3),
   * pip_dequeue_with_lock(3)
   */
  int pip_suspend( pip_locked_queue_t *queue,
		   pip_enqueuehook_t hook,
		   int flag );
  /** @}*/

  /**
   * \brief Dequeue a PiP task from the locked queue and make it
   *  eligible to run
   *  @{
   * \param[in] queue pointer to a locked task queue
   * \param[out] taskp Dequeued PiP task
   * \param[in] flag Specifying scheduling policy
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EINAVL \c queue is \c NULL
   * \retval ENOENT The specified queue is empty
   *
   * \sa pip_locked_queue_init(3),
   * pip_suspend(3), pip_enqueue_with_lock(3),
   * pip_dequeue_with_lock(3)
   */
  int pip_dequeue( pip_locked_queue_t *queue,
		   pip_task_t **taskp,
		   int flag );
  /** @}*/

  /**
   * \brief Put a PiP task into the specified locked queue
   *  @{
   * \param[in] queue pointer to a locked task queue
   * \param[out] task PiP task to be enqueued
   * \param[in] flag Specifying scheduling policy
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EINAVL \c queue is \c NULL
   *
   * \sa pip_locked_queue_init(3), pip_dequeue_and_involve(3),
   * pip_suspend_and_enqueue(3),
   * pip_dequeue_with_lock(3)
   */
  int pip_enqueue_with_lock( pip_locked_queue_t *queue,
			     pip_task_t *task,
			     int flag );
  /** @}*/

  /**
   * \brief Dequeue the first PiP task in the locked queue
   *  @{
   * \param[in] queue pointer to a locked task queue
   * \param[out] taskp pointer to the dequeued PiP task
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EINAVL \c queue is \c NULL
   *
   * \sa pip_locked_queue_init(3), pip_dequeue_and_involve(3),
   * pip_suspend_and_enqueue(3), pip_enqueue_with_lock(3)
   */
  int pip_dequeue_with_lock( pip_locked_queue_t *queue,
			     pip_task_t **taskp );
  /** @}*/

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _pip_ulp_h_ */
