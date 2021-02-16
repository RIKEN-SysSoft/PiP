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

#include <pip/pip_internal.h>

#include <sys/syscall.h>
#include <sys/prctl.h>
#include <elf.h>

void pip_set_name( pip_task_internal_t *task ) {
  /* this function is to set the right    */
  /* name shown by the ps and top command */
#ifdef PR_SET_NAME
  pip_root_t 	*root     = AA(task)->task_root;
  int		pipid     = TA(task)->pipid;
  char		*progname = MA(task)->args.prog;
  char 		*funcname = MA(task)->args.funcname;
  char 		symbol[3], name[16], prg[16];

  symbol[2] = '\0';
  if( root->task_root == task ) {
    /* root process */
    symbol[0] = 'R';
  } else {
    symbol[0] = ( pipid % 10 ) + '0';
  }
  switch( root->opts & PIP_MODE_MASK ) {
  case PIP_MODE_PROCESS_PRELOAD:
    symbol[1] = ':';
    break;
  case PIP_MODE_PROCESS_PIPCLONE:
    symbol[1] = ';';
    break;
  case PIP_MODE_PROCESS_GOT:
    symbol[1] = '.';
    break;
  case PIP_MODE_PTHREAD:
    symbol[1] = '|';
    break;
  default:
    symbol[1] = '?';
    break;
  }
  if( progname == NULL ) {
    memset( prg, 0, sizeof(prg) );
    if( !pip_is_threaded_() ) {
      prctl( PR_GET_NAME, prg, 0, 0, 0 );
    } else {
      pthread_getname_np( pthread_self(), prg, sizeof(prg) );
    }
    progname = prg;
  } else {
    char *p;
    if( ( p = strrchr( progname, '/' ) ) != NULL) {
      progname = p + 1;
    }
  }
  if( funcname == NULL ) {
    snprintf( name, 16, "%s%s",    symbol, progname );
  } else {
    snprintf( name, 16, "%s%s@%s", symbol, progname, funcname );
  }
  if( !pip_is_threaded_() ) {
#define FMT "/proc/self/task/%u/comm"
    char fname[sizeof(FMT)+8];
    int fd;

    (void) prctl( PR_SET_NAME, name, 0, 0, 0 );
    sprintf( fname, FMT, (unsigned int) pip_gettid() );
    if( ( fd = open( fname, O_RDWR ) ) >= 0 ) {
      (void) write( fd, name, strlen(name) );
      (void) close( fd );
    }
  } else {
    (void) pthread_setname_np( pthread_self(), name );
  }
  DBGF( "CommandName:'%s'", name );
#endif
}

int pip_check_pie( const char *path, int flag_verbose ) {
  struct stat stbuf;
  Elf64_Ehdr elfh;
  int fd;
  int err = 0;

  if( strchr( path, '/' ) == NULL ) {
    if( flag_verbose ) {
      pip_err_mesg( "'%s' is not a path (no slash '/')", path );
    }
    err = ENOENT;
  } else if( ( fd = open( path, O_RDONLY ) ) < 0 ) {
    err = errno;
    if( flag_verbose ) {
      pip_err_mesg( "'%s': open() fails (%s)", path, strerror( errno ) );
    }
  } else {
    if( fstat( fd, &stbuf ) < 0 ) {
      err = errno;
      if( flag_verbose ) {
	pip_err_mesg( "'%s': stat() fails (%s)", path, strerror( errno ) );
      }
    } else if( ( stbuf.st_mode & (S_IXUSR|S_IXGRP|S_IXOTH) ) == 0 ) {
      if( flag_verbose ) {
	pip_err_mesg( "'%s' is not executable", path );
      }
      err = EACCES;
    } else if( read( fd, &elfh, sizeof( elfh ) ) != sizeof( elfh ) ) {
      if( flag_verbose ) {
	pip_err_mesg( "Unable to read '%s'", path );
      }
      err = EUNATCH;
    } else if( elfh.e_ident[EI_MAG0] != ELFMAG0 ||
	       elfh.e_ident[EI_MAG1] != ELFMAG1 ||
	       elfh.e_ident[EI_MAG2] != ELFMAG2 ||
	       elfh.e_ident[EI_MAG3] != ELFMAG3 ) {
      if( flag_verbose ) {
	pip_err_mesg( "'%s' is not ELF", path );
      }
      err = ELIBBAD;
    } else if( elfh.e_type != ET_DYN ) {
      if( flag_verbose ) {
	pip_err_mesg( "'%s' is not PIE", path );
      }
      err = ELIBEXEC;
    }
    (void) close( fd );
  }
  return err;
}

/* the following function(s) are for debugging */

int pip_debug_env( void ) {
  static int flag = 0;
  if( !flag ) {
    if( getenv( "PIP_NODEBUG" ) ) {
      flag = -1;
    } else {
      flag = 1;
    }
  }
  return flag > 0;
}

#define FDPATH_LEN	(512)
#define RDLINK_BUF	(256)
#define RDLINK_BUF_SP	(RDLINK_BUF+8)

