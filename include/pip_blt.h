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
  * Written by Atsushi HORI <ahori@riken.jp>, 2016-2019
*/

#ifndef _pip_blt_h_
#define _pip_blt_h_

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <pip.h>

#define PIP_SYNC_AUTO			(0x0001)
#define PIP_SYNC_BUSYWAIT		(0x0002)
#define PIP_SYNC_YIELD			(0x0004)
#define PIP_SYNC_BLOCKING		(0x0008)

#define PIP_SYNC_MASK					\
  (PIP_SYNC_BUSYWAIT|PIP_SYNC_BLOCKING|PIP_SYNC_YIELD|PIP_SYNC_AUTO)

#define PIP_TASK_ALL			(-1)

#define PIP_ENV_SYNC			"PIP_SYNC"
#define PIP_ENV_SYNC_AUTO		"auto"
#define PIP_ENV_SYNC_BUSY		"busy"
#define PIP_ENV_SYNC_BUSYWAIT		"busywait"
#define PIP_ENV_SYNC_YIELD		"yield"
#define PIP_ENV_SYNC_BLOCK		"block"
#define PIP_ENV_SYNC_BLOCKING		"blocking"

#define PIP_TOPT_BUSYWAIT		(0x01)


typedef struct pip_queue {
  struct pip_queue	*next;
  struct pip_queue	*prev;
} pip_task_t;

typedef pip_task_t	pip_list_t;

#define PIP_TASKQ_NEXT(L)	((L)->next)
#define PIP_TASKQ_PREV(L)	((L)->prev)
#define PIP_TASKQ_PREV_NEXT(L)	((L)->prev->next)
#define PIP_TASKQ_NEXT_PREV(L)	((L)->next->prev)

#define PIP_TASKQ_INIT(L)					\
  do { PIP_TASKQ_NEXT(L) = (L); PIP_TASKQ_PREV(L) = (L); } while(0)

#define PIP_TASKQ_ENQ_FIRST(L,E)				\
  do { PIP_TASKQ_NEXT(E)      = PIP_TASKQ_NEXT(L);		\
       PIP_TASKQ_PREV(E)      = (L);				\
       PIP_TASKQ_NEXT_PREV(L) = (E);				\
       PIP_TASKQ_NEXT(L)      = (E); } while(0)

#define PIP_TASKQ_ENQ_LAST(L,E)					\
  do { PIP_TASKQ_NEXT(E)      = (L);				\
       PIP_TASKQ_PREV(E)      = PIP_TASKQ_PREV(L);		\
       PIP_TASKQ_PREV_NEXT(L) = (E);				\
       PIP_TASKQ_PREV(L)      = (E); } while(0)

#define PIP_TASKQ_DEQ(E)					\
  do { PIP_TASKQ_NEXT_PREV(E) = PIP_TASKQ_PREV(E);		\
       PIP_TASKQ_PREV_NEXT(E) = PIP_TASKQ_NEXT(E); 		\
       PIP_TASKQ_INIT(E); } while(0)

