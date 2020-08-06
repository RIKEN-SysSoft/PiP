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

#include <sys/mman.h>
#include <link.h>

typedef struct {
  char	*dsoname;
  char 	*symname;
  void	*new_addr;
} pip_phdr_itr_args;

typedef struct {
  void 	**got_entry;
  void	*old_addr;
} pip_got_patch_list_t;

static pip_got_patch_list_t    *pip_got_patch_list  = NULL;
static int			pip_got_patch_size  = 0;
static int			pip_got_patch_currp = 0;
static int			pip_got_patch_ok    = 0;

static void pip_unprotect_page( void *addr ) {
  FILE	 *fp_maps;
  size_t sz, l;
  ssize_t pgsz = sysconf( _SC_PAGESIZE );
  char	 *line;
  char	 perm[4];
  void	 *sta, *end;
  void   *page = (void*) ( (uintptr_t)addr & ~( pgsz - 1 ) );

  ASSERT( pgsz <= 0 );
  ASSERT( ( fp_maps = fopen( "/proc/self/maps", "r" ) ) == NULL );

  line = NULL;
  sz   = 0;
  while( ( l = getline( &line, &sz, fp_maps ) ) > 0 ) {
    //DBGF( "l:%d sz:%d line(%p):%s", (int)l, (int)sz, line, line );
    if( l == sz ) {
      ASSERT( ( line = (char*)realloc(line,l+1) ) == NULL );
    }
    line[l] = '\0';
    if( sscanf( line, "%p-%p %4s", &sta, &end, perm ) == 3 ) {
      if( sta <= addr  &&  addr < end ) {
	if( perm[0] == 'r' && perm[1] == 'w' ) {
	  /* the target page is already readable and writable */
	} else {
	  ASSERT( mprotect( page, pgsz, PROT_READ | PROT_WRITE ) );
	}
	break;
      }
    }
  }
  if( line != NULL ) free( line );
  fclose( fp_maps );
}

static void pip_undo_got_patch( void ) {
  int i;
  for( i=0; i<pip_got_patch_currp; i++ ) {
    void **got_entry = pip_got_patch_list[i].got_entry;
    void *old_addr   = pip_got_patch_list[i].old_addr;
    *got_entry = old_addr;
  }
  free( pip_got_patch_list );
  pip_got_patch_list  = NULL;
  pip_got_patch_size  = 0;
  pip_got_patch_currp = 0;
}

static void pip_add_got_patch( void **got_entry, void *old_addr ) {
  if( pip_got_patch_list == NULL ) {
    size_t pgsz = sysconf(_SC_PAGESIZE);
    pip_page_alloc( pgsz, (void**) &pip_got_patch_list );
    pip_got_patch_size  = pgsz / sizeof(pip_got_patch_list_t);
    pip_got_patch_currp = 0;
  } else if( pip_got_patch_currp == pip_got_patch_size ) {
    pip_got_patch_list_t *expanded;
    int			 newsz = pip_got_patch_size * 2;
    pip_page_alloc( newsz*sizeof(pip_got_patch_list_t), (void**) &expanded );
    memcpy( expanded, pip_got_patch_list, pip_got_patch_currp );
    free( pip_got_patch_list );
    pip_got_patch_list = expanded;
    pip_got_patch_size = newsz;
  }
  pip_got_patch_list[pip_got_patch_currp].got_entry = got_entry;
  pip_got_patch_list[pip_got_patch_currp].old_addr  = old_addr;
  pip_got_patch_currp ++;
}

static ElfW(Dyn) *pip_get_dynsec( struct dl_phdr_info *info ) {
  int i;
  for( i=0; i<info->dlpi_phnum; i++ ) {
    /* search DYNAMIC ELF section */
    if( info->dlpi_phdr[i].p_type == PT_DYNAMIC ) {
      return (ElfW(Dyn)*) ( info->dlpi_addr + info->dlpi_phdr[i].p_vaddr );
    }
  }
  return NULL;
}

static int pip_get_dynent_val( ElfW(Dyn) *dyn, int type ) {
  int i;
  for( i=0; dyn[i].d_tag!=0||dyn[i].d_un.d_val!=0; i++ ) {
    if( dyn[i].d_tag == type ) return dyn[i].d_un.d_val;
  }
  return 0;
}

static void *pip_get_dynent_ptr( ElfW(Dyn) *dyn, int type ) {
  int i;
  for( i=0; dyn[i].d_tag!=0||dyn[i].d_un.d_val!=0; i++ ) {
    if( dyn[i].d_tag == type ) return (void*) dyn[i].d_un.d_ptr;
  }
  return NULL;
}

static int pip_replace_clone_itr( struct dl_phdr_info *info,
				  size_t size,
				  void *args ) {
  pip_phdr_itr_args *itr_args = (pip_phdr_itr_args*) args;
  char	*dsoname  = itr_args->dsoname;
  char	*symname  = itr_args->symname;
  void	*new_addr = itr_args->new_addr;
  char	*fname, *bname;
  int	i;

  fname = (char*) info->dlpi_name;
  if( fname == NULL ) return 0;
  DBGF( "fname:%s", fname );
  bname = strrchr( fname, '/' );
  if( bname == NULL ) {
    bname = fname;
  } else {
    bname ++;		/* skp '/' */
  }

  ENTERF( "fname:%s dsoname:%s", bname, dsoname );
  if( dsoname  == NULL || *dsoname == '\0' ||
      strncmp( dsoname, bname, strlen(dsoname) ) == 0 ) {
    ElfW(Dyn) 	*dynsec = pip_get_dynsec( info );
    ElfW(Rela) 	*rela   = (ElfW(Rela)*) pip_get_dynent_ptr( dynsec, DT_JMPREL );
    ElfW(Sym)	*symtab = (ElfW(Sym)*)  pip_get_dynent_ptr( dynsec, DT_SYMTAB );
    char	*strtab = (char*)       pip_get_dynent_ptr( dynsec, DT_STRTAB );
    int		nrela   = pip_get_dynent_val(dynsec,DT_PLTRELSZ)/sizeof(ElfW(Rela));

    for( i=0; i<nrela; i++,rela++ ) {
      int symidx;
      if( sizeof(void*) == 8 ) {
        symidx = ELF64_R_SYM(rela->r_info);
      } else {
        symidx = ELF32_R_SYM(rela->r_info);
      }
      char *sym = strtab + symtab[symidx].st_name;
      DBGF( "%s : %s", sym, symname );
      if( strcmp( sym, symname ) == 0 ) {
	void	*secbase    = (void*) info->dlpi_addr;
	void	**got_entry = (void**) ( secbase + rela->r_offset );

	DBGF( "SYM[%d] '%s'  GOT:%p", i, sym, got_entry );
	pip_unprotect_page( (void*) got_entry );
	pip_add_got_patch( got_entry, *got_entry );
	*got_entry = new_addr;
	pip_got_patch_ok = 1;
	break;
      }
    }
  }
  return 0;
}

int pip_patch_GOT( char *dsoname, char *symname, void *new_addr ) {
  pip_phdr_itr_args itr_args;

  ENTER;
  itr_args.dsoname  = dsoname;
  itr_args.symname  = symname;
  itr_args.new_addr = new_addr;
  (void) dl_iterate_phdr( pip_replace_clone_itr, (void*) &itr_args );
  RETURN_NE( pip_got_patch_ok );
}

void pip_undo_patch_GOT( void ) {
  ENTER;
  pip_undo_got_patch();
  pip_got_patch_ok = 0;
  RETURNV;
}
