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

#define DBG_NL 	fprintf( stderr, "\n" )
#define DBG_TAG 	\
  fprintf( stderr, "[TID:%d] %s:%d %s(): ", (int)syscall(__NR_gettid), 	\
	   __FILE__, __LINE__, __func__ )
#define DBG	{ DBG_TAG; DBG_NL; }
#define DBGF(...)	\
  do { DBG_TAG; fprintf( stderr, __VA_ARGS__ ); DBG_NL; } while(0)
#define RETURN(X)	\
  do {if(X){DBG_TAG;fprintf(stderr,"ERROR RETURN (%d)\n",X);}return (X);} while(0)

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

#endif
