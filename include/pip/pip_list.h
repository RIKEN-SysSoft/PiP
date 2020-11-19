/*
 * $PIP_license: <Simplified BSD License>
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *     Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 * 
 *     Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 * $
 * $RIKEN_copyright: Riken Center for Computational Sceience (R-CCS),
 * System Software Development Team, 2016-2020
 * $
 * $PIP_VERSION: Version 2.0.0$
 *
 * $Author: Atsushi Hori (R-CCS) mailto: ahori@riken.jp or ahori@me.com
 * $
 */

#ifndef _pip_list_h_
#define _pip_list_h_

typedef struct pip_list {
  struct pip_list	*next;
  struct pip_list	*prev;
} pip_list_t;

#define PIP_LIST_NEXT(L)	(((pip_list_t*)(L))->next)
#define PIP_LIST_PREV(L)	(((pip_list_t*)(L))->prev)
#define PIP_LIST_PREV_NEXT(L)	(((pip_list_t*)(L))->prev->next)
#define PIP_LIST_NEXT_PREV(L)	(((pip_list_t*)(L))->next->prev)

#define PIP_LIST_INIT(L)					\
  do { PIP_LIST_NEXT(L) = PIP_LIST_PREV(L) =			\
      (pip_list_t*)(L); } while(0)

#define PIP_LIST_ENQ_FIRST(L,E)				\
  do { PIP_LIST_NEXT(E)      = PIP_LIST_NEXT(L);		\
       PIP_LIST_PREV(E)      = (pip_taskt*)(L);		\
       PIP_LIST_NEXT_PREV(L) = (pip_list_t*)(E);		\
       PIP_LIST_NEXT(L)      = (pip_list_t*)(E); } while(0)

#define PIP_LIST_ADD(L,E)					\
  do { PIP_LIST_NEXT(E)      = (pip_list_t*)(L);		\
       PIP_LIST_PREV(E)      = PIP_LIST_PREV(L);		\
       PIP_LIST_PREV_NEXT(L) = (pip_list_t*)(E);		\
       PIP_LIST_PREV(L)      = (pip_list_t*)(E); } while(0)

#define PIP_LIST_DEL(E)					\
  do { PIP_LIST_NEXT_PREV(E) = PIP_LIST_PREV(E);		\
       PIP_LIST_PREV_NEXT(E) = PIP_LIST_NEXT(E); 		\
       PIP_LIST_INIT(E); } while(0)

#define PIP_LIST_APPEND(P,Q)					\
  do { if( !PIP_LIST_ISEMPTY(Q) ) {				\
      PIP_LIST_NEXT_PREV(Q) = PIP_LIST_PREV(P);		\
      PIP_LIST_PREV_NEXT(Q) = (P);				\
      PIP_LIST_PREV_NEXT(P) = PIP_LIST_NEXT(Q);		\
      PIP_LIST_PREV(P)      = PIP_LIST_PREV(Q);		\
      PIP_LIST_INIT(Q); } } while(0)

#define PIP_LIST_ISEMPTY(L)					\
  ( PIP_LIST_NEXT(L) == (L) && PIP_LIST_PREV(L) == (pip_list_t*)(L) )

#define PIP_LIST_FOREACH(L,E)					\
  for( (E)=PIP_LIST_NEXT(L); (pip_list_t*)(L)!=(E);		\
       (E)=PIP_LIST_NEXT(E) )

#define PIP_LIST_FOREACH_SAFE(L,E,TV)				\
  for( (E)=PIP_LIST_NEXT(L), (TV)=PIP_LIST_NEXT(E);		\
       (pip_list_t*)(L)!=(E);					\
       (E)=(TV), (TV)=PIP_LIST_NEXT(TV) )

#endif
