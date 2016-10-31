/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2016
*/

#include <stdio.h>
#include <errno.h>

#define PRINT_FL(FSTR,V)	\
  fprintf(stderr,"%s:%d %s=%d\n",__FILE__,__LINE__,FSTR,V)

#define TESTINT(F)		\
  do{int __xyz=(F); if(__xyz!=0){PRINT_FL(#F,__xyz);exit(9);}} while(0)

#define TESTSC(F)		\
  do{int __xyz=(F); if(__xyz!=0){PRINT_FL(#F,errno);exit(9);}} while(0)

#include <sys/time.h>

static inline double get_time( void ) {
  struct timeval tv;
  TESTINT( gettimeofday( &tv, NULL ) );
  return ((double)tv.tv_sec + (((double)tv.tv_usec) * 1.e-6));
}
