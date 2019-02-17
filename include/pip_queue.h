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

#ifndef _pip_queue_h_
#define _pip_queue_h_

#include <pip.h>

#define PIP_TASK_QUEUE_SETUP(Q)					\
  do { memset((Q),0,sizeof(pip_task_queue_t));				\
    PIP_LIST_INIT(((pip_task_t*)(Q))->queue);				\
  } while(0)

#define PIP_TASKQ_QUEUE(Q)	(((pip_task_queue_head_t*)(Q))->queue)
#define PIP_TASKQ_METHODS(Q)	(((pip_task_queue_head_t*)(Q))->methods)

#define PIP_ISEMPTY(Q)						\
  ( PIP_TASKQ_METHODS(Q) == NULL ) ?				\
  PIP_TASKQ_ISEMPTY( &PIP_TASKQ_QUEUE(Q) ) :			\
  PIP_TASKQ_METHODS(Q)->is_empty((Q))

#define PIP_ENQUEUE(Q,T)						\
  ( PIP_TASKQ_METHODS(Q) == NULL ) ?					\
  PIP_TASKQ_ENQ_LAST( &PIP_TASKQ_QUEUE(Q), (pip_task_t*)(T) ) :		\
  PIP_TASKQ_METHODS(Q)->enqueue( (Q), (T) )

#define PIP_DEQUEUE(Q,T)					\
  ( PIP_TASKQ_METHODS(Q) == NULL ) ?				\
  PIP_TASKQ_DEQ( &PIP_TASKQ_QUEUE(Q), (pip_task_t*)(T) ) :	\
  PIP_TASKQ_METHODS(Q)->dequeue( (Q), (T) )

#define PIP_DESTROY(Q)				\
  ( PIP_TASKQ_METHODS(Q) == NULL ) ?			 \
  PIP_TASKQ_ISEMPTY( &PIP_TASKQ_QUEUE(Q) ) ? 0 : EBUSY	 \
  PIP_TASKQ_MEHODS->destroy( (Q) )

#define PIP_LOCKED_QUEUE_SETUP(Q)				\
  do { PIP_TASK_QUEUE_SETUP(Q);					\
    pip_spin_init( &((pip_locked_queue_t*)(Q))->lock );		\
  } while(0)

#define PIP_LOCK_LOCKED_QUEUE(Q)		\
  pip_spin_lock( &((pip_locked_queue_t*)(Q))->lock )

#define PIP_UNLOCK_LOCKED_QUEUE(Q)		\
  pip_spin_unlock( &((pip_locked_queue_t*)(Q))->lock )

#endif /* _pip_queue_h */