#define PIP_TASKQ_APPEND(P,Q)					\
  do { if( !PIP_TASKQ_ISEMPTY(Q) ) {				\
      PIP_TASKQ_NEXT_PREV(Q) = PIP_TASKQ_PREV(P);		\
      PIP_TASKQ_PREV_NEXT(Q) = (P);				\
      PIP_TASKQ_PREV_NEXT(P) = PIP_TASKQ_NEXT(Q);		\
      PIP_TASKQ_PREV(P)      = PIP_TASKQ_PREV(Q);		\
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

struct pip_task_queue_methods;

typedef struct pip_task_queue {
  pip_task_t			queue;
  struct pip_task_queue_methods	*methods;
  pip_spinlock_t		lock;
  int32_t			length;
} pip_task_queue_t;

typedef void(*pip_enqueue_callback_t)(void*);

typedef int(*pip_task_queue_init_t)	(void*);
typedef void(*pip_task_queue_lock_t)	(void*);
typedef int(*pip_task_queue_trylock_t)	(void*);
typedef void(*pip_task_queue_unlock_t)	(void*);
typedef int(*pip_task_queue_isempty_t)	(void*);
typedef int(*pip_task_queue_count_t)	(void*, int*);
typedef void(*pip_task_queue_enqueue_t)	(void*, pip_task_t*);
typedef pip_task_t*(*pip_task_queue_dequeue_t)	(void*);
typedef void(*pip_task_queue_describe_t)	(void*,FILE*);
typedef int(*pip_task_queue_fin_t)	(void*);

typedef struct pip_task_queue_methods {
  pip_task_queue_init_t		init;
  pip_task_queue_lock_t		lock;
  pip_task_queue_trylock_t	trylock;
  pip_task_queue_unlock_t	unlock;
  pip_task_queue_isempty_t  	isempty;
  pip_task_queue_count_t	count;
  pip_task_queue_enqueue_t	enqueue;
  pip_task_queue_dequeue_t	dequeue;
  pip_task_queue_describe_t	describe;
  pip_task_queue_fin_t		finalize;
} pip_task_queue_methods_t;

#define PIP_QUEUE_TYPEDEF(T,V,N)    \
  typedef struct N {		    \
    pip_task_queue_t	task_queue; \
    T		   	V;	    \
  } N;

typedef struct pip_mutex_head {
  pip_atomic_t			lock;
  uint32_t			flags;
} pip_mutex_head_t;

typedef struct pip_mutex {
  pip_mutex_head_t		head;
  pip_task_queue_t		queue; /* this must be the last elm */
} pip_mutex_t;

typedef struct pip_barrier_head {
  pip_atomic_t			count_init;
  pip_atomic_t			count;
} pip_barrier_head_t;

typedef struct pip_barrier {
  pip_barrier_head_t		head;
  pip_task_queue_t		queue; /* this must be the last elm */
} pip_barrier_t;

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

/**
 * @addtogroup libpip libpip
 * \brief the PiP library
 * @{
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifdef __cplusplus
extern "C" {
#endif
#endif

  /**
   * \brief Initialize task queue
   *  @{
   * \param[in] queue A task queue
   * \param[in] methods Usre defined function table. If NULL then
   *  default functions will be used.
   * \param[in] aux User data associate with the queue
   *
   * \return This function returns no error
   */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
  int pip_task_queue_init( pip_task_queue_t *queue,
			   pip_task_queue_methods_t *methods );
  /** @}*/
#else
  static inline int pip_task_queue_init_( pip_task_queue_t *queue,
					  pip_task_queue_methods_t *methods ) {
    memset( queue, 0, sizeof(pip_task_queue_t) );
    PIP_TASKQ_INIT( &queue->queue );
    if( methods == NULL ) {
      queue->methods = NULL;
      pip_spin_init( &queue->lock );
      return 0;
    } else {
      queue->methods = methods;
      return queue->methods->init( queue );
    }
  }
#define pip_task_queue_init( Q, M )				\
  pip_task_queue_init_( (pip_task_queue_t*)(Q), (M) )
#endif

  /**
   * \brief Count the length of task queue
   *  @{
   * \param[in] queue A task queue
   * \param[out] np the queue length returned
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EINVAL \c np is \c NULL
   */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
  int pip_task_queue_count( pip_task_queue_t *queue, int *np );
  /** @}*/
#else
  static inline int
  pip_task_queue_count_( pip_task_queue_t *queue, int *np ) {
    int err = 0;
    if( np == NULL ) return EINVAL;
    if( queue->methods == NULL ) {
      *np = queue->length;
    } else {
      err = queue->methods->count( queue, np );
    }
    return err;
  }
#define pip_task_queue_count( Q, NP )			\
  pip_task_queue_count_( (pip_task_queue_t*)(Q), (NP) )
#endif

  /**
   * \brief Lock task queue
   *  @{
   * \param[in] queue A task queue
   *
   * \return This function returns no error
   */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
  void pip_task_queue_lock( pip_task_queue_t *queue );
  /** @}*/
#else
  static inline void pip_task_queue_lock_( pip_task_queue_t *queue ) {
    if( queue->methods == NULL ) {
      pip_spin_lock( &queue->lock );
    } else {
      queue->methods->lock( queue );
    }
  }
#define pip_task_queue_lock( Q )			\
  pip_task_queue_lock_( (pip_task_queue_t*)(Q) )
#endif

  /**
   * \brief Try locking task queue
   *  @{
   * \param[in] queue A task queue
   *
   * \return Returns true if lock succeeds.
   */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
  int pip_task_queue_trylock( pip_task_queue_t *queue );
  /** @}*/
#else
  static inline int pip_task_queue_trylock_( pip_task_queue_t *queue ) {
    if( queue->methods == NULL ) {
      return pip_spin_trylock( &queue->lock );
    } else {
      return queue->methods->trylock( (void*) queue );
    }
  }
#define pip_task_queue_trylock( Q )			\
  pip_task_queue_trylock_( (pip_task_queue_t*)(Q) )
#endif

  /**
   * \brief Unlock task queue
   *  @{
   * \param[in] queue A task queue
   *
   * \return This function returns no error
   */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
  void pip_task_queue_unlock( pip_task_queue_t *queue );
  /** @}*/
#else
  static inline void pip_task_queue_unlock_( pip_task_queue_t *queue ) {
    if( queue->methods == NULL ) {
      pip_spin_unlock( &queue->lock );
    } else {
      queue->methods->unlock( (void*) queue );
    }
  }
#define pip_task_queue_unlock( Q ) 	\
  pip_task_queue_unlock_( (pip_task_queue_t*)(Q) )
#endif

  /**
   * \brief Query function if the current task has some tasks to be
   *  scheduled with.
   *  @{
   * \param[in] queue A task queue
   *
   * \return Returns true if there is no tasks to schedule.
   */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
  int pip_task_queue_isempty( pip_task_queue_t *queue );
  /** @}*/
#else
  static inline int pip_task_queue_isempty_( pip_task_queue_t *queue ) {
    if( queue->methods == NULL ) {
      return PIP_TASKQ_ISEMPTY( &queue->queue );
    } else {
      return queue->methods->isempty( (void*) queue );
    }
  }
#define pip_task_queue_isempty( Q )			\
  pip_task_queue_isempty_( (pip_task_queue_t*)(Q) )
#endif

  /**
   * \brief Try locking task queue
   *  @{
   * \param[in] queue A task queue
   *
   * \return Returns true if lock succeeds.
   */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
  void pip_task_queue_enqueue( pip_task_queue_t *queue, pip_task_t *task );
  /** @}*/
#else
  static inline void
  pip_task_queue_enqueue_( pip_task_queue_t *queue, pip_task_t *task ) {
    if( queue->methods == NULL ) {
      PIP_TASKQ_ENQ_LAST( &queue->queue, task );
      queue->length ++;
    } else {
      queue->methods->enqueue( (void*) queue, task );
    }
  }
#define pip_task_queue_enqueue( Q, T )				\
  pip_task_queue_enqueue_( (pip_task_queue_t*) (Q), (T) )
#endif

  /**
   * \brief Dequeue a task from a task queue
   *  @{
   * \param[in] queue A task queue
   *
   * \return Dequeue a task in the specified task queue and return
   * it. If the task queue is empty then \b NULL is returned.
   */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
  pip_task_t* pip_task_queue_dequeue( pip_task_queue_t *queue );
  /** @}*/
#else
  static inline pip_task_t*
  pip_task_queue_dequeue_( pip_task_queue_t *queue ) {
    pip_task_t 		*task;

    if( queue->methods == NULL ) {
      if( PIP_TASKQ_ISEMPTY( &queue->queue ) ) return NULL;
      task = PIP_TASKQ_NEXT( &queue->queue );
      PIP_TASKQ_DEQ( task );
      queue->length --;
      return task;
    } else {
      return queue->methods->dequeue( (void*) queue );
    }
  }
#define pip_task_queue_dequeue( Q )			\
  pip_task_queue_dequeue_( (pip_task_queue_t*) (Q) )
#endif

extern void pip_task_queue_brief( pip_task_t *task, char *msg, size_t len );

  /**
   * \brief Describe queue
   *  @{
   * \param[in] queue A task queue
   * \param[in] a file pointer
   *
   * \return This function returns no error
   */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
  void pip_task_queue_describe( pip_task_queue_t *queue, FILE *fp );
  /** @}*/
#else
  static inline void
  pip_task_queue_describe_( pip_task_queue_t *queue, char *tag, FILE *fp ) {
    if( queue->methods == NULL ) {
      if( PIP_TASKQ_ISEMPTY( &queue->queue ) ) {
	fprintf( fp, "%s: (EMPTY)\n", tag );
      } else {
	pip_task_t *task;
	char msg[512];
	int i = 0;
	PIP_TASKQ_FOREACH( &queue->queue, task ) {
	  pip_task_queue_brief( task, msg, 512 );
	  if( i == 0 ) {
	    fprintf( fp, "%s: [0]:%s", tag, msg );
	  } else {
	    fprintf( fp, ", [%d]:%s", i, msg );
	  }
	  i++;
	}
	fprintf( fp, "\n" );
      }
    } else {
      queue->methods->describe( (void*) queue, fp );
    }
  }
#define pip_task_queue_describe( Q, T, F )			\
  pip_task_queue_describe_( (pip_task_queue_t*)(Q), (T), (F) )
#endif

  /**
   * \brief Finalize a task
   *  @{
   * \param[in] queue A task queue
   *
   * \return If succeedss, 0 is returned. Otherwise an error code is
   * returned.
   */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
  int pip_task_queue_fin( pip_task_queue_t *queue );
  /** @}*/
#else
  static inline int pip_task_queue_fin_( pip_task_queue_t *queue ) {
    if( queue->methods == NULL ) {
      if( !PIP_TASKQ_ISEMPTY( &queue->queue ) ) return EBUSY;
      return 0;
    } else {
      return queue->methods->finalize( (void*) queue );
    }
  }
#define pip_task_queue_fin( Q )			\
  pip_task_queue_fin_( (pip_task_queue_t*)(Q) )
#endif

  /**
   * \brief Yield
   *  @{
   *
   * \return Return 0 on success. Return an error code on error.
   *
   * \sa pip_yield_to(3)
   */
  int pip_yield( void );
  /** @}*/

  /**
   * \brief Yield to the specified PiP task
   *  @{
   * \param[in] task Target PiP task to switch
   *
   * Context-switch to the specified PiP task. If \c task is \c NULL, then
   * this works the same as \c pip_yield() does.
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM The specified task belongs to the other scheduling
   * domain.
   *
   * \sa pip_yield(3)
   */
  int pip_yield_to( pip_task_t *task );
  /** @}*/

  /**
   * \brief suspend the curren task and enqueue it
   *  @{
   * \param[in] queue A task queue
   * \param[in] callback A callback function which is called when enqueued
   * \param[in] cbarg An argument given to the callback function
   *
   * \return Return 0 on success. Return an error code on error.
   *
   * The \b queue is locked and unlocked when the current task is
   * enqueued. Then the \b callback function is called.
   *
   * As the result of suspension, if there is no other tasks to be
   * scheduled then the kernel thread will be blocked until it will be
   * given a task by resuming a suspended task.
   *
   * \sa pip_enqueu_and_suspend_nolock(3), pip_dequeue_and_resume(3)
   */
  int pip_suspend_and_enqueue( pip_task_queue_t *queue,
			       pip_enqueue_callback_t callback,
			       void *cbarg );
  /** @}*/

  /**
   * \brief suspend the curren task and enqueue it without locking the queue
   *  @{
   * \param[in] queue A task queue
   * \param[in] callback A callback function which is called when enqueued
   * \param[in] cbarg An argument given to the callback function
   *
   * \return Return 0 on success. Return an error code on error.
   *
   * It is the user's responsibility to lock the queue beofre calling
   * this function and unlock the queue after calling this
   * function. When the current task is enqueued the \b callback
   * function will be called.
   *
   * As the result of suspension, if there is no other tasks to be
   * scheduled then the kernel thread will be blocked until it will be
   * given a task by resuming a suspended task.
   *
   */
  int pip_suspend_and_enqueue_nolock( pip_task_queue_t *queue,
				      pip_enqueue_callback_t callback,
				      void *cbarg );
  /** @}*/

  /**
   * \brief dequeue a task and make it runnable
   *  @{
   * \param[in] queue A task queue
   * \param[in] sched A task to specify a scheduling domain
   *
   * \return If succeedss, 0 is returned. Otherwise an error code is
   * returned.
   * \retval ENOENT The queue is empty.
   *
   * The \b queue is locked and unlocked when dequeued.
   */
  int pip_dequeue_and_resume( pip_task_queue_t *queue, pip_task_t *sched );
  /** @}*/

  /**
   * \brief dequeue a task and make it runnable
   *  @{
   * \param[in] queue A task queue
   * \param[in] sched A task to specify a scheduling domain
   *
   * \return This function returns no error
   * \retval ENOENT The queue is empty.
   *
   * It is the user's responsibility to lock the queue beofre calling
   * this function and unlock the queue after calling this
   * function.
   */
  int pip_dequeue_and_resume_nolock( pip_task_queue_t *queue,
				     pip_task_t *sched );
  /** @}*/

  /**
   * \brief dequeue tasks and resume the execution of them
   *  @{
   * \param[in] queue A task queue
   * \param[in] sched A task to specify a scheduling domain
   * \param[inout] np A pointer to an interger which spcifies the
   *  number of tasks dequeued and actual number of tasks dequeued is
   *  returned.
   *
   * \return This function returns no error
   * \retval EINVAL the specified number of tasks is negative
   *
   * The \b queue is locked and unlocked when dequeued.
   *
   * It is the user's responsibility to lock the queue beofre calling
   * this function and unlock the queue after calling this
   * function.
   */
  int pip_dequeue_and_resume_N( pip_task_queue_t *queue,
				pip_task_t *sched,
				int *np );
  /** @}*/

  /**
   * \brief dequeue tasks and resume the execution of them
   *  @{
   * \param[in] queue A task queue
   * \param[in] sched A task to specify a scheduling domain
   * \param[inout] np A pointer to an interger which spcifies the
   *  number of tasks dequeued and actual number of tasks dequeued is
   *  returned.
   *
   * \return This function returns no error
   * \retval EINVAL the specified number of tasks is negative
   *
   * It is the user's responsibility to lock the queue beofre calling
   * this function and unlock the queue after calling this
   * function.
   */
  int pip_dequeue_and_resume_N_nolock( pip_task_queue_t *queue,
				       pip_task_t *sched,
				       int *np );
  /** @}*/

  /**
   * \brief Return the current task
   *  @{
   *
   * \return Return the current task.
   *
   */
  pip_task_t *pip_task_self( void );
  /** @}*/

  /**
   * \brief count the number of runnable tasks in the same scheduling
   *  domain
   *  @{
   * \param[out] countp number of tasks will be returned
   *
   * \return This function returns no error
   */
  int pip_count_runnable_tasks( int *countp );
  /** @}*/

  /**
   * \brief initialize barrier synchronization structure
   *  @{
   *
   * \param[in] barrp pointer to a PiP barrier structure
   * \param[in] n number of participants of this barrier
   * synchronization
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EINAVL \c n is invalid
   *
   * \note This barrier works on PiP tasks only.
   *
   * \sa pip_barrier_wait(3),
   * pip_barrier_init(3),
   * pip_barrier_wait(3),
   */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
  int pip_barrier_init( pip_barrier_t *barrp, int n );
  /** @}*/
#else
  int pip_barrier_init_( pip_barrier_t *barrp, int n );
#define pip_barrier_init( B, N ) \
  pip_barrier_init_( (pip_barrier_t*)(B), (N) )
#endif

  /**
   * \brief wait on barrier synchronization in a busy-wait way
   *  @{
   *
   * \param[in] barrp pointer to a PiP barrier structure
   *
   * \return Return 0 on success. Return an error code on error.
   *
   * \sa pip_barrier_init(3), pip_barrier_init(3),
   *
   */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
  int pip_barrier_wait( pip_barrier_t *barrp );
  /** @}*/
#else
  int pip_barrier_wait_( pip_barrier_t *barrp );
#define pip_barrier_wait( B ) \
  pip_barrier_wait_( (pip_barrier_t*)(B) )
#endif

  /**
   * \brief finalize barrier synchronization structure
   *  @{
   *
   * \param[in] barrp pointer to a PiP barrier structure
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EBUSY there are some tasks wating for barrier
   * synchronization
   *
   * \sa pip_barrier_wait(3),
   * pip_barrier_init(3),
   * pip_barrier_wait(3),
   */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
  int pip_barrier_fin( pip_barrier_t *queue );
  /** @}*/
#else
  int pip_barrier_fin_( pip_barrier_t *queue );
#define pip_barrier_fin( B ) \
  pip_barrier_fin_( (pip_barrier_t*)(B) )
#endif

  /**
   * \brief Initialize PiP mutex
   *  @{
   * \param[in,out] mutex pointer to the PiP task mutex
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EINAVL \c mutex is \c NULL
   *
   * \sa pip_mutex_lock(3), pip_mutex_unlock(3)
   */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
  int pip_mutex_init( pip_mutex_t *mutex );
  /** @}*/
#else
  int pip_mutex_init_( pip_mutex_t *mutex );
#define pip_mutex_init( M ) \
  pip_mutex_init_( (pip_mutex_t*)(M) )
#endif

  /**
   * \brief Lock PiP mutex
   *  @{
   * \param[in] mutex pointer to the PiP task mutex
   *
   * \return Return 0 on success. Return an error code on error.
   *
   * \sa pip_mutex_init(3), pip_mutex_unlock(3)
   */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
  int pip_mutex_lock( pip_mutex_t *mutex );
  /** @}*/
#else
  int pip_mutex_lock_( pip_mutex_t *mutex );
#define pip_mutex_lock( M ) \
  pip_mutex_lock_( (pip_mutex_t*)(M) )
#endif

  /**
   * \brief Unlock PiP mutex
   *  @{
   * \param[in] mutex pointer to the PiP task mutex
   *
   * \return Return 0 on success. Return an error code on error.
   *
   * \sa pip_mutex_init(3), pip_mutex_lock(3)
   */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
  int pip_mutex_unlock( pip_mutex_t *mutex );
  /** @}*/
#else
  int pip_mutex_unlock_( pip_mutex_t *mutex );
#define pip_mutex_unlock( M ) \
  pip_mutex_unlock_( (pip_mutex_t*)(M) )
#endif

  /**
   * \brief Finalize PiP mutex
   *  @{
   * \param[in,out] mutex pointer to the PiP task mutex
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EBUSY There is one or more waiting PiP task
   *
   * \sa pip_mutex_lock(3), pip_mutex_unlock(3)
   */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
  int pip_mutex_fin( pip_mutex_t *mutex );
  /** @}*/
#else
  int pip_mutex_fin_( pip_mutex_t *mutex );
#define pip_mutex_fin( M ) \
  pip_mutex_fin_( (pip_mutex_t*)(M) )
#endif

/**
 * @}
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifdef __cplusplus
}
#endif
#endif

#endif /* _pip_blt_h_ */
