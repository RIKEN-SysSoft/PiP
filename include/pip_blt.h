/*
  * $RIKEN_copyright: Riken Center for Computational Sceience,
  * System Software Development Team, 2016, 2017, 2018, 2019, 2020$
  * $PIP_VERSION: Version 3.0.0$
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

#ifndef DOXYGEN_INPROGRESS

#include <pip_machdep.h>
#include <string.h>

#define PIP_SYNC_AUTO			(0x0001U)
#define PIP_SYNC_BUSYWAIT		(0x0002U)
#define PIP_SYNC_YIELD			(0x0004U)
#define PIP_SYNC_BLOCKING		(0x0008U)

#define PIP_SYNC_MASK			(PIP_SYNC_AUTO     |	\
					 PIP_SYNC_BUSYWAIT |	\
					 PIP_SYNC_YIELD    |	\
					 PIP_SYNC_BLOCKING)

#define PIP_TASK_INACTIVE		(0x01000U)
#define PIP_TASK_ACTIVE			(0x02000U)

#define PIP_TASK_MASK				\
  (PIP_TASK_INACTIVE|PIP_TASK_ACTIVE)

#define PIP_TASK_ALL			(-1)

#define PIP_YIELD_DEFAULT		(0x0U)
#define PIP_YIELD_USER			(0x1U)
#define PIP_YIELD_SYSTEM		(0x2U)

#define PIP_ENV_SYNC			"PIP_SYNC"
#define PIP_ENV_SYNC_AUTO		"auto"
#define PIP_ENV_SYNC_BUSY		"busy"
#define PIP_ENV_SYNC_BUSYWAIT		"busywait"
#define PIP_ENV_SYNC_YIELD		"yield"
#define PIP_ENV_SYNC_BLOCK		"block"
#define PIP_ENV_SYNC_BLOCKING		"blocking"

typedef struct pip_task {
  struct pip_task	*next;
  struct pip_task	*prev;
} pip_task_t;

#define PIP_TASKQ_NEXT(L)	(((pip_task_t*)(L))->next)
#define PIP_TASKQ_PREV(L)	(((pip_task_t*)(L))->prev)
#define PIP_TASKQ_PREV_NEXT(L)	(((pip_task_t*)(L))->prev->next)
#define PIP_TASKQ_NEXT_PREV(L)	(((pip_task_t*)(L))->next->prev)

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

#endif /* DOXYGEN */

#ifdef DOXYGEN_INPROGRESS
#ifndef INLINE
#define INLINE
#endif
#else
#ifndef INLINE
#define INLINE			inline static
#endif
#endif

