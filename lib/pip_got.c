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
#include <stdarg.h>

pip_spinlock_t pip_lock_got_clone PIP_PRIVATE;

typedef int(*pip_clone_syscall_t)(int(*)(void*), void*, int, void*, ...);

static pip_clone_syscall_t  pip_clone_original  = NULL;
static pip_clone_syscall_t *pip_clone_got_entry = NULL;

typedef struct {
  char	*dsoname;
  char 	*symbol;
} pip_phdr_itr_args;

static int
pip_clone( int(*fn)(void*), void *child_stack, int flags, void *args, ... ) {
  pid_t		 tid = pip_gettid();
  pip_spinlock_t oldval;
  int 		 retval = -1;

  while( 1 ) {
    oldval = pip_spin_trylock_wv( &pip_lock_got_clone, PIP_LOCK_OTHERWISE );
    if( oldval == tid ) {
      /* called and locked by PiP lib */
      break;
    }
    if( oldval == PIP_LOCK_UNLOCKED ) { /* lock succeeds */
      /* not called by PiP lib */
      break;
    }
  }
  DBG;
  {
    va_list ap;
    va_start( ap, args );
    pid_t *ptid = va_arg( ap, pid_t*);
    void  *tls  = va_arg( ap, void*);
    pid_t *ctid = va_arg( ap, pid_t*);

    if( oldval == tid ) {
      flags &= ~(CLONE_FS);	 /* 0x00200 */
      flags &= ~(CLONE_FILES);	 /* 0x00400 */
      flags &= ~(CLONE_SIGHAND); /* 0x00800 */
      flags &= ~(CLONE_THREAD);	 /* 0x10000 */
      flags &= ~0xff;
      flags |= CLONE_VM;
      flags |= CLONE_SETTLS;  /* do not reset the CLONE_SETTLS flag */
      flags |= CLONE_PTRACE;
      flags |= SIGCHLD;
    }
    retval = pip_clone_original( fn, child_stack, flags, args, ptid, tls, ctid );

    va_end( ap );
  }
  pip_spin_unlock( &pip_lock_got_clone );
  return retval;
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
    if( dyn[i].d_tag == type ) {
      return (void*) dyn[i].d_un.d_ptr;
    }
  }
  return NULL;
}

static int pip_replace_clone_itr( struct dl_phdr_info *info,
				  size_t size,
				  void *args ) {
  pip_phdr_itr_args *itr_args = (pip_phdr_itr_args*) args;
  char	*dsoname = itr_args->dsoname;
  char	*symname = itr_args->symbol;
  char	*fname, *bname;
  int	got_len, i, j;
  void	*got_top;

  fname = (char*) info->dlpi_name;
  if( fname == NULL ) return 0;
  bname = strrchr( fname, '/' );
  if( bname == NULL ) {
    bname = fname;
  } else {
    bname ++;		/* skp '/' */
  }

  ENTERF( "fname:%s dsoname:%s", bname, dsoname );
  if( dsoname == NULL ||
      strncmp( dsoname, bname, strlen(dsoname) ) == 0 ) {
    DBG;
    ElfW(Dyn) *dynsec = pip_get_dynsec( info );
    ElfW(Rela) *rela  = (ElfW(Rela)*) pip_get_dynent_ptr( dynsec, DT_JMPREL   );
    int		nrela = pip_get_dynent_val( dynsec, DT_RELASZ ) / sizeof( ElfW(Rela) );
    ElfW(Sym)	*symtab = (ElfW(Sym)*) pip_get_dynent_ptr( dynsec, DT_SYMTAB );
    char	*strtab = (char*) pip_get_dynent_ptr( dynsec, DT_STRTAB );
    void	*secbase = (void*) info->dlpi_addr;

    for( i=0; i<nrela; i++, rela++ ) {
      int symidx;
      if( sizeof(void*) == 8 ) {
        symidx = ELF64_R_SYM(rela->r_info);
      } else {
        symidx = ELF32_R_SYM(rela->r_info);
      }
      char *sym = strtab + symtab[symidx].st_name;
      DBGF( "SYM[%d] '%s'", i, sym );
      if( strcmp( sym, symname ) == 0 ) {
	DBGF( "    GOT: %p", secbase + rela->r_offset );
      }
    }
    /* Obtain GOT address and # GOT entries */
    got_len = pip_get_dynent_val( dynsec, DT_RELAENT );
    got_top = pip_get_dynent_ptr( dynsec, DT_PLTGOT  );

    if( got_len > 0 && got_top != NULL ) {
      /* find target function entry in the GOT */
      for( j=0; j<got_len; j++ ) {
	void	**got_entry = (void**) ( got_top + sizeof(void*) * j );
	void	*faddr = *got_entry;

	if( faddr != NULL ) {
	  Dl_info 	di;

	  memset( &di, 0, sizeof(di) );
	  dladdr( faddr, &di );
	  if( di.dli_sname != NULL ) {
	    DBGF( "dli_sname:%s  faddr:%p  GOT:%p", di.dli_sname, faddr, got_entry );
	    if( strncmp( di.dli_sname, symname, strlen(symname) ) == 0 ) {
	      /* then replace the GOT enntry */
	      pip_clone_original  = (pip_clone_syscall_t)  faddr;
		pip_clone_got_entry = (pip_clone_syscall_t*) got_entry;
		//RETURN( 0 );
	    }
	  }
	}
      }
    }
  }
  return 0;
}

static void *pip_dummy( void *dummy ) {
  return NULL;
}

int pip_replace_clone( void ) {
  pip_phdr_itr_args itr_args;

  ENTER;
  if( pip_clone_original  == NULL &&
      pip_clone_got_entry == NULL ) {
    {
      pthread_t th;
      pthread_create( &th, NULL, pip_dummy, NULL );
      pthread_join( th, NULL );
    }
    /* then find the GOT entry of the clone */
    itr_args.dsoname = "libpthread.so";
    itr_args.symbol  = "__clone";
    (void) dl_iterate_phdr( pip_replace_clone_itr, (void*) &itr_args );

    if( pip_clone_original  != NULL &&
	pip_clone_got_entry != NULL ) {
      /* if succeeded */
      DBGF( "clone_orig:%p   got:%p", pip_clone_original, pip_clone_got_entry );
      *pip_clone_got_entry = (void*) pip_clone;
      pip_spin_init( &pip_lock_got_clone );
    } else {
      RETURN( ENOENT );
    }
  }
  RETURN( 0 );
}

void pip_undo_reoplace_clone( void ) {
#ifdef AH
  if( pip_clone_got_entry != NULL ) {
    *pip_clone_got_entry = (void*) clone;
    pip_clone_original  = NULL;
    pip_clone_got_entry = NULL;
  }
#endif
}
