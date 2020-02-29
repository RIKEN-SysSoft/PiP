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

static int
pip_replace_clone_syscall( struct dl_phdr_info *info, size_t size, void *notused ) {
  char	*soname, *basename;
  char	*libcname = "libpthread.so";
  int	got_len, i, j;
  void	*got_top;

  soname = (char*) info->dlpi_name;
  if( soname == NULL ) return 0;
  basename = strrchr( soname, '/' );
  if( basename == NULL ) {
    basename = soname;
  } else {
    basename ++;		/* skp '/' */
  }

  ENTERF( "basename:%s", basename );
  if( strncmp( libcname, basename, strlen(libcname) ) == 0 ) {
    DBG;
    for( i=0; i<info->dlpi_phnum; i++ ) {
      /* search DYNAMIC ELF section */
      if( info->dlpi_phdr[i].p_type == PT_DYNAMIC ) {
	ElfW(Dyn) *dyn = (ElfW(Dyn)*)
	  ( (void*) (info->dlpi_addr + info->dlpi_phdr[i].p_vaddr) );
	got_len = -1;
	got_top = NULL;
	/* Obtain GOT address and # GOT entries */
	for( j=0; dyn[j].d_tag!=0||dyn[j].d_un.d_val!=0; j++ ) {
	  if( dyn[j].d_tag == DT_RELAENT ) got_len = (int)   dyn[j].d_un.d_val;
	  if( dyn[j].d_tag == DT_PLTGOT  ) got_top = (void*) dyn[j].d_un.d_ptr;
	}
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
		if( strcmp( di.dli_sname, "clone" ) == 0 ) {
		  /* then replace the GOT enntry */
		  pip_clone_original  = (pip_clone_syscall_t)  faddr;
		  pip_clone_got_entry = (pip_clone_syscall_t*) got_entry;
		  RETURN( 0 );
		}
	      }
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
  ENTER;
  if( pip_clone_original  == NULL &&
      pip_clone_got_entry == NULL ) {
    pthread_t th;
    pthread_create( &th, NULL, pip_dummy, NULL );
    pthread_join( th, NULL );
    /* then find the GOT entry of the clone */
    (void) dl_iterate_phdr( pip_replace_clone_syscall, NULL );

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
