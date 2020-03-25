/*
 * $RIKEN_copyright: 2018 Riken Center for Computational Sceience,
 * 	  System Software Devlopment Team. All rights researved$
 * $PIP_VERSION: Version 1.0$
 * $PIP_license: <Simplified BSD License>
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * official policies, either expressed or implied, of the PiP project.$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>
 */

#include <pip_internal.h>

#include <sys/syscall.h>
#include <sys/prctl.h>
#include <elf.h>

void pip_set_name( char *symbol, char *progname, char *funcname ) {
#ifdef PR_SET_NAME
  /* the following code is to set the right */
  /* name shown by the ps and top commands  */
  char nam[16];

  if( progname == NULL ) {
    char prg[16];
    prctl( PR_GET_NAME, prg, 0, 0, 0 );
    snprintf( nam, 16, "%s%s",      symbol, prg );
  } else {
    char *p;
    if( ( p = strrchr( progname, '/' ) ) != NULL) {
      progname = p + 1;
    }
    if( funcname == NULL ) {
      snprintf( nam, 16, "%s%s",    symbol, progname );
    } else {
      snprintf( nam, 16, "%s%s@%s", symbol, progname, funcname );
    }
  }
  if( !pip_is_threaded_() ) {
#define FMT "/proc/self/task/%u/comm"
    char fname[sizeof(FMT)+8];
    int fd;

    (void) prctl( PR_SET_NAME, nam, 0, 0, 0 );
    sprintf( fname, FMT, (unsigned int) pip_gettid() );
    if( ( fd = open( fname, O_RDWR ) ) >= 0 ) {
      (void) write( fd, nam, strlen(nam) );
      (void) close( fd );
    }
  } else {
    (void) pthread_setname_np( pthread_self(), nam );
  }
  if( 0 ) {
    char cmd[16];
    prctl( PR_GET_NAME, cmd, 0, 0, 0 );
    printf( "CommanndName:'%s'\n", cmd );
  }
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

void pip_pipidstr( pip_task_internal_t *taski, char *buf ) {
  switch( taski->pipid ) {
  case PIP_PIPID_ROOT:
    strcpy( buf, "?root?" );
    break;
  case PIP_PIPID_MYSELF:
    strcpy( buf, "?myself?" );
    break;
  case PIP_PIPID_ANY:
    strcpy( buf, "?any?" );
    break;
  case PIP_PIPID_NULL:
    strcpy( buf, "?null?" );
    break;
  default:
    if( taski->task_sched == NULL ) {
      (void) sprintf( buf, "%d[-]", taski->pipid );
    } else if( pip_task->task_sched == pip_task ) {
      (void) sprintf( buf, "%d[*]", taski->pipid );
    } else if( pip_task->task_sched != NULL ) {
      (void) sprintf( buf, "%d[%d]",
		      taski->pipid, taski->task_sched->pipid );
    } else {
      (void) sprintf( buf, "%d/?", taski->pipid );
    }
    break;
  }
}

static char *pip_type_str_( pip_task_internal_t *taski ) {
  char *typestr;

  if( taski->type == PIP_TYPE_NONE ) {
    typestr = "none";
  } else if( PIP_ISA_ROOT( taski ) ) {
    if( PIP_IS_SUSPENDED( taski ) ) {
      typestr = "root";
    } else {
      typestr = "ROOT";
    }
  } else if( PIP_ISA_TASK( taski ) ) {
    if( PIP_IS_SUSPENDED( taski ) ) {
      typestr = "task";
    } else {
      typestr = "TASK";
    }
  } else {
    typestr = "(**)";
  }
  return typestr;
}

char * pip_type_str( void ) {
  char *typestr;

  if( pip_task == NULL ) {
    typestr = "(NULL)";
  } else {
    typestr = pip_type_str_( pip_task );
  }
  return typestr;
}

pid_t pip_gettid( void ) {
  return (pid_t) syscall( (long int) SYS_gettid );
}

int pip_tgkill( int tgid, int tid, int signal ) {
  return (int) syscall( (long int) SYS_tgkill, tgid, tid, signal );
}

int pip_tkill( int tid, int signal ) {
  return (int) syscall( (long int) SYS_tkill, tid, signal );
}

int pip_idstr( char *buf, size_t sz ) {
  pid_t	tid = pip_gettid();
  char *pre  = "<";
  char *post = ">";
  int	n = 0;

  if( pip_task == NULL ) {
    n = snprintf( buf, sz, "(%d)", tid );
  } else if( PIP_ISA_ROOT( pip_task ) ) {
    if( PIP_IS_SUSPENDED( pip_task ) ) {
      n = snprintf( buf, sz, "%sroot:(%d)%s", pre, tid, post );
    } else {
      n = snprintf( buf, sz, "%sROOT:(%d)%s", pre, tid, post );
    }
  } else if( PIP_ISA_TASK( pip_task ) ) {
    char idstr[64];

    pip_pipidstr( pip_task, idstr );
    if( PIP_IS_SUSPENDED( pip_task ) ) {
      n = snprintf( buf, sz, "%stask:%s(%d)%s", pre, idstr, tid, post );
    } else {
      n = snprintf( buf, sz, "%sTASK:%s(%d)%s", pre, idstr, tid, post );
    }
  } else {
    n = snprintf( buf, sz, "%sType:0x%x(%d)%s ",
		  pre, pip_task->type, tid, post );
  }
  return n;
}

static void pip_message( FILE *fp, char *tagf, const char *format, va_list ap ) {
#define PIP_MESGLEN		(512)
  char mesg[PIP_MESGLEN];
  char idstr[PIP_MIDLEN];
  int len;

  len = pip_idstr( idstr, PIP_MIDLEN );
  len = snprintf( &mesg[0], PIP_MESGLEN-len, tagf, idstr );
  vsnprintf( &mesg[len], PIP_MESGLEN-len, format, ap );
  fprintf( stderr, "%s\n", mesg );
}

void pip_info_fmesg( FILE *fp, const char *format, ... ) {
  va_list ap;
  va_start( ap, format );
  if( fp == NULL ) fp = stderr;
  pip_message( fp, "PiP-INFO%s ", format, ap );
}

void pip_info_mesg( const char *format, ... ) {
  va_list ap;
  va_start( ap, format );
  pip_message( stderr, "PiP-INFO%s ", format, ap );
}

void pip_warn_mesg( const char *format, ... ) {
  va_list ap;
  va_start( ap, format );
  pip_message( stderr, "PiP-WRN%s ", format, ap );
}

void pip_err_mesg( const char *format, ... ) {
  va_list ap;
  va_start( ap, format );
  pip_message( stderr, "PiP-ERR%s ", format, ap );
}

void pip_task_describe( FILE *fp, const char *tag, int pipid ) {
  if( pip_check_pipid( &pipid ) == 0 ) {
    pip_task_internal_t *task = pip_get_task( pipid );
    char *typestr = pip_type_str_( task );
    if( tag == NULL ) {
      pip_info_fmesg( fp,
		      "%s[%d]@%p (sched:%p,ctx:%p)",
		      typestr,
		      pipid,
		      task,
		      task->task_sched,
		      task->ctx_suspend );
    } else {
      pip_info_fmesg( fp,
		      "%s: %s[%d]@%p (sched:%p,ctx:%p)",
		      tag,
		      typestr,
		      pipid,
		      task,
		      task->task_sched,
		      task->ctx_suspend );
    }
  } else {
    if( tag == NULL ) {
      pip_info_fmesg( fp, "TASK:(pipid:%d is invlaid)", pipid );
    } else {
      pip_info_fmesg( fp, "%s: TASK:(pipid:%d is invlaid)", tag, pipid );
    }
  }
}

void pip_queue_describe( FILE *fp, const char *tag, pip_task_t *queue ) {
  if( fp == NULL ) fp = stderr;
  if( queue == NULL ) {
    if( tag == NULL ) {
      pip_info_fmesg( fp, "QUEUE:(nil)" );
    } else {
      pip_info_fmesg( fp, "%s: QUEUE:(nil)", tag );
    }
  } else if( PIP_TASKQ_ISEMPTY( queue ) ) {
    if( tag == NULL ) {
      pip_info_fmesg( fp, "QUEUE:%p[%d]  EMPTY",
		      queue, (int) PIP_TASKI(queue)->pipid );
    } else {
      pip_info_fmesg( fp, "%s: QUEUE:%p[%d]  EMPTY",
		      tag, queue, (int) PIP_TASKI(queue)->pipid );
    }
  } else {
    pip_task_t 		*u, *next, *prev;
    pip_task_internal_t *t;
    int 		i, pn, pp;

    next = queue->next;
    prev = queue->next;
    if( prev == queue ) {
      prev = (void*) 0x8888;
      pp = -1;
    } else {
      pp = PIP_TASKI(prev)->pipid;
    }
    if( next == queue ) {
      next = (void*) 0x8888;
      pn = -1;
    } else {
      pn = PIP_TASKI(next)->pipid;
    }

    t = PIP_TASKI( queue );

    if( tag == NULL ) {
      pip_info_fmesg( fp,
		      "QUEUE:%p next:%p[%d] prev:%p[%d]",
		      queue, next, (int) pn, prev, (int) pp );
    } else {
      pip_info_fmesg( fp,
		      "%s: QUEUE:%p next:%p[%d] prev:%p[%d]",
		      tag, queue, next, pn, prev, pp );
    }
    i = 0;
    PIP_TASKQ_FOREACH( queue, u ) {
      next = u->next;
      prev = u->next;
      if( prev == queue ) {
	prev = (void*) 0x8888; /* prev == root */
	pp   = -1;
      } else {
	pp = PIP_TASKI(prev)->pipid;
      }
      if( next == queue ) {
	next = (void*) 0x8888; /* next == root */
	pn = -1;
      } else {
	pn = PIP_TASKI(next)->pipid;
      }
      t = PIP_TASKI( u );
      if( tag == NULL ) {
	pip_info_fmesg( fp,
			"QUEUE[%d]:%p "
			"(sched:%p[%d],ctx=%p) next:%p[%d] prev=%p[%d]",
			i,
			t,
			t->task_sched,
			(int) (t->task_sched!=NULL)?t->task_sched->pipid:-1,
			t->ctx_suspend,
			next, pn,
			prev, pp );
      } else {
	pip_info_fmesg( fp,
			"%s: QUEUE[%d]:%p "
			"(sched:%p[%d],ctx=%p) next:%p[%d] prev=%p[%d]",
			tag,
			i,
			t,
			t->task_sched,
			(int) (t->task_sched!=NULL)?t->task_sched->pipid:-1,
			t->ctx_suspend,
			next, pn,
			prev, pp );
      }
      if( u->next == u->prev && u->next != queue ) {
	if( tag == NULL ) {
	  pip_info_fmesg( fp, "BROKEN !!!!!!!!!!!!!" );
	} else {
	  pip_info_fmesg( fp, "%s: BROKEN !!!!!!!!!!!!!", tag );
	}
	break;
      }
      i++;
    }
  }
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

#define PIP_DEBUG_BUFSZ		(4096)

void pip_fprint_maps( FILE *fp ) {
  int fd = open( "/proc/self/maps", O_RDONLY );
  char buf[PIP_DEBUG_BUFSZ];

  while( 1 ) {
    ssize_t rc;
    size_t  wc;
    char *p;

    if( ( rc = read( fd, buf, PIP_DEBUG_BUFSZ ) ) <= 0 ) break;
    p = buf;
    do {
      wc = fwrite( p, 1, rc, fp );
      p  += wc;
      rc -= wc;
    } while( rc > 0 );
  }
  close( fd );
}

void pip_print_maps( void ) { pip_fprint_maps( stdout ); }

void pip_fprint_fd( FILE *fp, int fd ) {
  char idstr[64];
  char fdpath[512];
  char fdname[256];
  ssize_t sz;

  pip_idstr( idstr, 64 );
  sprintf( fdpath, "/proc/self/fd/%d", fd );
  if( ( sz = readlink( fdpath, fdname, 256 ) ) > 0 ) {
    fdname[sz] = '\0';
    fprintf( fp, "%s %d -> %s", idstr, fd, fdname );
  }
}

void pip_print_fd( int fd ) { pip_fprint_fd( stderr, fd ); }

void pip_fprint_fds( FILE *fp ) {
  DIR *dir = opendir( "/proc/self/fd" );
  struct dirent *de;
  char idstr[64];
  char fdpath[512];
  char fdname[256];
  char coe = ' ';
  ssize_t sz;

  pip_idstr( idstr, 64 );
  if( dir != NULL ) {
    int fd_dir = dirfd( dir );
    int fd;

    while( ( de = readdir( dir ) ) != NULL ) {
      sprintf( fdpath, "/proc/self/fd/%s", de->d_name );
      if( ( sz = readlink( fdpath, fdname, 256 ) ) > 0 ) {
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
    pip_idstr( idstr, 64 );
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
  (void) pip_idstr( idstr, PIP_MIDLEN );

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

static char*
pip_check_inside( FILE *fp, void *addr, void *laddr, void **startp ) {
  static char 	*line = NULL;
  size_t 	ls = 0;
  void 		*sta, *end;
  char 		perm[4], *p;

  rewind( fp );
  while( getline( &line, &ls, fp ) > 0 ) {
    if( sscanf( line, "%p-%p %4s", &sta, &end, perm ) > 0 ) {
      if( sta <= addr  &&  addr < end &&
	  sta <= laddr && laddr < end &&
	  perm[2] == 'x' ) {
	//fprintf( stderr, "sta:%p  addr:%p  laddr:%p\n", sta, addr, laddr );
	*startp = sta;
	/* then extract the filename part */
	p = line + strlen(line) - 1;
	while( *p != ' ' && *p != '\0' ) p--;
	if( *p != '\0' ) return p;
	break;
      }
    }
  }
  return NULL;
}

static char *pip_determine_pipid( pip_root_t *root,
				  FILE *fp,
				  void *addr,
				  void **startp,
				  int *pipidp ) {
  struct link_map *lm;
  char *file = NULL;
  int id, i;

  id = 0;
  if( pip_task != NULL ) {
    id = ( pip_task->pipid < 0 ) ? 0 : pip_task->pipid;
  }
  for( i=id; i<root->ntasks; i++ ) {
    for( lm = (struct link_map*) root->tasks[i].annex->loaded;
	 lm != NULL;
	 lm = lm->l_next ) {
      file = pip_check_inside( fp, addr, (void*) lm->l_addr, startp );
      if( file != NULL ) {
	*pipidp = i;
	return file;
      }
    }
  }
  for( i=0; i<id; i++ ) {
    for( lm = (struct link_map*) root->tasks[i].annex->loaded;
	 lm != NULL;
	 lm = lm->l_next ) {
      file = pip_check_inside( fp, addr, (void*) lm->l_addr, startp );
      if( file != NULL ) {
	*pipidp = i;
	return file;
      }
    }
  }
  for( lm = (struct link_map*) root->task_root->annex->loaded;
       lm != NULL;
       lm = lm->l_next ) {
    file = pip_check_inside( fp, addr, (void*) lm->l_addr, startp );
    if( file != NULL ) {
      *pipidp = PIP_PIPID_ROOT;
      return file;
    }
  }
  return NULL;
}

static void pip_print_bt( FILE *fp_maps, FILE *fp_out,
			  pip_root_t *root, int depth, void *addr ) {
  Dl_info 	info;
  char	idstr[64], fname[64], sname[64], saddr[64], off[64];
  char  *file  = NULL;
  void	*start = NULL;
  int 	pipid  = PIP_PIPID_NULL;

  idstr[0] = '\0';
  fname[0] = '\0';
  sname[0] = '\0';
  saddr[0] = '\0';
  off[0]   = '\0';

  if( root != NULL ) {
    file = pip_determine_pipid( root, fp_maps, addr, &start, &pipid );
    if( pipid == PIP_PIPID_ROOT ) {
      strcpy( idstr,  "PIPID:R " );
    } else if( pipid != PIP_PIPID_NULL ) {
      sprintf( idstr, "PIPID:%d%c", pipid, ' ' );
    }
  }
  if( dladdr( addr, &info ) != 0 ) {
    if( info.dli_fname != NULL ) {
      snprintf( fname, sizeof(fname), "%s", basename( info.dli_fname ) );
    }
    if( info.dli_sname != NULL ) {
      sprintf( sname, ":%s", info.dli_sname );
    }
    if( info.dli_saddr != NULL ) {
      sprintf( saddr, "@%p", info.dli_saddr );
      if( start != NULL ) {
	sprintf( off,   "(0x%lx)", (off_t)(info.dli_saddr-start) );
      }
    } else {
      sprintf( saddr, "@%p", addr );
      if( start != NULL ) {
	sprintf( off,   "(0x%lx)", (off_t)(addr-start) );
      }
    }
  } else {
    sprintf( fname, "%s", file );
    sprintf( saddr, "@%p", addr );
    if( start != NULL ) {
      sprintf( off,   "(0x%lx)", (off_t)(addr-start) );
    }
  }
  fprintf( fp_out, "PBT[%d] %s%s%s%s%s\n",
	   depth, idstr, fname, sname, saddr, off );
}

#define DEPTH_MAX	(32)
#define BACKTRACE(FM,FO,RT,D,N)					\
  do { if( (N)<(D) ) {						\
      void *_addr_ = __builtin_return_address(N);		\
      if( _addr_ != NULL &&					\
	  __builtin_frame_address(N) != NULL )			\
	pip_print_bt( (FM), (FO), (RT), (N), _addr_ );		\
    } else goto done; } while(0)

void pip_backtrace_fd( int depth, int fd ) {
  pip_root_t	*root = pip_root;
  FILE	 	*fp_maps = NULL;
  FILE		*fp_out  = NULL;
  char		*envroot;

  ENTER;
  if( root == NULL ) {
    if( ( envroot = getenv( PIP_ROOT_ENV ) ) != NULL &&
	*envroot != '\0' ) {
      root = (pip_root_t*) strtoll( envroot, NULL, 16 );
      if( root == NULL ||
	  !pip_is_magic_ok( root ) ||
	  !pip_is_version_ok( root ) ) {
	root = NULL;
      }
    }
  }
  fflush( NULL );

  depth = ( depth <= 0 || depth > DEPTH_MAX ) ? DEPTH_MAX : depth;
  if( ( fp_maps = fopen( "/proc/self/maps", "r" ) ) != NULL &&
      ( fp_out  = fdopen( fd, "w" ) ) != NULL ) {
    BACKTRACE(  fp_maps, fp_out, root, depth,  0 );
    BACKTRACE(  fp_maps, fp_out, root, depth,  1 );
    BACKTRACE(  fp_maps, fp_out, root, depth,  2 );
    BACKTRACE(  fp_maps, fp_out, root, depth,  3 );
    BACKTRACE(  fp_maps, fp_out, root, depth,  4 );
    BACKTRACE(  fp_maps, fp_out, root, depth,  5 );
    BACKTRACE(  fp_maps, fp_out, root, depth,  6 );
    BACKTRACE(  fp_maps, fp_out, root, depth,  7 );
    BACKTRACE(  fp_maps, fp_out, root, depth,  8 );
    BACKTRACE(  fp_maps, fp_out, root, depth,  9 );
    BACKTRACE(  fp_maps, fp_out, root, depth, 10 );
    BACKTRACE(  fp_maps, fp_out, root, depth, 11 );
    BACKTRACE(  fp_maps, fp_out, root, depth, 12 );
    BACKTRACE(  fp_maps, fp_out, root, depth, 13 );
    BACKTRACE(  fp_maps, fp_out, root, depth, 14 );
    BACKTRACE(  fp_maps, fp_out, root, depth, 15 );
    BACKTRACE(  fp_maps, fp_out, root, depth, 16 );
    BACKTRACE(  fp_maps, fp_out, root, depth, 17 );
    BACKTRACE(  fp_maps, fp_out, root, depth, 18 );
    BACKTRACE(  fp_maps, fp_out, root, depth, 19 );
    BACKTRACE(  fp_maps, fp_out, root, depth, 20 );
    BACKTRACE(  fp_maps, fp_out, root, depth, 21 );
    BACKTRACE(  fp_maps, fp_out, root, depth, 22 );
    BACKTRACE(  fp_maps, fp_out, root, depth, 23 );
    BACKTRACE(  fp_maps, fp_out, root, depth, 24 );
    BACKTRACE(  fp_maps, fp_out, root, depth, 25 );
    BACKTRACE(  fp_maps, fp_out, root, depth, 26 );
    BACKTRACE(  fp_maps, fp_out, root, depth, 27 );
    BACKTRACE(  fp_maps, fp_out, root, depth, 28 );
    BACKTRACE(  fp_maps, fp_out, root, depth, 29 );
    BACKTRACE(  fp_maps, fp_out, root, depth, 30 );
    BACKTRACE(  fp_maps, fp_out, root, depth, 31 );
  done:
    fflush( fp_out );
  }
  if( fp_maps != NULL ) fclose( fp_maps );
  RETURNV;
}

int pip_is_debug_build( void ) {
#ifdef DEBUG
  return 1;
#else
  return 0;
#endif
}
