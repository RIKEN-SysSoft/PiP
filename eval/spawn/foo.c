/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#include <unistd.h>
#include <stdio.h>

int main() {
  printf( "Hello PID=%d\n", getpid() );
  return 0;
}