#ifndef DOXYGEN_INPROGRESS
#ifdef __cplusplus
extern "C" {
#endif
#endif

  /**
   * \addtogroup PiP-1-spawn
   * @{
   */

  /**
   * \PiPManEntry{pip_blt_spawn}
   *
   * \brief spawn a PiP BLT/ULP (Bi-Level Task / User-Level Process)
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_blt_spawn( pip_spawn_program_t *progp,
   *		   uint32_t coreno,
   *		   uint32_t opts,
   *		   int *pipidp,
   *		   pip_task_t **bltp,
   *		   pip_task_queue_t *queue,
   *		   pip_spawn_hook_t *hookp );
   *
   * \description
   * This function spawns a BLT (PiP task) specified by \c progp. The
   * created annd returned BLT is another form of a PiP task. It is an
   * opaque object, essentially a double-linked list. Thus created BLT
   * can be enqueued or dequeued to/from a \p pip_task_queue_t.
   * \par
   * In the process execution mode, the file descriptors having the
   * \c FD_CLOEXEC flag is closed and will not be passed to the spawned
   * PiP task. This simulated close-on-exec will not take place in the
   * pthread execution mode.
   *
   * \param[out] progp \b pip_spawn_program_t
   * \param[in] coreno CPU core number for the PiP task to be bound to. By
   *  default, \p coreno is set to zero, for example, then the calling
   *  task will be bound to the first core available. This is in mind
   *  that the available core numbers are not contiguous. To specify
   *  an absolute core number, \p coreno must be bitwise-ORed with
   *  \p PIP_CPUCORE_ABS. If
   *  \p PIP_CPUCORE_ASIS is specified, then the core binding will not
   *  take place.
   * \param[in] opts option flags. If \p PIP_TASK_INACTIVE is set, the
   *  created BLT is suspended and enqueued to the specified
   *  \p queue. Otherwise the BLT will schedules the BLTs in \p queue.
   * \param[in,out] pipidp Specify PiP ID of the spawned PiP task. If
   *  \c PIP_PIPID_ANY is specified, then the PiP ID of the spawned PiP
   *  task is up to the PiP library and the assigned PiP ID will be
   *  returned. The PiP execution mode can also be specified (see below).
   * \param[in,out] bltp returns created BLT
   * \param[in] queue PiP task queue. See the above \p opts description.
   * \param[in] hookp Hook information to be invoked before and after
   *  the program invokation.
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM PiP library is not yet initialized
   * \retval EPERM PiP task tries to spawn child task
   * \retval EINVAL \c progp is \c NULL
   * \retval EINVAL \c opts is invalid and/or unacceptable
   * \retval EINVAL the value off \c pipidp is invalid
   * \retval EBUSY specified PiP ID is alredy occupied
   * \retval ENOMEM not enough memory
   * \retval ENXIO \c dlmopen failss
   *
   * @par Execution mode option
   * Users may explicitly specify the PiP execution mode.
   * This execution mode can be categorized in two; process mode and
   * thread mode. In the process execution mode, each PiP task may
   * have its own file descriptors, signal handlers, and so on, just
   * like a process. Contrastingly, in the pthread executionn mode, file
   * descriptors and signal handlers are shared among PiP root and PiP
   * tasks while maintaining the privatized variables.
   * \par
   * To spawn a PiP task in the process mode, the PiP library modifies
   * the \b clone() flag so that the created PiP task can exhibit the
   * alomost same way with that of normal Linux process. There are
   * three ways implmented; using LD_PRELOAD, modifying GLIBC, and
   * modifying GIOT entry of the \b clone() systemcall. One of the
   * option flag values; \b PIP_MODE_PTHREAD, \b PIP_MODE_PROCESS,
   * \b PIP_MODE_PROCESS_PRELOAD, \b PIP_MODE_PROCESS_PIPCLONE, or
   * b PIP_MODE_PROCESS_GOT can be specified as the option flag. Or,
   * users may specify the execution mode by the PIP_MODE environment
   * described below.
   *
   * \note In theory, there is no reason to restrict for a PiP task to
   * spawn another PiP task. However, the current implementation fails
   * to do so. If the root process is multithreaded, only the main
   * thread can call this function.
   *
   * \environment
   * \arg \b PIP_MODE Specifying the PiP execution mode. The value can be one of;
   * 'process', 'process:preload', 'process:got' and 'thread' (or 'pthread').
   * \arg \b PIP_STACKSZ Sepcifying the stack size (in bytes). The
   * \b KMP_STACKSIZE and \b OMP_STACKSIZE can also be specified. The 't',
   * 'g', 'm', 'k' and 'b' posfix character can be used.
   * \arg \b PIP_STOP_ON_START Specifying the PIP ID to stop on start
   * PiP task program to debug from the beginning. If the
   * before hook is specified, then the PiP task will be stopped just
   * before calling the before hook.
   * \arg \b PIP_STACKSZ Sepcifying the stack size (in bytes). The
   * \b KMP_STACKSIZE and \b OMP_STACKSIZE can also be specified. The 't',
   * 'g', 'm', 'k' and 'b' posfix character can be used.
   *
   * \bugs
   * In theory, there is no reason to restrict for a PiP task to
   * spawn another PiP task. However, the current glibc implementation
   * does not allow to do so.
   * \par
   * If the root process is multithreaded, only the main
   * thread can call this function.
   *
   * \sa pip_task_spawn
   * \sa pip_spawn_from_main
   * \sa pip_spawn_from_func
   * \sa pip_spawn_hook
   * \sa pip_task_spawn
   * \sa pip_spawn
   */
#ifdef DOXYGEN_INPROGRESS
int pip_blt_spawn( pip_spawn_program_t *progp,
		   uint32_t coreno,
		   uint32_t opts,
		   int *pipidp,
		   pip_task_t **bltp,
		   pip_task_queue_t *queue,
		   pip_spawn_hook_t *hookp );
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
   * @}
   */

  /**
   * \defgroup ULP-0-yield Yielding Functionns
   * @{
   * \page pip-yield Yielding functions
   * \description Yielding execution of the calling BLT/ULP
   */

  /**
   * \PiPManEntry{pip_yield}
   *
   * \brief Yield
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_yield( int flag );
   *
   * \param[in] flag to specify the behavior of yielding. See below.
   *
   * \return No context-switch takes place during the call, then this
   * returns zero. If the context-switch to the other BLT happens,
   * then this returns \p EINTR.
   *
   * \param flag If \b PIP_YIELD_USER, the calling task is scheduling PiP
   * task(s) then the calling task switch to the next eligible-to-run
   * BLT. If \b PIP_YIELD_SYSTEM, regardless if the calling task is active
   * or inactive, it calls \p sched_yield.
   * If \b PIP_YIELD_DEFAULT or zero, then both \b PIP_YIELD_USER and
   * \b PIP_YIELD_SYSTEM will be effective.
   *
   * \sa pip_yield_to
   */
  int pip_yield( int flag );

  /**
   * \PiPManEntry{pip_yield_to}
   *
   * \brief Yield to the specified PiP task
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_yield( pip_task_t *task );
   *
   * \description
   * Context-switch to the specified PiP task. If \c task is \c NULL, then
   * this works the same as what \c pip_yield(3) does with
   * \p PIP_YIELD_DEFAULT.
   *
   * \param[in] task Target PiP task to switch.
   *
   * \return Return \p Zero or EINTR on success. Return an error code
   * on error.
   * \retval EPERM PiP library is not yet initialized or already
   * \retval EPERM The specified task belongs to the other scheduling
   * domain.
   *
   * \sa pip_yield
   */
  int pip_yield_to( pip_task_t *task );

  /**
   * @}
   */

  /**
   * \defgroup ULP-1-task-queue Task Queue Operations
   * @{
   * \page ulp-task-queue Task queue operations
   * \description Manipulating ULP/BLT task queue functions
   */

  /**
   * \PiPManEntry{pip_task_queue_init}
   *
   * \brief Initialize task queue
   *
   * \synopsis
   * \#include <pip.h> \n
   *  int pip_task_queue_init( pip_task_queue_t *queue,
   *			   pip_task_queue_methods_t *methods );
   *
   * \param[in] queue A task queue
   * \param[in] methods Must be set to \p NULL. Researved for future use.
   *
   * \return Always return 0.
   */