void pip_fprint_fd( FILE *fp, int fd ) {
  char idstr[64];
  char fdpath[FDPATH_LEN];
  char fdname[RDLINK_BUF_SP];
  ssize_t sz;

  (void) pip_idstr( idstr, sizeof(idstr) );
  snprintf( fdpath, FDPATH_LEN, "/proc/self/fd/%d", fd );
  if( ( sz = readlink( fdpath, fdname, RDLINK_BUF ) ) > 0 ) {
    fdname[sz] = '\0';
    fprintf( fp, "%s %d -> %s", idstr, fd, fdname );
  }
}

void pip_print_fd( int fd ) { pip_fprint_fd( stderr, fd ); }

void pip_fprint_fds( FILE *fp ) {
  DIR *dir = opendir( "/proc/self/fd" );
  struct dirent *de;
  char idstr[64];
  char fdpath[FDPATH_LEN];
  char fdname[RDLINK_BUF_SP];
  char coe = ' ';
  ssize_t sz;

  (void) pip_idstr( idstr, sizeof(idstr) );
  if( dir != NULL ) {
    int fd_dir = dirfd( dir );
    int fd;

    while( ( de = readdir( dir ) ) != NULL ) {
      snprintf( fdpath, FDPATH_LEN, "/proc/self/fd/%s", de->d_name );
      if( ( sz = readlink( fdpath, fdname, RDLINK_BUF ) ) > 0 ) {
	fdname[sz] = '\0';
	if( ( fd = atoi( de->d_name ) ) != fd_dir ) {
	  if( pip_isa_coefd( fd ) ) coe = '*';
	  fprintf( fp, "%s %s -> %s %c", idstr, fdpath, fdname, coe );
	} else {
	  fprintf( fp, "%s %s -> %s  opendir(\"/proc/self/fd\")",
		   idstr, fdpath, fdname );
	}
      }
    }
    closedir( dir );
  }
}

void pip_print_fds( void ) { pip_fprint_fds( stderr ); }

void pip_check_addr( char *tag, void *addr ) {
  FILE *maps = fopen( "/proc/self/maps", "r" );
  char idstr[64];
  char *line = NULL;
  size_t sz  = 0;
  int retval;

  if( tag == NULL ) {
    (void) pip_idstr( idstr, sizeof(idstr) );
    tag = &idstr[0];
  }
  while( maps != NULL ) {
    void *start, *end;

    if( ( retval = getline( &line, &sz, maps ) ) < 0 ) {
      fprintf( stderr, "getline()=%d\n", errno );
      break;
    } else if( retval == 0 ) {
      continue;
    }
    line[retval] = '\0';
    if( sscanf( line, "%p-%p", &start, &end ) == 2 ) {
      if( (intptr_t) addr >= (intptr_t) start &&
	  (intptr_t) addr <  (intptr_t) end ) {
	fprintf( stderr, "%s %p: %s", tag, addr, line );
	goto found;
      }
    }
  }
  fprintf( stderr, "%s %p (not found)\n", tag, addr );
 found:
  if( line != NULL ) free( line );
  fclose( maps );
}

double pip_gettime( void ) {
  struct timeval tv;
  gettimeofday( &tv, NULL );
  return ((double)tv.tv_sec + (((double)tv.tv_usec) * 1.0e-6));
}

void pip_fprint_loaded_solibs( FILE *fp ) {
  void *handle = NULL;
  char idstr[PIP_MIDLEN];
  int err;

  /* pip_init() must be called in advance */
  (void) pip_idstr( idstr, sizeof(idstr) );

  if( ( err = pip_get_dso( PIP_PIPID_MYSELF, &handle ) ) != 0 ) {
    pip_info_fmesg( fp, "%s (no solibs found: %d)\n", idstr, err );
  } else {
    struct link_map *map = (struct link_map*) handle;
    for( ; map!=NULL; map=map->l_next ) {
      char *fname;
      if( *map->l_name == '\0' ) {
	fname = "(noname)";
      } else {
	fname = map->l_name;
      }
      pip_info_fmesg( fp, "%s %s at %p\n", idstr, fname, (void*)map->l_addr );
    }
  }
  if( pip_isa_root() && handle != NULL ) dlclose( handle );
}

void pip_print_loaded_solibs( FILE *file ) {
  pip_fprint_loaded_solibs( file );
}

static int pip_dsos_cb( struct dl_phdr_info *info, size_t size, void *data ) {
  FILE *fp = (FILE*) data;
  int i;

  fprintf( fp, "name=%s (%d segments)\n", info->dlpi_name, info->dlpi_phnum);

  for ( i=0; i<info->dlpi_phnum; i++ ) {
    fprintf( fp, "\t\t header %2d: address=%10p\n", i,
	     (void *) (info->dlpi_addr + info->dlpi_phdr[i].p_vaddr ) );
  }
  return 0;
}

void pip_fprint_dsos( FILE *fp ) {
  dl_iterate_phdr( pip_dsos_cb, (void*) fp );
}

void pip_print_dsos( void ) {
  FILE *fp = stderr;
  dl_iterate_phdr( pip_dsos_cb, (void*) fp );
}

int pip_is_debug_build( void ) {
#ifdef DEBUG
  return 1;
#else
  return 0;
#endif
}
