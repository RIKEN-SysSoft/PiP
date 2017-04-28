/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
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
#define RETURN(X)							\
  do { DBG_PRTBUF; if(X) { DBG_TAG; DBG_PRNT("ERROR RETURN (%d)",X);	\
			   DBG_OUTPUT; } return (X); } while(0)

#else

#define DBG
#define DBGF(...)
#define RETURN(X)	return (X)

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
