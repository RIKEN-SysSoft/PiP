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

/**** debug macros ****/

#ifndef DEBUG
//#define DEBUG
#endif

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
#define DBG_OUTPUT	fprintf(stderr,"%s\n",_dbuf)
#define DBG_TAG		\
  do { DBG_PRNT("[PID:%d] %s:%d %s(): ",(int)getpid(),			\
		__FILE__, __LINE__, __func__ );	} while(0)

#define DBG		\
  do { DBG_PRTBUF; DBG_TAG; DBG_OUTPUT; } while(0)
#define DBGF(...)	\
  do { DBG_PRTBUF; DBG_TAG; DBG_PRNT(__VA_ARGS__); DBG_OUTPUT; }while(0)
#define RETURN(X)	\
  do { DBG_PRTBUF; if(X) { DBG_TAG; DBG_PRNT("ERROR RETURN (%d)\n",X);	\
			   DBG_OUTPUT; } return (X); } while(0)

#else

#define DBG_NL
#define DBG_TAG
#define DBG
#define DBGF(...)
#define RETURN(X)	return (X)

#endif

#include <fcntl.h>
#include <unistd.h>

#define PIP_DEBUG_BUFSZ		(4096)

inline static void pip_print_maps( void ) __attribute__ ((unused));
inline static void pip_print_maps( void ) {
  int fd = open( "/proc/self/maps", O_RDONLY );
  char buf[PIP_DEBUG_BUFSZ];

  while( 1 ) {
    ssize_t rc, wc;
    char *p;

    if( ( rc = read( fd, buf, PIP_DEBUG_BUFSZ ) ) <= 0 ) break;
    p = buf;
    do {
      if( ( wc = write( 1, p, rc ) ) < 0 ) break; /* STDOUT */
      p  += wc;
      rc -= wc;
    } while( rc > 0 );
  }
  close( fd );
}

#include <asm/prctl.h>
#include <sys/prctl.h>
#include <errno.h>

inline static void print_fs_segreg( void ) {
  int arch_prctl(int, unsigned long*);
  unsigned long fsreg;
  if( arch_prctl( ARCH_GET_FS, &fsreg ) == 0 ) {
    fprintf( stderr, "FS REGISTER: 0x%lx\n", fsreg );
  } else {
    fprintf( stderr, "FS REGISTER: (unable to get:%d)\n", errno );
  }
}

#include <ctype.h>

#ifdef DO_CHECK_CTYPE
#define CHECK_CTYPE					\
  do{ DBGF( "__ctype_b_loc()=%p", __ctype_b_loc() );			\
  DBGF( "__ctype_toupper_loc()=%p", __ctype_toupper_loc() );		\
  DBGF( "__ctype_tolower_loc()=%p", __ctype_tolower_loc() ); } while( 0 )
#else
#define CHECK_CTYPE
#endif

#endif
