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

#ifndef _pip_blt_h_
#define _pip_blt_h_

#ifdef DOXYGEN_SHOULD_SKIP_THIS
#ifndef DOXYGEN_INPROGRESS
#define DOXYGEN_INPROGRESS
#endif
#endif

#ifndef DOXYGEN_INPROGRESS

#include <pip.h>
#include <pip_machdep.h>
#include <string.h>

#define PIP_SYNC_AUTO			(0x0001)
#define PIP_SYNC_BUSYWAIT		(0x0002)
#define PIP_SYNC_YIELD			(0x0004)
#define PIP_SYNC_BLOCKING		(0x0008)

#define PIP_SYNC_MASK			(PIP_SYNC_AUTO     |	\
					 PIP_SYNC_BUSYWAIT |	\
					 PIP_SYNC_YIELD    |	\
					 PIP_SYNC_BLOCKING)

#define PIP_TASK_INACTIVE		(0x01000)
#define PIP_TASK_ACTIVE			(0x02000)

#define PIP_TASK_MASK				\
  (PIP_TASK_INACTIVE|PIP_TASK_ACTIVE)

#define PIP_TASK_ALL			(-1)

#define PIP_YIELD_DEFAULT		(0x0)
#define PIP_YIELD_USER			(0x1)
#define PIP_YIELD_SYSTEM		(0x2)

#define PIP_ENV_SYNC			"PIP_SYNC"
#define PIP_ENV_SYNC_AUTO		"auto"
#define PIP_ENV_SYNC_BUSY		"busy"
#define PIP_ENV_SYNC_BUSYWAIT		"busywait"
#define PIP_ENV_SYNC_YIELD		"yield"
#define PIP_ENV_SYNC_BLOCK		"block"
#define PIP_ENV_SYNC_BLOCKING		"blocking"

#define PIP_TOPT_BUSYWAIT		(0x01)

typedef struct pip_task {
  struct pip_task	*next;
  struct pip_task	*prev;
} pip_task_t;

#define PIP_TASKQ_NEXT(L)	(((pip_task_t*)(L))->next)
#define PIP_TASKQ_PREV(L)	(((pip_task_t*)(L))->prev)
#define PIP_TASKQ_PREV_NEXT(L)	(((pip_task_t*)(L))->prev->next)
#define PIP_TASKQ_NEXT_PREV(L)	(((pip_task_t*)(L))->next->prev)

//#ifdef DEBUG
#ifdef AH
#define PIP_TASKQ_CHECK(Q)					\
  ASSERTD( PIP_TASKQ_NEXT(Q) != PIP_TASKQ_PREV(Q) )
#else
#define PIP_TASKQ_CHECK(Q)
#endif

#define PIP_TASKQ_INIT(L)					\
  do { PIP_TASKQ_NEXT(L) = PIP_TASKQ_PREV(L) =			\
      (pip_task_t*)(L); } while(0)

#define PIP_TASKQ_ENQ_FIRST(L,E)				\
  do { PIP_TASKQ_CHECK(E);					\
       PIP_TASKQ_NEXT(E)      = PIP_TASKQ_NEXT(L);		\
       PIP_TASKQ_PREV(E)      = (pip_taskt*)(L);		\
       PIP_TASKQ_NEXT_PREV(L) = (pip_task_t*)(E);		\
       PIP_TASKQ_NEXT(L)      = (pip_task_t*)(E); } while(0)

#define PIP_TASKQ_ENQ_LAST(L,E)					\
  do { PIP_TASKQ_CHECK(E);					\
       PIP_TASKQ_NEXT(E)      = (pip_task_t*)(L);		\
       PIP_TASKQ_PREV(E)      = PIP_TASKQ_PREV(L);		\
       PIP_TASKQ_PREV_NEXT(L) = (pip_task_t*)(E);		\
       PIP_TASKQ_PREV(L)      = (pip_task_t*)(E); } while(0)

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
  ( PIP_TASKQ_NEXT(L) == (L) && PIP_TASKQ_PREV(L) == (pip_task_t*)(L) )

#define PIP_TASKQ_FOREACH(L,E)					\
  for( (E)=PIP_TASKQ_NEXT(L); (pip_task_t*)(L)!=(E);		\
       (E)=PIP_TASKQ_NEXT(E) )

