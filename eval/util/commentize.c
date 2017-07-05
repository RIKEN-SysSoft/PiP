/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2017
 */

#define _GNU_SOURCE
#include <stdio.h>

int main( int argc, char **argv ) {
  FILE *fp;
  char *line = NULL;
  size_t n = 0;

  if( argv[1] == NULL ) {
    fprintf( stderr, "filename must be specified\n" );
    return 1;
  } else if( ( fp = fopen( argv[1], "r" ) ) == NULL ) {
    fprintf( stderr, "Unable to open '%s'\n", argv[1] );
    return 1;
  } else {
    while( getline( &line, &n, fp ) >= 0 ) {
      printf( "# %s", line );
    }
    fclose( fp );
  }
  return 0;
}
