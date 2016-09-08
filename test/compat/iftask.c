/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2016
*/

#define DEBUG
#include <test.h>

#define SZ	(10)

int array[SZ] = { 0 };

void init_func( void ) __attribute__ ((constructor));
void init_func( void ) {
  int i;

  fprintf( stderr, "constructor is called\n" );
  for( i=0; i<SZ; i++ ) {
    array[i] = i;
  }
}

void dest_func( void ) __attribute__ ((destructor));
void dest_func( void ) {
  fprintf( stderr, "destructor is called\n" );
}

int main( int argc, char **argv ) {
  int i;

  for( i=0; i<SZ; i++ ) {
    if( array[i] != i ) {
      fprintf( stderr, "array[%d]=%d !!!\n", i, array[i] );
    }
  }
  return 0;
}