#ifdef DOXYGEN_INPROGRESS
  int pip_task_queue_init( pip_task_queue_t *queue,
			   pip_task_queue_methods_t *methods );
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
   * \PiPManEntry{pip_task_queue_trylock}
   *
   * \brief Try locking task queue
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_task_queue_trylock( pip_task_queue_t *queue );
   *
   * \param[in] queue A task queue
   *
   * \return Returns a non-zero value if lock succeeds.
   */
#ifdef DOXYGEN_INPROGRESS
  int pip_task_queue_trylock( pip_task_queue_t *queue );
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
   * \PiPManEntry{pip_task_queue_lock}
   *
   * \brief Lock task queue
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_task_queue_lock( pip_task_queue_t *queue );
   *
   * \param[in] queue A task queue
   *
   * \return This function returns no error
   */
#ifdef DOXYGEN_INPROGRESS
  void pip_task_queue_lock( pip_task_queue_t *queue );
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
   * \PiPManEntry{pip_task_queue_unlock}
   *
   * \brief Unlock task queue
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_task_queue_unlock( pip_task_queue_t *queue );
   *
   * \param[in] queue A task queue
   *
   * \return This function returns no error
   */
#ifdef DOXYGEN_INPROGRESS
  void pip_task_queue_unlock( pip_task_queue_t *queue );
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
   * \PiPManEntry{pip_task_queue_isempty}
   *
   * \brief Query function if the current task has some tasks to be
   *  scheduled with.
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_task_queue_isempty( pip_task_queue_t *queue );
   *
   * \param[in] queue A task queue
   *
   * \return Returns a non-zero value if the queue is empty
   */
#ifdef DOXYGEN_INPROGRESS
  int pip_task_queue_isempty( pip_task_queue_t *queue );
