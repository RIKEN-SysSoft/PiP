#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define DEBUG
#include <pip.h>

int
main(int argc, char **argv)
{
  int c, mode, rv = 0;
  int option_pip_mode = 0;
  int pipid;

  while ((c = getopt( argc, argv, "CLPT" )) != -1) {
    switch (c) {
    case 'C':
      option_pip_mode = PIP_MODE_PROCESS_PIPCLONE;
      break;
    case 'L':
      option_pip_mode = PIP_MODE_PROCESS_PRELOAD;
      break;
    case 'P':
      option_pip_mode = PIP_MODE_PROCESS;
      break;
    case 'T':
      option_pip_mode = PIP_MODE_PTHREAD;
      break;
    default:
      fprintf(stderr, "Usage: pip_mode [-CLPT]\n");
      exit(2);
    }
  }

  rv = pip_init( &pipid, NULL, NULL, option_pip_mode );
  if ( rv != 0 ) {
    fprintf( stderr, "pip_init: %s\n", strerror( rv ));
    return EXIT_FAILURE;
  }

  rv = pip_get_mode( &mode );
  if ( rv != 0 ) {
    fprintf( stderr, "pip_get_mode: %s\n", strerror( rv ));
    return EXIT_FAILURE;
  }

  switch ( mode ) {
  case PIP_MODE_PROCESS_PIPCLONE:
    printf( "%s\n", PIP_ENV_MODE_PROCESS_PIPCLONE );
    break;
  case PIP_MODE_PROCESS_PRELOAD:
    printf( "%s\n", PIP_ENV_MODE_PROCESS_PRELOAD );
    break;
  case PIP_MODE_PROCESS:
    printf( "%s\n", PIP_ENV_MODE_PROCESS );
    break;
  case PIP_MODE_PTHREAD:
    printf( "%s\n", PIP_ENV_MODE_PTHREAD );
    break;
  }

  return EXIT_SUCCESS;
}
