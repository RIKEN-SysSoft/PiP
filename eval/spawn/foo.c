/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#include <stdio.h>
//#include <pip.h>

int main() {
  printf( "Hello PID=%d\n", getpid() );
  //pip_exit( 0 );
  return 0;
}
