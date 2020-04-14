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

#ifndef _pip_debug_h_
#define _pip_debug_h_

#ifndef DOXYGEN_SHOULD_SKIP_THIS

/**** debug macros ****/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sys/types.h>
#include <sys/syscall.h>
#include <dlfcn.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <pip_util.h>

extern int  pip_idstr( char *buf, size_t sz );

#define DBGSW		pip_debug_env()

#define DBGBUFLEN	(256)
#define DBGTAGLEN	(32)

#define DBG_PRTBUF	char _dbuf[DBGBUFLEN]={'\0'};	\
  			int _nn=DBGBUFLEN;
#define DBG_PRNT(...)	do {				\
    snprintf(_dbuf+strlen(_dbuf),_nn,__VA_ARGS__);	\
    _nn=DBGBUFLEN-strlen(_dbuf); } while(0)

#ifdef DEBUG_LOCK
#define DBG_LOCK				\
  if( pip_root != NULL ) pip_spin_lock( &pip_root->lock_bt )
#define DBG_UNLOCK				\
  if( pip_root != NULL ) pip_spin_unlock( &pip_root->lock_bt )
#else
#define DBG_LOCK
#define DBG_UNLOCK
#endif

#define DBG_OUTPUT	\
  do { int _dbuf_len = strlen( _dbuf ), _dbuf_rv;		\
    _dbuf[ _dbuf_len ] = '\n';					\
    DBG_LOCK;							\
    _dbuf_rv = write( 2, _dbuf, _dbuf_len + 1 );		\
    DBG_UNLOCK;							\
    (void)_dbuf_rv;						\
    _dbuf[0]='\0';_nn=DBGBUFLEN;} while(0)

#define DBG_OUTPUT_NNL	\
  do { int _dbuf_len = strlen( _dbuf ), _dbuf_rv;		\
    _dbuf_rv = write( 2, _dbuf, _dbuf_len );			\
    (void)_dbuf_rv;						\
    _dbuf[0]='\0';_nn=DBGBUFLEN;} while(0)

#define DBG_NN

#define DBG_TAG							\
  do { char __tag[DBGTAGLEN]; pip_idstr(__tag,DBGTAGLEN);	\
    DBG_PRNT("%s %s:%d %s()",__tag,				\
	     basename(__FILE__), __LINE__, __func__ );	} while(0)

#define EMSG(...)							\
  do { DBG_PRTBUF; DBG_TAG; DBG_PRNT(": ");				\
    DBG_PRNT(__VA_ARGS__); DBG_OUTPUT; } while(0)

#define EMSG_NNL(...)							\
  do { DBG_PRTBUF; DBG_TAG; DBG_PRNT(": ");				\
    DBG_PRNT(__VA_ARGS__); DBG_OUTPUT_NNL; } while(0)

#define NL_EMSG(...)							\
  do { DBG_PRTBUF; DBG_PRNT("\n"); DBG_TAG; DBG_PRNT(": ");		\
    DBG_PRNT(__VA_ARGS__); DBG_OUTPUT; } while(0)


#ifdef DEBUG

extern int pip_debug_env( void );
#define DBG_TAG_ENTER							\
  do { char __tag[DBGTAGLEN]; pip_idstr(__tag,DBGTAGLEN);		\
    DBG_PRNT("%s %s:%d >> %s()",__tag,					\
	     basename(__FILE__), __LINE__, __func__ );	} while(0)
#define DBG_TAG_LEAVE							\
  do { char __tag[DBGTAGLEN]; pip_idstr(__tag,DBGTAGLEN);		\
    DBG_PRNT("%s %s:%d << %s()",__tag,					\
	     basename(__FILE__), __LINE__, __func__ );	} while(0)

#define DBG						\
  if(DBGSW) { DBG_PRTBUF; DBG_TAG; DBG_OUTPUT; }

#define DBG_NL						\
  if(DBGSW) { DBG_PRTBUF; DBG_PRNT("\n"); DBG_OUTPUT; }

#define DBGF(...)						\
  if(DBGSW) { EMSG(__VA_ARGS__); }

#define DBGF_NNL(...)						\
  if(DBGSW) { EMSG_NNL(__VA_ARGS__); }

#define ENTER							\
  if(DBGSW) { DBG_PRTBUF; DBG_TAG_ENTER; DBG_OUTPUT; }

#define ENTERF(...)							\
  if(DBGSW) do { DBG_PRTBUF; DBG_TAG_ENTER; DBG_PRNT(": ");	\
      DBG_PRNT(__VA_ARGS__); DBG_OUTPUT; } while(0)

#define RETURN(X)							\
  do { int __xxx=(X);							\
    if(DBGSW) { DBG_PRTBUF; DBG_TAG_LEAVE;				\
      if(__xxx) { DBG_PRNT(": ERROR RETURN %d:'%s'",__xxx,strerror(__xxx)); \
      } else { DBG_PRNT(": returns %d",__xxx); }			\
      DBG_OUTPUT; } return (__xxx); } while(0)

#define RETURNV								\
  do { if(DBGSW) { DBG_PRTBUF; DBG_TAG_LEAVE; DBG_OUTPUT; }		\
    return; } while(0)

#define ASSERTD(X)							\
  if(DBGSW) { if(X) { NL_EMSG("{%s} Assertion FAILED !!!!!!\n",#X);	\
      pip_debug_info(); } } while(0)

#define DO_CHECK_CTYPE

#else

#define DBG
#define DBGF(...)
#define DBGF_NNL(...)
#define ENTER
#define ENTERF(...)
#define RETURN(X)		return(X)
#define RETURNV			return
#define ASSERTD(X)

#endif	/* !DEBUG */

#ifdef DO_CHECK_CTYPE
#include <ctype.h>
#define PIP_CHECK_CTYPE						\
  do{ DBGF( "isalnum('a')=%d", isalnum('a') ); } while(0)
#else
#define PIP_CHECK_CTYPE
#endif

#define ASSERT(X)							\
  if(X){NL_EMSG("{%s} Assertion FAILED !!!!!!\n",#X);pip_debug_info();	\
    } else { DBGF( "{%s} -- Assertion OK", #X ); }

#define NEVER_REACH_HERE						\
  do { NL_EMSG( "Should never reach here !!!!!!" ); } while(0)

#define TASK_DESCRIBE( ID )			\
  pip_task_describe( stderr, __func__, (ID) );

#define QUEUE_DESCRIBE( Q )				\
  pip_queue_describe( stderr, __func__, PIP_TASKQ(Q) );

#define ERRJ		{ DBG;                goto error; }
#define ERRJ_ERRNO	{ DBG; err=errno;     goto error; }
#define ERRJ_ERR(ENO)	{ DBG; err=(ENO);     goto error; }
#define ERRJ_CHK(FUNC)	{ if( (FUNC) ) { DBG; goto error; } }

#endif
#endif