#define PIP_TASKQ_FOREACH_SAFE(L,E,TV)				\
  for( (E)=PIP_TASKQ_NEXT(L), (TV)=PIP_TASKQ_NEXT(E);		\
       (pip_task_t*)(L)!=(E);					\
       (E)=(TV), (TV)=PIP_TASKQ_NEXT(TV) )

#define PIP_TASKQ_MOVE(D,S)		\
  do { if( PIP_TASKQ_ISEMPTY(S) ) {		\
      PIP_TASKQ_INIT(D);			\
    } else {					\
      (D)->next = (S)->next;			\
      (D)->prev = (S)->prev;			\
      PIP_TASKQ_INIT(S); } } while(0)


typedef pip_task_t	pip_list_t;

/* aliases */
#define PIP_LIST_INIT(L)		PIP_TASKQ_INIT(L)
#define PIP_LIST_ISEMPTY(L)		PIP_TASKQ_ISEMPTY(L)
#define PIP_LIST_ADD(L,E)		PIP_TASKQ_ENQ_LAST(L,E)
#define PIP_LIST_DEL(E)			PIP_TASKQ_DEQ(E)
#define PIP_LIST_FOREACH(L,E)		PIP_TASKQ_FOREACH(L,E)
#define PIP_LIST_FOREACH_SAFE(L,E,F)	PIP_TASKQ_FOREACH_SAFE(L,E,F)
#define PIP_LIST_MOVE(D,S)		PIP_TASKQ_MOVE(D,S)

struct pip_task_queue_methods;

typedef struct pip_task_queue {
  volatile pip_task_t			queue;
  struct pip_task_queue_methods		*methods;
  pip_spinlock_t			lock;
  volatile uint32_t			length;
  void					*aux;
} pip_task_queue_t;

typedef void(*pip_enqueue_callback_t)(void*);

#define PIP_CB_UNLOCK_AFTER_ENQUEUE	((pip_enqueue_callback_t)1)

typedef int  (*pip_task_queue_init_t)		(void*);
typedef void (*pip_task_queue_lock_t)		(void*);
typedef int  (*pip_task_queue_trylock_t)	(void*);
typedef void (*pip_task_queue_unlock_t)		(void*);
typedef int  (*pip_task_queue_isempty_t)	(void*);
typedef int  (*pip_task_queue_count_t)		(void*, int*);
typedef void (*pip_task_queue_enqueue_t)	(void*, pip_task_t*);
typedef pip_task_t *(*pip_task_queue_dequeue_t)	(void*);
typedef void (*pip_task_queue_describe_t)	(void*,FILE*);
typedef int (*pip_task_queue_fin_t)		(void*);

typedef struct pip_task_queue_methods {
  pip_task_queue_init_t		init;
  pip_task_queue_lock_t		lock;
  pip_task_queue_trylock_t	trylock;
  pip_task_queue_unlock_t	unlock;
  pip_task_queue_isempty_t  	isempty;
  pip_task_queue_count_t	count;
  pip_task_queue_enqueue_t	enqueue;
  pip_task_queue_dequeue_t	dequeue;
  pip_task_queue_fin_t		fin;
  pip_task_queue_describe_t	describe;
} pip_task_queue_methods_t;

typedef struct pip_barrier {
  pip_atomic_t			count_init;
  pip_atomic_t			count;
  pip_task_queue_t		queue; /* this must be placed at the end */
} pip_barrier_t;

typedef struct pip_mutex {
  pip_atomic_t			count;
  pip_task_queue_t		queue; /* this must be placed at the end */
} pip_mutex_t;

#define PIP_QUEUE_TYPEDEF(N,B)	    \
  typedef struct N {		    \
    pip_task_queue_t	pip_queue; \
    B;				   \
  } N;

#define PIP_MUTEX_TYPEDEF(N,B)	    \
  typedef struct N {		    \
    pip_mutex_t		pip_mutex; \
    B;				   \
  } N;

#define PIP_BARTIER_TYPEDEF(N,B)    \
  typedef struct N {		    \
    pip_barrier_t	pip_barrier; \
    B;				     \
  } N;

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#ifdef DOXYGEN_INPROGRESS
#ifndef INLINE
#define INLINE
#endif
#else
#ifndef INLINE
#define INLINE			inline static
#endif
#endif

/**
 * @addtogroup libpip libpip
 * \brief the PiP library
 * @{
 */

extern int pip_task_str( char*, size_t, pip_task_t* );

