/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2016, 2017, 2018
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
#define DBG_OUTPUT	do { _dbuf[ strlen( _dbuf ) ] = '\n';	\
    write( 2, _dbuf, strlen(_dbuf) ); _dbuf[0]='\0';} while(0)
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

#define PIP_TASK_DESCRIBE( ID )				\
  pip_task_describe( stderr, __func__, (ID) );
#define PIP_ULP_DESCRIBE( ULP )				\
  void pip_ulp_describe( stderr, __func__, (ULP) );
#define PIP_ULP_QUEUE_DESCRIBE( Q )			\
  pip_ulp_queue_describe( stderr, __func__, (Q) );

#else

#define DBG
#define DBGF(...)
#define RETURN(X)	return (X)
#define ASSERT(X)

#define PIP_TASK_DESCRIBE( ID )
#define PIP_ULP_DESCRIBE( ULP )
#define PIP_ULP_QUEUE_DESCRIBE( Q )

#endif

#if defined( DO_CHECK_CTYPE ) || defined( DEBUG )
#include <ctype.h>
#define PIP_CHECK_CTYPE					\
  do{ DBGF( "__ctype_b_loc()=%p", __ctype_b_loc() );			\
  DBGF( "__ctype_toupper_loc()=%p", __ctype_toupper_loc() );		\
  DBGF( "__ctype_tolower_loc()=%p", __ctype_tolower_loc() ); } while( 0 )
#else
#define PIP_CHECK_CTYPE
#endif

#endif

#endif
