#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#define BUFSZ		(1024)

static inline void print_maps( void ) {
  int fd = open( "/proc/self/maps", O_RDONLY );
  char buf[BUFSZ];

  fflush( NULL );
  printf( "PID: %d\n", getpid() );
  while( 1 ) {
    ssize_t rc, wc;

    if( ( rc = read( fd, buf, BUFSZ ) ) <= 0 ) break;
    fwrite( buf, rc, 1, stdout );
  }
  close( fd );
  fflush( stdout );
}
