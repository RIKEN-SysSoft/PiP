/*
 * $PIP_license: <Simplified BSD License>
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *     Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 * 
 *     Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 * $
 * $RIKEN_copyright: Riken Center for Computational Sceience (R-CCS),
 * System Software Development Team, 2016-2021
 * $
 * $PIP_VERSION: Version 3.0.0$
 *
 * $Author: Atsushi Hori (R-CCS)
 * Query:   procinproc-info@googlegroups.com
 * User ML: procinproc-users@googlegroups.com
 * $
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <elf.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <libgen.h>
#include <error.h>
#include <errno.h>

#undef DEBUG

//#define	DF_1_PIE	0x08000000

char *cmd;

#ifdef DF_1_PIE

static void read_elf64_header( int fd, Elf64_Ehdr *ehdr ) {
  if( read( fd, ehdr, sizeof(Elf64_Ehdr) ) != sizeof(Elf64_Ehdr) ) {
    fprintf( stderr, "%s: Unable to read\n", cmd );
    exit( 2 );
  } else if( ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
	     ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
	     ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
	     ehdr->e_ident[EI_MAG3] != ELFMAG3 ) {
    fprintf( stderr, "Not ELF\n" );
    exit( 1 );
  } else if( ehdr->e_ident[EI_CLASS] != ELFCLASS64 ) {
    fprintf( stderr, "%s: 32bit class is not supported\n", cmd );
    exit( 2 );
  }
}

static void
read_elf64_section_header( int fd, int nth, Elf64_Ehdr *ehdr, Elf64_Shdr *shdr ) {
  off_t off = ehdr->e_shoff + ( ehdr->e_shentsize * nth );
  if( pread( fd, shdr, sizeof(Elf64_Shdr), off ) != sizeof(Elf64_Shdr) ) {
    fprintf( stderr, "%s: Unable to read section header\n", cmd );
    exit( 2 );
  }
#ifdef DEBUG
  printf( "%s: [%d] Type:%d  Name:%d  Link:%d\n", cmd,
	  nth, (int) shdr->sh_type, (int) shdr->sh_name, (int) shdr->sh_link );
#endif
}

#ifdef NOT_USED
static void
write_elf64_section_header( int fd, int nth, Elf64_Ehdr *ehdr, Elf64_Shdr *shdr ) {
  off_t off = ehdr->e_shoff + ( ehdr->e_shentsize * nth );
  if( pwrite( fd, shdr, sizeof(Elf64_Shdr), off ) != sizeof(Elf64_Shdr) ) {
    fprintf( stderr, "%s: Unable to write section header\n", cmd );
    exit( 2 );
  }
#ifdef DEBUG
  printf( "[%d] Type:%d  Name:%d  Link:%d\n",
	  nth, (int) shdr->sh_type, (int) shdr->sh_name, (int) shdr->sh_link );
#endif
}
#endif

void read_elf64_dynamic_section( int fd, off_t offset, size_t size, Elf64_Dyn *dyns ) {
  if( pread( fd, dyns, size, offset ) != size ) {
    fprintf( stderr, "%s: Unable to read dynamic section\n", cmd );
    exit( 2 );
  }
}

void write_elf64_dynamic_section( int fd, off_t offset, size_t size, Elf64_Dyn *dyns ) {
  if( pwrite( fd, dyns, size, offset ) != size ) {
    fprintf( stderr, "%s: Unable to write dynamic section\n", cmd );
    exit( 2 );
  }
}

static int unpie( char *path, int flag_check ) {
  Elf64_Ehdr 	ehdr;
  Elf64_Shdr	shdr;
  Elf64_Dyn	*dyns = NULL;
  int		fd, i, j, n;
  int 		flag_dyn = 0, flag_pie = 0, retval = 0;

  if( ( fd = open( path, O_RDWR ) ) < 0 ) {
    fprintf( stderr, "%s: open(%s) fails (%s)\n", cmd, path, strerror( errno ) );
    return 2;
  }
  read_elf64_header( fd, &ehdr );
  if( ehdr.e_type == ET_DYN ) {
    flag_dyn = 1;
  }
  for( i=0; i<ehdr.e_shnum; i++ ) {
    read_elf64_section_header( fd, i, &ehdr, &shdr );
    if( shdr.sh_type == SHT_DYNAMIC ) {
      dyns = (Elf64_Dyn*) malloc( shdr.sh_size );
      read_elf64_dynamic_section( fd, shdr.sh_offset, shdr.sh_size, dyns );
      n = shdr.sh_size / sizeof(Elf64_Dyn);
      for( j=0; j<n; j++ ) {
	if( dyns[j].d_tag == DT_FLAGS_1 ) {
	  if( dyns[j].d_un.d_val & DF_1_PIE ) {
	    flag_pie = 1;
	    dyns[j].d_un.d_val &= ~DF_1_PIE;
	  }
	}
      }
      if( ! flag_check ) {
	write_elf64_dynamic_section( fd, shdr.sh_offset, shdr.sh_size, dyns );
      }
      goto done;
    }
  }
  fprintf( stderr, "%s: Unable to find DYNAMIC section (statically linked?)\n", cmd );
  retval = 2;
 done:
  if( flag_check ) {
    if( flag_pie ) {
      fprintf( stderr, "%s: '%s' is PIE (not 'unpie-ed' yet)\n", cmd, path );
      retval = 1;
    } else if( flag_dyn ) {
      fprintf( stderr, "%s: '%s' is PIE (already 'unpie-ed' PIE)\n", cmd, path );
    } else {
      fprintf( stderr, "%s: '%s' is NOT PIE\n", cmd, path );
      retval = 1;
    }
  }
  free( dyns );
  (void) close( fd );
  return retval;
}

static void print_usage( void ) {
  fprintf( stderr, "%s [-c] <PIE>\n", cmd );
  fprintf( stderr, "   -c: check if <PIE> is PIE\n" );
  exit( 2 );
}

int main( int argc, char **argv ) {
  int flag_check = 0;
  char *fname = argv[1];

  cmd = basename( argv[0] );
  if( argc == 3 ) {
    if( strcmp( argv[1], "-c" ) == 0 ) {
      fname = argv[2];
      flag_check = 1;
    } else {
      print_usage();
    }
  }
  if( argc < 2 || argc > 3 ) print_usage();
  return unpie( fname, flag_check );
}

#else /* DF_1_PIE is not defined */

int main( int argc, char **argv ) {
  return 0;
}

#endif