#else
  INLINE int pip_task_queue_isempty_( pip_task_queue_t *queue ) {
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
   * \PiPManEntry{pip_task_queue_count}
   *
   * \brief Count the length of task queue
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_task_queue_count( pip_task_queue_t *queue, int *np );
   *
   * \param[in] queue A task queue
   * \param[out] np the queue length returned
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EINVAL \c queue is \c NULL
   * \retval EINVAL \c np is \c NULL
   */
#ifdef DOXYGEN_INPROGRESS
  int pip_task_queue_count( pip_task_queue_t *queue, int *np );
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
   * \PiPManEntry{pip_task_queue_enqueue}
   *
   * \brief Enqueue a BLT
   *
   * \synopsis
   * \#include <pip.h> \n
   * void pip_task_queue_enqueue( pip_task_queue_t *queue,
   * pip_task_t *task );
   *
   * \param[in] queue A task queue
   * \param[in] task A task to be enqueued
   *
   * \note
   * It is the user responsibility to lock (and unlock) the queue.
   */
#ifdef DOXYGEN_INPROGRESS
  void pip_task_queue_enqueue( pip_task_queue_t *queue, pip_task_t *task );
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
   * \PiPManEntry{pip_task_queue_dequeue}
   *
   * \brief Dequeue a task from a task queue
   *
   * \synopsis
   * \#include <pip.h> \n
   * pip_task_t* pip_task_queue_dequeue( pip_task_queue_t *queue );
   *
   * \param[in] queue A task queue
   *
   * \return Dequeued task iss returned. If the queue is empty
   * then \p NULL is returned.
   *
   * \note
   * It is the user responsibility to lock (and unlock) the queue.
   */
#ifdef DOXYGEN_INPROGRESS
  pip_task_t* pip_task_queue_dequeue( pip_task_queue_t *queue );
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
   * \PiPManEntry{pip_task_queue_describe}
   *
   * \brief Describe queue
   *
   * \synopsis
   * \#include <pip.h> \n
   * void pip_task_queue_describe( pip_task_queue_t *queue, FILE *fp );
   *
   * \param[in] queue A task queue
   * \param[in] fp a File pointer
   */
#ifdef DOXYGEN_INPROGRESS
  void pip_task_queue_describe( pip_task_queue_t *queue, FILE *fp );
#else
  INLINE void
  pip_task_queue_describe_( pip_task_queue_t *queue, char *tag, FILE *fp ) {
    extern int pip_task_str( char*, size_t, pip_task_t* );
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
   * \PiPManEntry{pip_task_queue_fin}
   *
   * \brief Finalize a task queue
   *
   * \synopsis
   * \#include <pip.h> \n
   *  int pip_task_queue_fin( pip_task_queue_t *queue );
   *
   * \param[in] queue A task queue
   *
   * \return Zero is returned always
   */
#ifdef DOXYGEN_INPROGRESS
  int pip_task_queue_fin( pip_task_queue_t *queue );
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
   * @}
   */

  /**
   * \defgroup ULP-2-suspension Suspending and Resuming BLT/ULP
   * @{
   * \page ulp-suspension Suspending and resuming BLT/ULP
   * \description Suspending and resuming BLT/ULP
   */

  /**
   * \PiPManEntry{pip_suspend_and_enqueue}
   *
   * \brief suspend the curren task and enqueue it with lock
   *
   * \synopsis
   * \#include <pip.h> \n
   *  int pip_suspend_and_enqueue( pip_task_queue_t *queue,
   *			       pip_enqueue_callback_t callback,
   *			       void *cbarg );
   *
   * \description
   * The \b queue is locked just before the calling task is enqueued and
   * unlocked after the calling task is
   * enqueued. After then the \b callback function is called.
   * \par
   * As the result of this suspension, a context-switch takes place if
   * there is at least one elgible-to-run task in the scheduling queue
   * (this is hidden from users). If there is no other task to schedule
   * then the kernel thread of the current task will be blocked.
   *
   * \param[in] queue A task queue
   * \param[in] callback A callback function which is called
   * immediately after the task is enqueued
   * \param[in] cbarg An argument given to the callback function
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM  PiP library is not initialized yet
   * \retval EINVAL \p queue is \p NULL
   *
   * \sa pip_enqueue_and_suspend_nolock
   * \sa pip_dequeue_and_resume
   */
#ifdef DOXYGEN_INPROGRESS
  int pip_suspend_and_enqueue( pip_task_queue_t *queue,
			       pip_enqueue_callback_t callback,
			       void *cbarg );
#else
  int pip_suspend_and_enqueue_( pip_task_queue_t *queue,
				pip_enqueue_callback_t callback,
				void *cbarg );
#define pip_suspend_and_enqueue( Q, C, A ) \
  pip_suspend_and_enqueue_( (pip_task_queue_t*)(Q), (C), (A) )
#endif

  /**
   * \PiPManEntry{pip_suspend_and_enqueue_nolock}
   *
   * \brief suspend the curren task and enqueue it without locking the queue
   *
   * \synopsis
   * \#include <pip.h> \n
   *  int pip_suspend_and_enqueue_nolock( pip_task_queue_t *queue,
   *			       pip_enqueue_callback_t callback,
   *			       void *cbarg );
   *
   * \description
   * Unlike \p pip_suspend_and_enqueue, this function never locks the queue.
   * It is the user's responsibility to lock the queue beofre calling
   * this function and unlock the queue after calling this
   * function. The \b callback function can be used for unlocking.
   * \par
   * As the result of this suspension, a context-switch takes place if
   * there is at least one elgible-to-run task in the scheduling queue
   * (this is hidden from users). If there is no other task to schedule
   * then the kernel thread of the current task will be blocked.
   *
   * \param[in] queue A task queue
   * \param[in] callback A callback function which is called when enqueued
   * \param[in] cbarg An argument given to the callback function
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM  PiP library is not initialized yet
   * \retval EINVAL \p queue is \p NULL
   */
#ifdef DOXYGEN_INPROGRESS
  int pip_suspend_and_enqueue_nolock( pip_task_queue_t *queue,
				      pip_enqueue_callback_t callback,
				      void *cbarg );
#else
  int pip_suspend_and_enqueue_nolock_( pip_task_queue_t *queue,
				       pip_enqueue_callback_t callback,
				       void *cbarg );
#define pip_suspend_and_enqueue_nolock( Q, C, A ) \
  pip_suspend_and_enqueue_nolock_( (pip_task_queue_t*)(Q), (C), (A) )
#endif

  /**
   * \PiPManEntry{pip_dequeue_and_resume}
   *
   * \brief dequeue a task and make it runnable
   *
   * \description
   * The \b queue is locked and then unlocked when to dequeued a task.
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_dequeue_and_resume( pip_task_queue_t *queue, pip_task_t *sched );
   *
   * \param[in] queue A task queue
   * \param[in] sched A task to specify a scheduling domain
   *
   * \return If succeedss, 0 is returned. Otherwise an error code is
   * returned.
   * \retval EPERM  PiP library is not initialized yet
   * \retval EINVAL \p queue is \p NULL
   * \retval ENOENT \p queue is empty.
   */
#ifdef DOXYGEN_INPROGRESS
  int pip_dequeue_and_resume( pip_task_queue_t *queue, pip_task_t *sched );
#else
  int pip_dequeue_and_resume_( pip_task_queue_t *queue, pip_task_t *sched );
#define pip_dequeue_and_resume( Q, S )		\
  pip_dequeue_and_resume_( (pip_task_queue_t*)(Q), (S) )
#endif

  /**
   * \PiPManEntry{pip_dequeue_and_resume_nolock}
   *
   * \brief dequeue a task and make it runnable
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_dequeue_and_resume( pip_task_queue_t *queue, pip_task_t *sched );
   *
   * \description
   * Task in the queue is dequeued and
   * scheduled by the specified \p sched. If
   * \p sched is NULL, then the task is enqueued into the scheduling
   * queue of calling task.
   * \par
   * It is the user's responsibility to lock the queue beofre calling
   * this function and unlock the queue after calling this
   * function.
   *
   * \param[in] queue A task queue
   * \param[in] sched A task to specify a scheduling domain
   *
   * \return This function returns no error
   * \retval EPERM  PiP library is not initialized yet
   * \retval EINVAL \p queue is \p NULL
   * \retval ENOENT \p queue is empty.
   */
#ifdef DOXYGEN_INPROGRESS
  int pip_dequeue_and_resume_nolock( pip_task_queue_t *queue,
				     pip_task_t *sched );
#else
  int pip_dequeue_and_resume_nolock_( pip_task_queue_t *queue,
				      pip_task_t *sched );
#define pip_dequeue_and_resume_nolock( Q, S )	\
  pip_dequeue_and_resume_nolock_( (pip_task_queue_t*)(Q), (S) )
#endif

  /**
   * \PiPManEntry{pip_dequeue_and_resume_N}
   *
   * \brief dequeue multiple tasks and resume the execution of them
   *
   * \synopsis
   * \#include <pip.h> \n
   *  int pip_dequeue_and_resume_N( pip_task_queue_t *queue,
   *				pip_task_t *sched,
   *				int *np );
   *
   * \description
   * The specified number of tasks are dequeued and scheduled by the
   * specified \p sched. If \p sched is NULL, then the task is
   * enqueued into the scheduling queue of calling task.
   * \par
   * The \b queue is locked and unlocked when dequeued.
   *
   * \param[in] queue A task queue
   * \param[in] sched A task to specify a scheduling domain
   * \param[in,out] np A pointer to an interger which spcifies the
   *  number of tasks dequeued and actual number of tasks dequeued is
   *  returned. When \p PIP_TASK_ALL is specified, then all tasks in
   * the queue will be resumed.
   *
   * \return This function returns no error
   * \retval EPERM  PiP library is not initialized yet
   * \retval EINVAL \p queue is \p NULL
   * \retval EINVAL the specified number of tasks is invalid
   * \retval ENOENT \p queue is empty.
   *
   * It is the user's responsibility to lock the queue beofre calling
   * this function and unlock the queue after calling this
   * function.
   */
#ifdef DOXYGEN_INPROGRESS
  int pip_dequeue_and_resume_N( pip_task_queue_t *queue,
				pip_task_t *sched,
				int *np );
#else
  int pip_dequeue_and_resume_N_( pip_task_queue_t *queue,
				 pip_task_t *sched,
				 int *np );
#define pip_dequeue_and_resume_N( Q, S, N )	\
  pip_dequeue_and_resume_N_( (pip_task_queue_t*)(Q), (S), (N) )

#endif

  /**
   * \PiPManEntry{pip_dequeue_and_resume_N_nolock}
   *
   * \brief dequeue tasks and resume the execution of them
   *
   * \synopsis
   * \#include <pip.h> \n
   *  int pip_dequeue_and_resume_N_nolock( pip_task_queue_t *queue,
   *				pip_task_t *sched, int *np );
   *
   * \description
   * The specified number of tasks are dequeued and scheduled by the
   * specified \p sched. If \p sched is NULL, then the task is
   * enqueued into the scheduling queue of calling task.
   * \par
   * It is the user's responsibility to lock the queue beofre calling
   * this function and unlock the queue after calling this
   * function.
   *
   * \param[in] queue A task queue
   * \param[in] sched A task to specify a scheduling domain
   * \param[in,out] np A pointer to an interger which spcifies the
   *  number of tasks dequeued and actual number of tasks dequeued is
   *  returned. When \p PIP_TASK_ALL is specified, then all tasks in
   * the queue will be resumed.
   *
   * \return This function returns no error
   * \retval EPERM  PiP library is not initialized yet
   * \retval EINVAL \p queue is \p NULL
   * \retval EINVAL the specified number of tasks is invalid
   * \retval ENOENT \p queue is empty.
   */
#ifdef DOXYGEN_INPROGRESS
  int pip_dequeue_and_resume_N_nolock( pip_task_queue_t *queue,
				       pip_task_t *sched,
				       int *np );
#else
  int pip_dequeue_and_resume_N_nolock_( pip_task_queue_t *queue,
					pip_task_t *sched, int *np );
#define pip_dequeue_and_resume_N_nolock( Q, S, N )			\
  pip_dequeue_and_resume_N_nolock_( (pip_task_queue_t*)(Q), (S), (N) )
#endif

  /**
   * @}
   */

  /**
   * \defgroup ULP-3-misc BLT/ULP Miscellaneous Function
   * @{
   * \page ulp-misc BLT/ULP miscellaneous function
   * \description BLT/ULP miscellaneous function
   */

  /**
   * \PiPManEntry{pip_task_self}
   *
   * \brief Return the current task
   *
   * \synopsis
   * \#include <pip.h> \n
   * pip_task_t *pip_task_self( void );
   *
   * \return Return the current task.
   *
   */
  pip_task_t *pip_task_self( void );

  /**
   * \PiPManEntry{pip_get_task_pipid}
   *
   * \brief Return PIPID of a PiP task
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_get_task_pipid( pip_task_t *task, int *pipidp );
   *
   * \param[in] task a PiP task
   * \param[out] pipidp PiP ID of the specified task
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EINAVL \c task is \c NULL
   * \retval EPERM PiP library is not yet initialized or already
   * finalized
   */
  int pip_get_task_pipid( pip_task_t *task, int *pipidp );

  /**
   * \PiPManEntry{pip_get_task_by_pipid}
   *
   * \brief get PiP task from PiP ID
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_get_task_by_pipid( int pipid, pip_task_t **taskp );
   *
   * \param[in] pipid PiP ID
   * \param[out] taskp returning PiP task of the specified PiP ID
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM PiP library is not yet initialized or already
   * finalized
   * \retval ENOENT No such PiP task
   * \retval ERANGE The specified \c pipid is out of ramge
   */
  int pip_get_task_by_pipid( int pipid, pip_task_t **taskp );

  /**
   * \PiPManEntry{pip_set_aux}
   *
   * \brief Associate user data with a PiP task
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_set_aux( pip_task_t *task, void *aux );
   *
   * \param[in] task PiP task. If \c NULL, then the data is associated
   * with the current PiP task
   * \param[in] aux Pointer to the user dat to assocate with
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM PiP library is not yet initialized or already
   * finalized
   *
   * \sa pip_get_aux
   */
  int pip_set_aux( pip_task_t *task, void *aux );

  /**
   * \PiPManEntry{pip_get_aux}
   *
   * \brief Retrieve the user data associated with a PiP task
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_get_aux( pip_task_t *task, void **auxp );
   *
   * \param[in] task PiP task. If \c NULL, then the data is associated
   * with the current PiP task
   * \param[out] auxp Returned user data
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EINAVL \c domainp is \c NULL or \c auxp is \c NULL
   * \retval EPERM PiP library is not yet initialized or already
   * finalized
   *
   * \sa pip_set_aux
   */
  int pip_get_aux( pip_task_t *task, void **auxp );

  /**
   * \PiPManEntry{pip_get_sched_domain}
   *
   * \brief Return the task representing the scheduling domain
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_get_sched_domain( pip_task_t **domainp );
   *
   * \param[out] domainp Returned scheduling domain of the current task
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM PiP library is not yet initialized or already
   * finalized
   */
  int pip_get_sched_domain( pip_task_t **domainp );
  /**
   * @}
   */

  /**
   * \defgroup ULP-4-barrier BLT/ULP Barrier Functions
   * @{
   * \page ulp-barrier BLT/ULP barrier synchronization functions
   * \description BLT/ULP barrier synchronization functions
   */

  /**
   * \PiPManEntry{pip_barrier_init}
   *
   * \brief initialize barrier synchronization structure
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_barrier_init( pip_barrier_t *barrp, int n );
   *
   * \param[in] barrp pointer to a PiP barrier structure
   * \param[in] n number of participants of this barrier
   * synchronization
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM PiP library is not yet initialized or already
   * finalized
   * \retval EINAVL \c n is invalid
   *
   * \note This barrier works on PiP tasks only.
   *
   * \sa pip_barrier_init
   * \sa pip_barrier_fin
   */
#ifdef DOXYGEN_INPROGRESS
  int pip_barrier_init( pip_barrier_t *barrp, int n );
#else
  int pip_barrier_init_( pip_barrier_t *barrp, int n );
#define pip_barrier_init( B, N ) \
  pip_barrier_init_( (pip_barrier_t*)(B), (N) )
#endif

  /**
   * \PiPManEntry{pip_barrier_wait}
   *
   * \brief wait on barrier synchronization in a busy-wait way
   * int pip_barrier_wait( pip_barrier_t *barrp );
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_barrier_wait( pip_barrier_t *barrp );
   *
   * \param[in] barrp pointer to a PiP barrier structure
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM PiP library is not yet initialized or already
   * finalized
   *
   * \sa pip_barrier_init
   * \sa pip_barrier_fin
   *
   */
#ifdef DOXYGEN_INPROGRESS
  int pip_barrier_wait( pip_barrier_t *barrp );
#else
  int pip_barrier_wait_( pip_barrier_t *barrp );
#define pip_barrier_wait( B ) \
  pip_barrier_wait_( (pip_barrier_t*)(B) )
#endif

  /**
   * \PiPManEntry{pip_barrier_fin}
   *
   * \brief finalize barrier synchronization structure
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_barrier_fin( pip_barrier_t *barrp );
   *
   * \param[in] barrp pointer to a PiP barrier structure
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM PiP library is not yet initialized or already
   * finalized
   * \retval EBUSY there are some tasks wating for barrier
   * synchronization
   *
   * \sa pip_barrier_init
   * \sa pip_barrier_wait
   */
#ifdef DOXYGEN_INPROGRESS
  int pip_barrier_fin( pip_barrier_t *barrp );
#else
  int pip_barrier_fin_( pip_barrier_t *queue );
#define pip_barrier_fin( B ) \
  pip_barrier_fin_( (pip_barrier_t*)(B) )
#endif

  /**
   * @}
   */

  /**
   * \defgroup ULP-5-mutex BLT/ULP Mutex Functions
   * @{
   * \page ulp-barrier BLT/ULP mutex functions
   * \description BLT/ULP mutex  functions
   */

  /**
   * \PiPManEntry{pip_mutex_init}
   *
   * \brief Initialize PiP mutex
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_mutex_init( pip_mutex_t *mutex );
   *
   * \param[in,out] mutex pointer to the PiP task mutex
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM PiP library is not yet initialized or already
   * finalized
   *
   * \sa pip_mutex_lock
   * \sa pip_mutex_unlock
   * \sa pip_mutex_fin
   */
#ifdef DOXYGEN_INPROGRESS
  int pip_mutex_init( pip_mutex_t *mutex );
#else
  int pip_mutex_init_( pip_mutex_t *mutex );
#define pip_mutex_init( M ) \
  pip_mutex_init_( (pip_mutex_t*)(M) )
#endif

  /**
   * \PiPManEntry{pip_mutex_lock}
   *
   * \brief Lock PiP mutex
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_mutex_lock( pip_mutex_t *mutex );
   *
   * \param[in] mutex pointer to the PiP task mutex
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM PiP library is not yet initialized or already
   * finalized
   *
   * \sa pip_mutex_init
   * \sa pip_mutex_unlock
   * \sa pip_mutex_fin
   */
#ifdef DOXYGEN_INPROGRESS
  int pip_mutex_lock( pip_mutex_t *mutex );
#else
  int pip_mutex_lock_( pip_mutex_t *mutex );
#define pip_mutex_lock( M ) \
  pip_mutex_lock_( (pip_mutex_t*)(M) )
#endif

  /**
   * \PiPManEntry{pip_mutex_unlock}
   *
   * \brief Unlock PiP mutex
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_mutex_unlock( pip_mutex_t *mutex );
   *
   * \param[in] mutex pointer to the PiP task mutex
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM PiP library is not yet initialized or already
   * finalized
   *
   * \sa pip_mutex_init
   * \sa pip_mutex_lock
   * \sa pip_mutex_fin
   */
#ifdef DOXYGEN_INPROGRESS
  int pip_mutex_unlock( pip_mutex_t *mutex );
#else
  int pip_mutex_unlock_( pip_mutex_t *mutex );
#define pip_mutex_unlock( M ) \
  pip_mutex_unlock_( (pip_mutex_t*)(M) )
#endif

  /**
   * \PiPManEntry{pip_mutex_fin}
   *
   * \brief Finalize PiP mutex
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_mutex_fin( pip_mutex_t *mutex );
   *
   * \param[in,out] mutex pointer to the PiP task mutex
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM PiP library is not yet initialized or already
   * finalized
   * \retval EBUSY There is one or more waiting PiP task
   *
   * \sa pip_mutex_lock
   * \sa pip_mutex_unlock
   */
#ifdef DOXYGEN_INPROGRESS
  int pip_mutex_fin( pip_mutex_t *mutex );
#else
  int pip_mutex_fin_( pip_mutex_t *mutex );
#define pip_mutex_fin( M ) \
  pip_mutex_fin_( (pip_mutex_t*)(M) )
#endif

  /**
   * @}
   */

  /**
   * \defgroup ULP-6-coupling BLT/ULP Coupling/Decoupling Functions
   * @{
   * \page ulp-coupling BLT/ULP coupling/decoupling functions
   * \description BLT/ULP coupling/decoupling functions
   */

  /**
   * \PiPManEntry{pip_couple}
   *
   * \brief Couple the curren task with the original kernel thread
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_couple( void );
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM PiP library is not yet initialized or already finalized
   * \retval EBUSY the curren task is already coupled with a kernel thread
   */
  int pip_couple( void );

  /**
   * \PiPManEntry{pip_decouple}
   *
   * \brief Decouple the curren task from the kernel thread
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_decouple( pip_task_t *sched )
   *
   * \param[in] task specify the scheduling task to schedule the decoupled task
   * (calling this function). If \c NULL, then the previously coupled
   * pip_task takes place.
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM PiP library is not yet initialized or already finalized
   * \retval EBUSY the curren task is already decoupled from a kernel thread
   */
  int pip_decouple( pip_task_t *task );

  /**
   * @}
   */

#ifndef DOXYGEN_INPROGRESS
#ifdef __cplusplus
}
#endif
#endif

#endif /* _pip_blt_h_ */
