/*
  * $RIKEN_copyright: Riken Center for Computational Sceience,
  * System Software Development Team, 2016, 2017, 2018, 2019, 2020$
  * $PIP_VERSION: Version 1.2.0$
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
  * Written by Atsushi HORI <ahori@riken.jp>, 2016
*/

#ifndef _pip_debug_h_
#define _pip_debug_h_

#ifndef DOXYGEN_SHOULD_SKIP_THIS

/**** debug macros ****/

#ifdef DEBUG

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <string.h>

#define DBG_PRTBUF	char _dbuf[1024]={'\0'}
#define DBG_PRNT(...)	sprintf(_dbuf+strlen(_dbuf),__VA_ARGS__)
#define DBG_OUTPUT	\
  do { int _dbuf_len = strlen( _dbuf ), _dbuf_rv;		\
    _dbuf[ _dbuf_len ] = '\n';					\
    _dbuf_rv = write( 2, _dbuf, _dbuf_len + 1 );		\
    (void)_dbuf_rv;						\
    _dbuf[0]='\0';} while(0)
#define DBG_TAG							\
  do { char __tag[64]; pip_idstr(__tag,64);			\
    DBG_PRNT("%s %s:%d %s(): ",__tag,				\
	     __FILE__, __LINE__, __func__ );	} while(0)

#define DBG						\
  do { DBG_PRTBUF; DBG_TAG; DBG_OUTPUT; } while(0)
#define DBGF(...)							\
  do { DBG_PRTBUF; DBG_TAG; DBG_PRNT(__VA_ARGS__); DBG_OUTPUT; }while(0)
#define RETURN(X)						\
  do { DBG_PRTBUF; int __xxx=(X); if(__xxx) {				\
      DBG_TAG; DBG_PRNT("ERROR RETURN (%d)",__xxx); DBG_OUTPUT; }	\
    return (__xxx); } while(0)

#define ASSERT(X)	\
  do { if( !(X) ) DBGF( "%s failed !!!", #X ); } while(0)

#else

#define DBG
#define DBGF(...)
#define RETURN(X)	return (X)
#define ASSERT(X)

#endif

#ifdef DO_CHECK_CTYPE
#include <ctype.h>
#define CHECK_CTYPE					\
  do{ DBGF( "__ctype_b_loc()=%p", __ctype_b_loc() );			\
  DBGF( "__ctype_toupper_loc()=%p", __ctype_toupper_loc() );		\
  DBGF( "__ctype_tolower_loc()=%p", __ctype_tolower_loc() ); } while( 0 )
#else
#define CHECK_CTYPE
#endif

#endif

#endif
