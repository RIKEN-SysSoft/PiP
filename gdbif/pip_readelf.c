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
 * Query:   procinproc-info+noreply@googlegroups.com
 * User ML: procinproc-users+noreply@googlegroups.com
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
#include <error.h>
#include <errno.h>

static void read_elf64_header( int fd, Elf64_Ehdr *ehdr ) {
  if( read( fd, ehdr, sizeof(Elf64_Ehdr) ) != sizeof(Elf64_Ehdr) ) {
    fprintf( stderr, "Unable to read\n" );
    exit( 1 );
  } else if( ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
	     ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
	     ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
	     ehdr->e_ident[EI_MAG3] != ELFMAG3 ) {
    fprintf( stderr, "Not an ELF\n" );
    exit( 1 );
  } else if( ehdr->e_ident[EI_CLASS] != ELFCLASS64 ) {
    fprintf( stderr, "32bit class is not supported\n" );
    exit( 1 );
  }
}

static void
read_elf64_section_header( int fd, int nth, Elf64_Ehdr *ehdr, Elf64_Shdr *shdr ) {
  if( pread( fd,
	     shdr,
	     sizeof(Elf64_Shdr),
	     ehdr->e_shoff+(ehdr->e_shentsize*nth) ) != sizeof(Elf64_Shdr) ) {
    fprintf( stderr, "Unable to read section header\n" );
    exit( 1 );
  }
#ifdef DEBUG
  printf( "[%d] Type:%d  Name:%d  Link:%d\n",
	  nth, (int) shdr->sh_type, (int) shdr->sh_name, (int) shdr->sh_link );
#endif
}

static void read_elf64_symtab( int fd, Elf64_Shdr *shdr, Elf64_Sym *symtab ) {
  if( pread( fd, symtab, shdr->sh_size, shdr->sh_offset ) != shdr->sh_size ) {
    fprintf( stderr, "Unable to read SYMTAB section\n" );
    exit( 1 );
  }
}

static int64_t read_elf64_symbol_value( int fd, Elf64_Ehdr *ehdr, Elf64_Sym *sym ) {
  Elf64_Shdr 	shdr;
  int64_t	val;
  read_elf64_section_header( fd, sym->st_shndx, ehdr, &shdr );
#ifdef DEBUG
  printf( "SHNDX:%d  Sz:%d  st_val:%d\n",
	  (int) sym->st_shndx, (int) sym->st_size, (int) sym->st_value );
#endif
  if( pread( fd, &val, sym->st_size, shdr.sh_offset+sym->st_value ) != sym->st_size ) {
    fprintf( stderr, "Unable to read value\n" );
    exit( 1 );
  }
  return val;
}

static void read_elf64( char *path, FILE *fout ) {
  Elf64_Ehdr 	ehdr;
  Elf64_Shdr	shdr, shdr_str;
  Elf64_Sym	*symtab, *sym;
  char		*strtab;
  int64_t     	val;
  int		fd, i, j, n;

  if( ( fd = open( path, O_RDONLY ) ) < 0 ) {
    fprintf( stderr, "'%s': open() fails (%s)\n", path, strerror( errno ) );
    exit( 1 );
  }
  read_elf64_header( fd, &ehdr );
  for( i=0; i<ehdr.e_shnum; i++ ) {
    read_elf64_section_header( fd, i, &ehdr, &shdr );
    if( shdr.sh_type == SHT_SYMTAB &&
	shdr.sh_link != 0 ) {
      symtab = (Elf64_Sym*) malloc( shdr.sh_size );
      read_elf64_symtab( fd, &shdr, symtab );
      read_elf64_section_header( fd, shdr.sh_link, &ehdr, &shdr_str );
      strtab = (char*) malloc( shdr_str.sh_size );
      if( pread( fd, strtab, shdr_str.sh_size, shdr_str.sh_offset ) != shdr_str.sh_size ) {
	fprintf( stderr, "Unable to read STRTAB section\n" );
	exit( 1 );
      }
      n = shdr.sh_size / sizeof(Elf64_Sym);
      sym = symtab;
      for( j=0; j<n; j++ ) {
	int bind = ELF64_ST_BIND(sym->st_info);
	if( bind == STB_GLOBAL ) {
	  val  = read_elf64_symbol_value( fd, &ehdr, sym );
	  char *name = strtab + sym->st_name;
	  fprintf( fout, "#define %s\t(%d)\n", name, (int) val );
	}
	sym ++;
      }
      free( symtab );
      free( strtab );
    }
  }
  (void) close( fd );
}

static void print_usage( char *prog ) {
  fprintf( stderr, "%s <dot_O> [<dot_H>]\n", prog );
  exit( 1 );
}

int main( int argc, char **argv ) {
  FILE *fout;
  if( argc < 2 || argc > 3 ) print_usage( argv[0] );
  if( argc == 3 ) {
    if( ( fout = fopen( argv[2], "w" ) ) == NULL ) {
      fprintf( stderr, "%s: Unable to open\n", argv[2] );
      exit( 1 );
    }
  } else {
    fout = stdout;
  }
  read_elf64( argv[1], fout );
  return 0;
}