#ifndef DOXYGEN_INPROGRESS
#ifdef __cplusplus
extern "C" {
#endif
#endif

  /**
   * \brief spawn a PiP BLT (Bi-Level Task)
   *  @{
   * \param[in] progp Program information to spawn as a PiP task
   * \param[in] coreno Core number for the PiP task to be bound to. If
   *  \c PIP_CPUCORE_ASIS is specified, then the core binding will not
   *  take place.
   * \param[in] opts option flags
   * \param[in,out] pipidp Specify PiP ID of the spawned PiP task. If
   *  \c PIP_PIPID_ANY is specified, then the PiP ID of the spawned PiP
   *  task is up to the PiP library and the assigned PiP ID will be
   *  returned.
   * \param[in,out] bltp returns created BLT
   * \param[in] queue PiP task queue where the created BLT will be added
   * \param[in] hookp Hook information to be invoked before and after
   *  the program invokation.
   *
   * \note In theory, there is no reason to restrict for a PiP task to
   * spawn another PiP task. However, the current implementation fails
   * to do so. If the root process is multithreaded, only the main
   * thread can call this function.
   * \note In the process mode, the file descriptors set the
   * close-on-exec flag will be closed on the created child task.
   *
   * \return zero is returned if this function succeeds. On error, an
   * error number is returned.
   * \retval EPERM PiP task tries to spawn child task
   * \retval EBUSY Specified PiP ID is alredy occupied
   *
   * \sa pip_task_spawn(3), pip_spawn_from_main(3)
   *
   */
#ifdef DOXYGEN_INPROGRESS
int pip_blt_spawn( pip_spawn_program_t *progp,
		   uint32_t coreno,
		   uint32_t opts,
		   int *pipidp,
		   pip_task_t **bltp,
		   pip_task_queue_t *queue,
		   pip_spawn_hook_t *hookp );
  /** @}*/
#else
int pip_blt_spawn_( pip_spawn_program_t *progp,
		    uint32_t coreno,
		    uint32_t opts,
		    int *pipidp,
		    pip_task_t **bltp,
		    pip_task_queue_t *queue,
		    pip_spawn_hook_t *hookp );
#define pip_blt_spawn( progp, coreno, opts, pipid, bltp, queue, hookp ) \
  pip_blt_spawn_( (progp), (coreno), (opts), (pipid), (bltp),		\
		  (pip_task_queue_t*) (queue), (hookp) )
#endif

  /**
   * \brief Yield
   *  @{
   * \param[in] flag to specify the behavior of yielding
   *
   * \return Return 0 on success. Return an error code on error.
   *
   * \sa pip_yield_to(3)
   */
  int pip_yield( int flag );
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
   * \retval EPERM PiP library is not yet initialized or already
   *
   * \sa pip_yield(3)
   */
  int pip_yield_to( pip_task_t *task );
  /** @}*/

  /**
   * \brief Initialize task queue
   *  @{
   * \param[in] queue A task queue
   * \param[in] methods Usre defined function table. If NULL then
   *  default functions will be used.
   *
   * \return This function returns no error
   */
#ifdef DOXYGEN_INPROGRESS
  int pip_task_queue_init( pip_task_queue_t *queue,
			   pip_task_queue_methods_t *methods );
  /** @}*/
#else
  INLINE int pip_task_queue_init_( pip_task_queue_t *queue,
				   pip_task_queue_methods_t *methods ) {
    if( queue == NULL ) return EINVAL;
    queue->methods = methods;
    if( methods == NULL || methods->init == NULL) {
      PIP_TASKQ_INIT( &queue->queue );
      pip_spin_init( &queue->lock );
      queue->methods = NULL;
      queue->length  = 0;
      queue->aux     = NULL;
      return 0;
    } else {
      return methods->init( queue );
    }
  }
#define pip_task_queue_init( Q, M )				\
  pip_task_queue_init_( (pip_task_queue_t*)(Q), (M) )
#endif

  /**
   * \brief Try locking task queue
   *  @{
   * \param[in] queue A task queue
   *
   * \return Returns true if lock succeeds.
   */
#ifdef DOXYGEN_INPROGRESS
  int pip_task_queue_trylock( pip_task_queue_t *queue );
  /** @}*/
#else
  INLINE int pip_task_queue_trylock_( pip_task_queue_t *queue ) {
    if( queue == NULL ) return EINVAL;
    if( queue->methods == NULL || queue->methods->trylock == NULL ) {
      return pip_spin_trylock( &queue->lock );
    } else {
      return queue->methods->trylock( (void*) queue );
    }
  }
#define pip_task_queue_trylock( Q )			\
  pip_task_queue_trylock_( (pip_task_queue_t*)(Q) )
#endif

  /**
   * \brief Lock task queue
   *  @{
   * \param[in] queue A task queue
   *
   * \return This function returns no error
   */
#ifdef DOXYGEN_INPROGRESS
  void pip_task_queue_lock( pip_task_queue_t *queue );
  /** @}*/
#else
  INLINE void pip_task_queue_lock_( pip_task_queue_t *queue ) {
    if( queue == NULL ) return;
    if( queue->methods == NULL || queue->methods->lock == NULL ) {
      while( !pip_spin_trylock( &queue->lock ) ) {
	(void) pip_pause();
      }
    } else {
      queue->methods->lock( queue );
    }
  }
#define pip_task_queue_lock( Q )			\
  pip_task_queue_lock_( (pip_task_queue_t*)(Q) )
#endif

  /**
   * \brief Unlock task queue
   *  @{
   * \param[in] queue A task queue
   *
   * \return This function returns no error
   */
#ifdef DOXYGEN_INPROGRESS
  void pip_task_queue_unlock( pip_task_queue_t *queue );
  /** @}*/
#else
  INLINE void pip_task_queue_unlock_( pip_task_queue_t *queue ) {
    if( queue == NULL ) return;
    if( queue->methods == NULL || queue->methods->unlock == NULL ) {
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
   * \return Returns true if there is no tasks to in the queue
   */
#ifdef DOXYGEN_INPROGRESS
  int pip_task_queue_isempty( pip_task_queue_t *queue );
  /** @}*/
#else
  INLINE int pip_task_queue_isempty_( pip_task_queue_t *queue ) {
    if( queue == NULL ) return EINVAL;
    if( queue->methods == NULL || queue->methods->isempty == NULL ) {
      return PIP_TASKQ_ISEMPTY( &queue->queue );
    } else {
      return queue->methods->isempty( (void*) queue );
    }
  }
#define pip_task_queue_isempty( Q )			\
  pip_task_queue_isempty_( (pip_task_queue_t*)(Q) )
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
#ifdef DOXYGEN_INPROGRESS
  int pip_task_queue_count( pip_task_queue_t *queue, int *np );
  /** @}*/
#else
  INLINE int
  pip_task_queue_count_( pip_task_queue_t *queue, int *np ) {
    int err = 0;
    if( queue == NULL ) return EINVAL;
    if( np    == NULL ) return EINVAL;
    if( queue->methods == NULL || queue->methods->count == NULL ) {
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
   * \brief Enqueue a BLT
   *  @{
   * \param[in] queue A task queue
   * \param[in] task A task to be enqueued
   *
   */
#ifdef DOXYGEN_INPROGRESS
  void pip_task_queue_enqueue( pip_task_queue_t *queue, pip_task_t *task );
  /** @}*/
#else
  INLINE void
  pip_task_queue_enqueue_( pip_task_queue_t *queue, pip_task_t *task ) {
    if( queue == NULL ) return;
    if( queue->methods == NULL || queue->methods->enqueue == NULL ) {
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
#ifdef DOXYGEN_INPROGRESS
  pip_task_t* pip_task_queue_dequeue( pip_task_queue_t *queue );
  /** @}*/
#else
  INLINE pip_task_t*
  pip_task_queue_dequeue_( pip_task_queue_t *queue ) {
    if( queue == NULL ) return NULL;
    if( queue->methods == NULL || queue->methods->dequeue == NULL ) {
      pip_task_t *first;
      if( PIP_TASKQ_ISEMPTY( &queue->queue ) ) return NULL;
      first = PIP_TASKQ_NEXT( &queue->queue );
      PIP_TASKQ_DEQ( first );
      queue->length --;
      return first;
    } else {
      return queue->methods->dequeue( (void*) queue );
    }
  }
#define pip_task_queue_dequeue( Q )			\
  pip_task_queue_dequeue_( (pip_task_queue_t*) (Q) )
#endif

  /**
   * \brief Describe queue
   *  @{
   * \param[in] queue A task queue
   * \param[in] fp a file pointer
   *
   * \return This function returns no error
   */
#ifdef DOXYGEN_INPROGRESS
  void pip_task_queue_describe( pip_task_queue_t *queue, FILE *fp );
  /** @}*/
#else
  INLINE void
  pip_task_queue_describe_( pip_task_queue_t *queue, char *tag, FILE *fp ) {
    if( queue == NULL ) return;
    if( queue->methods == NULL || queue->methods->describe == NULL ) {
      if( PIP_TASKQ_ISEMPTY( &queue->queue ) ) {
	fprintf( fp, "%s: Queue is (EMPTY)\n", tag );
      } else {
	pip_task_t *task;
	char msg[256];
	int i = 0;
	PIP_TASKQ_FOREACH( &queue->queue, task ) {
	  (void) pip_task_str( msg, sizeof(msg), task );
	  fprintf( fp, "%s: Queue[%d/%d]:%s\n", tag, i++, queue->length, msg );
	}
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
#ifdef DOXYGEN_INPROGRESS
  int pip_task_queue_fin( pip_task_queue_t *queue );
  /** @}*/
#else
  INLINE int pip_task_queue_fin_( pip_task_queue_t *queue ) {
    if( queue == NULL ) return EINVAL;
    if( queue->methods == NULL || queue->methods->fin == NULL ) {
      if( !PIP_TASKQ_ISEMPTY( &queue->queue ) ) return EBUSY;
      return 0;
    } else {
      return queue->methods->fin( (void*) queue );
    }
  }
#define pip_task_queue_fin( Q )			\
  pip_task_queue_fin_( (pip_task_queue_t*)(Q) )
#endif

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
#ifdef DOXYGEN_INPROGRESS
  int pip_suspend_and_enqueue( pip_task_queue_t *queue,
			       pip_enqueue_callback_t callback,
			       void *cbarg );
  /** @}*/
#else
  int pip_suspend_and_enqueue_( pip_task_queue_t *queue,
				pip_enqueue_callback_t callback,
				void *cbarg );
#define pip_suspend_and_enqueue( Q, C, A ) \
  pip_suspend_and_enqueue_( (pip_task_queue_t*)(Q), (C), (A) )
#endif

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
#ifdef DOXYGEN_INPROGRESS
  int pip_suspend_and_enqueue_nolock( pip_task_queue_t *queue,
				      pip_enqueue_callback_t callback,
				      void *cbarg );
  /** @}*/
#else
  int pip_suspend_and_enqueue_nolock_( pip_task_queue_t *queue,
				       pip_enqueue_callback_t callback,
				       void *cbarg );
#define pip_suspend_and_enqueue_nolock( Q, C, A ) \
  pip_suspend_and_enqueue_nolock_( (pip_task_queue_t*)(Q), (C), (A) )
#endif

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
#ifdef DOXYGEN_INPROGRESS
  int pip_dequeue_and_resume( pip_task_queue_t *queue, pip_task_t *sched );
  /** @}*/
#else
  int pip_dequeue_and_resume_( pip_task_queue_t *queue, pip_task_t *sched );
#define pip_dequeue_and_resume( Q, S )		\
  pip_dequeue_and_resume_( (pip_task_queue_t*)(Q), (S) )
#endif

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
#ifdef DOXYGEN_INPROGRESS
  int pip_dequeue_and_resume_nolock( pip_task_queue_t *queue,
				     pip_task_t *sched );
  /** @}*/
#else
  int pip_dequeue_and_resume_nolock_( pip_task_queue_t *queue,
				      pip_task_t *sched );
#define pip_dequeue_and_resume_nolock( Q, S )	\
  pip_dequeue_and_resume_nolock_( (pip_task_queue_t*)(Q), (S) )
#endif

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
#ifdef DOXYGEN_INPROGRESS
  int pip_dequeue_and_resume_N( pip_task_queue_t *queue,
				pip_task_t *sched,
				int *np );
  /** @}*/
#else
  int pip_dequeue_and_resume_N_( pip_task_queue_t *queue,
				 pip_task_t *sched,
				 int *np );
#define pip_dequeue_and_resume_N( Q, S, N )	\
  pip_dequeue_and_resume_N_( (pip_task_queue_t*)(Q), (S), (N) )

#endif

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
#ifdef DOXYGEN_INPROGRESS
  int pip_dequeue_and_resume_N_nolock( pip_task_queue_t *queue,
				       pip_task_t *sched,
				       int *np );
  /** @}*/
#else
  int pip_dequeue_and_resume_N_nolock_( pip_task_queue_t *queue,
					pip_task_t *sched, int *np );
#define pip_dequeue_and_resume_N_nolock( Q, S, N )			\
  pip_dequeue_and_resume_N_nolock_( (pip_task_queue_t*)(Q), (S), (N) )
#endif

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

  /**
   * \brief Return the number of runnable tasks in the current
   * scheduling domain
   *  @{
   *
   * \param[out] countp pointer to the counter value returning
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EINAVL \c countp is \c NULL
   */
  int pip_count_runnable_tasks( int *countp );
  /** @}*/

  /**
   * \brief Return PIPID of a PiP task
   *  @{
   *
   * \param[in] task a PiP task
   * \param[out] pipidp  pointer to the PIPID value returning
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EINAVL \c task is \c NULL
   * \retval EPERM PiP library is not yet initialized or already
   * finalized
   */
  int pip_get_task_pipid( pip_task_t *task, int *pipidp );
  /** @}*/

  /**
   * \brief get PiP task from PiP ID
   *  @{
   * \param[in] pipid PiP ID
   * \param[out] taskp PiP task of the specified PiP ID
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EINVAL \c pipidp is \c NULL
   * \retval EPERM PiP library is not yet initialized or already
   * finalized
   * \retval ENOENT No such PiP task
   */
  int pip_get_task_from_pipid( int pipid, pip_task_t **taskp );
  /** @}*/

  /**
   * \brief Associate user data with a PiP task
   *  @{
   *
   * \param[in] task PiP task. If \c NULL, then the data is associated
   * with the current PiP task
   * \param[in] aux Pointer to the user dat to assocate with
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM PiP library is not yet initialized or already
   * finalized
   */
  int pip_set_aux( pip_task_t *task, void *aux );
  /** @}*/

  /**
   * \brief Retrive the user data associated with a PiP task
   *  @{
   *
   * \param[in] task PiP task. If \c NULL, then the data is associated
   * with the current PiP task
   * \param[out] auxp The pointer to the usder data will be stored
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EINAVL \c domainp is \c NULL or \c auxp is \c NULL
   * \retval EPERM PiP library is not yet initialized or already
   * finalized
   */
  int pip_get_aux( pip_task_t *task, void **auxp );
  /** @}*/

  /**
   * \brief Return the task representing the scheduling domain
   *  @{
   *
   * \param[out] domainp pointer to the domain task  returning
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EINAVL \c domainp is \c NULL
   * \retval EPERM PiP library is not yet initialized or already
   * finalized
   */
  int pip_get_sched_domain( pip_task_t **domainp );
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
#ifdef DOXYGEN_INPROGRESS
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
#ifdef DOXYGEN_INPROGRESS
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
#ifdef DOXYGEN_INPROGRESS
  int pip_barrier_fin( pip_barrier_t *barrp );
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
#ifdef DOXYGEN_INPROGRESS
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
#ifdef DOXYGEN_INPROGRESS
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
#ifdef DOXYGEN_INPROGRESS
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
#ifdef DOXYGEN_INPROGRESS
  int pip_mutex_fin( pip_mutex_t *mutex );
  /** @}*/
#else
  int pip_mutex_fin_( pip_mutex_t *mutex );
#define pip_mutex_fin( M ) \
  pip_mutex_fin_( (pip_mutex_t*)(M) )
#endif

  /**
   * \brief Couple the curren task with the original kernel thread
   *  @{
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM PiP library is not yet initialized or already finalized
   * \retval EBUSY the curren task is already coupled with a kernel thread
   */
  int pip_couple();
  /** @}*/

  /**
   * \brief Decouple the curren task from the kernel thread
   *  @{
   * \param[in] task specify the scheduling task to schedule the decoupled task
   * (calling this function)
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM PiP library is not yet initialized or already finalized
   * \retval EBUSY the curren task is already decoupled from a kernel thread
   */
  int pip_decouple( pip_task_t *task );
  /** @}*/

  int pip_set_syncflag( uint32_t flags );

#ifdef PIP_EXPERIMENTAL
  int pip_migrate( pip_task_t* );
#endif
/**
 * @}
 */

#ifndef DOXYGEN_INPROGRESS
#ifdef __cplusplus
}
#endif
#endif

#endif /* _pip_blt_h_ */
