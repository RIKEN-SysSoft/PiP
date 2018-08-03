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
 * Written by Atsushi HORI <ahori@riken.jp>, 2018
 */

#define _GNU_SOURCE
#include <stdarg.h>
#include <ctype.h>
#include <sched.h>
#include <stdio.h>

#ifndef DEBUG
//#define DEBUG
#endif

#include <pip.h>
#include <pip_ulp.h>
#include <pip_internal.h>
#include <pip_util.h>

#define PIP_HASHTAB_SZ	(1024)	/* must be power of 2 */

typedef uint64_t 	pip_hash_t;

typedef struct {
  pip_list_t			list;
  pip_hash_t			hashval;
  char				*name;
  void				*address;
  int				flag_exported;
  volatile  int			flag_canceled;
  pip_atomic_t			count_import;
} pip_namexp_entry_t;

typedef struct {
  pip_spinlock_t		lock;
  pip_list_t			list;
} pip_namexp_list_t;

typedef struct {
  int				pipid;
  pip_namexp_list_t		hash_table[PIP_HASHTAB_SZ];
} pip_named_exptab_t;

pip_task_t *pip_get_task_( int );
int pip_check_pipid( int *pipidp );

static void pip_namexp_lock( pip_spinlock_t *lock ) {
  DBGF( "LOCK %p", lock );
  pip_spin_lock( lock );
}

static void pip_namexp_unlock( pip_spinlock_t *lock ) {
  DBGF( "UNLOCK %p", lock );
  pip_spin_unlock( lock );
}

static void pip_add_namexp( pip_namexp_list_t *head, pip_namexp_entry_t *entry ) {
  PIP_LIST_ADD( (pip_list_t*) &head->list, (pip_list_t*) entry );
}

static void pip_del_namexp( pip_namexp_entry_t *entry ) {
  PIP_LIST_DEL( (pip_list_t*) entry );
}

int pip_named_export_init( pip_task_t *task ) {
  pip_named_exptab_t *namexp;
  int	i;

  namexp = (pip_named_exptab_t*) malloc( sizeof( pip_named_exptab_t ) );
  if( namexp == NULL ) RETURN( ENOMEM );
  memset( namexp, 0, sizeof( pip_named_exptab_t ) );

  namexp->pipid = task->pipid;
  for( i=0; i<PIP_HASHTAB_SZ; i++ ) {
    pip_spin_init( &namexp->hash_table[i].lock );
    PIP_LIST_INIT( &namexp->hash_table[i].list );
  }
  task->named_exptab = (void*) namexp;
  return 0;
}

int pip_named_export_fin( pip_task_t *task ) {
  pip_named_exptab_t	*namexp = (pip_named_exptab_t*) task->named_exptab;
  pip_namexp_list_t	*head;
  pip_namexp_entry_t	*entry;
  pip_list_t		*list, *next;
  int 			i;

  DBG;
  if( namexp != NULL ) {
    if( namexp->pipid != task->pipid ) {
      pip_err_mesg( "%s is called by PIPID:%d, but it was created by PIPID:%d",
		    __func__, task->pipid, namexp->pipid );
    }
    for( i=0; i<PIP_HASHTAB_SZ; i++ ) {
      head = &namexp->hash_table[i];

      //pip_namexp_lock( &head->lock );
      pip_spin_lock( &head->lock );
      PIP_LIST_FOREACH_SAFE( (pip_list_t*) &head->list, list, next ) {
	entry = (pip_namexp_entry_t*) list;
	if( entry->flag_exported ) {
	  pip_del_namexp( entry );
	  free( entry->name );
	  free( entry );
	} else {
	  /* the is a query entry, it must be */
	  /* free()ed by the query task or ulp */
	  entry->flag_canceled = 1;
	}
      }
      pip_spin_unlock( &head->lock );
      //pip_namexp_unlock( &head->lock );
    }
  }
  DBG;
  RETURN( 0 );
}

void pip_named_export_fin_all( void ) {
  int i;

  DBGF( "pip_root->ntasks:%d", pip_root->ntasks );
  for( i=0; i<pip_root->ntasks; i++ ) {
    DBGF( "PiP tasks: %d", i );
    free( pip_root->tasks[i].named_exptab );
    pip_root->tasks[i].named_exptab = NULL;
  }
  DBGF( "PiP root" );
  (void) pip_named_export_fin( pip_root->task_root );
  free( pip_root->task_root->named_exptab );
  pip_root->task_root->named_exptab = NULL;
}

static void pip_lock_hashtab_head( pip_namexp_list_t *head ) {
  pip_namexp_lock( &head->lock );
}

static void pip_unlock_hashtab_head( pip_namexp_list_t *head ) {
  pip_namexp_unlock( &head->lock );
}

static pip_namexp_list_t*
pip_lock_hashtab_list( pip_named_exptab_t *hashtab, pip_hash_t hash ) {
  int			idx = hash & ( PIP_HASHTAB_SZ - 1 );
  pip_namexp_list_t	*list = &hashtab->hash_table[idx];
  pip_spinlock_t	*lock = &list->lock;

  pip_namexp_lock( lock );
  return list;
}

static pip_hash_t pip_name_hash( const char *name ) {
  pip_hash_t hash = 0;
  int i;

  //DBGF( "%s", name );
  for( i=0; name[i]!='\0'; i++ ) {
    if( !isalnum( name[i] ) ) continue;
    hash <<= 1;
    hash ^= name[i];
  }
  hash += i;
  return hash;
}

static pip_namexp_entry_t*
pip_find_namexp( pip_namexp_list_t *head, pip_hash_t hash, char *name ) {
  pip_list_t		*entry;
  pip_namexp_entry_t	*name_entry;

  PIP_LIST_FOREACH( (pip_list_t*) &head->list, entry ) {
    name_entry = (pip_namexp_entry_t*) entry;
    //DBGF( "entry:%p  hash:0x%lx/0x%lx  name:%s/%s",
    //name_entry, name_entry->hashval, hash, name_entry->name, name );
    if( name_entry->hashval == hash &&
	strcmp( name_entry->name, name ) == 0 ) {
      DBGF( "FOUND -- head:%p  name='%s'", head, name );
      return name_entry;
    }
  }
  DBGF( "NOT found -- head:%p  name='%s'", head, name );
  return NULL;
}

static pip_namexp_entry_t *pip_new_entry( char *name, pip_hash_t hash ) {
  pip_namexp_entry_t *entry;

  entry = (pip_namexp_entry_t*) malloc( sizeof( pip_namexp_entry_t ) );
  DBGF( "entry:%p  %s@0x%lx", entry, name, hash );
  if( entry != NULL ) {
    PIP_LIST_INIT( &entry->list );
    entry->hashval       = hash;
    entry->name          = strdup( name );
    entry->address       = NULL;
    entry->flag_exported = 0;
    entry->flag_canceled = 0;
    entry->count_import  = 0;

    if( entry->name == NULL ) {
      free( entry );
      entry = NULL;
    }
  }
  return entry;
}

static pip_namexp_entry_t*
pip_new_export_entry( void *address, char *name, pip_hash_t hash ) {
  pip_namexp_entry_t *entry = pip_new_entry( name, hash );
  if( entry != NULL ) {
    entry->address       = address;
    entry->flag_exported = 1;
  }
  return entry;
}

int pip_named_export( void *exp, const char *format, ... ) {
  pip_named_exptab_t *namexp = (pip_named_exptab_t*) pip_task->named_exptab;
  pip_namexp_entry_t *entry, *new;
  pip_namexp_list_t  *head;
  va_list 	ap;
  pip_hash_t 	hash;
  char 		*name = NULL;
  int 		err = 0;

  va_start( ap, format );
  if( format == NULL ) {
    err = EINVAL;
    goto error;
  }
  if( vasprintf( &name, format, ap ) < 0 || name == NULL ) {
    err = ENOMEM;
    goto error;
  }
  hash = pip_name_hash( name );
  DBGF( "pipid:%d  name:%s", pip_task->pipid, name );

  head = pip_lock_hashtab_list( namexp, hash );
  {
    if( ( entry = pip_find_namexp( head, hash, name ) ) == NULL ) {
      if( ( entry = pip_new_export_entry( exp, name, hash ) ) == NULL ) {
	err = ENOMEM;
      } else {
	pip_add_namexp( head, entry );
      }
    } else if( !entry->flag_exported ) {
      /* the entry is query */
      entry->address = exp;
      if( ( new = pip_new_export_entry( exp, name, hash ) ) == NULL ) {
	err = ENOMEM;
      } else {			/* and somebody exported already, signal all */
	pip_del_namexp( entry );
	pip_add_namexp( head, new );
	/* note: we cannot free this entry since it was created by the */
	/* other PiP task and in this case that PiP task must free it  */
	//err = pip_namexp_semaphore_post_all( entry );
      }
    } else {
      /* already exists */
      err = EBUSY;
    }
  }
  pip_unlock_hashtab_head( head );
  pip_ulp_yield();
  free( name );
 error:
  va_end( ap );
  RETURN( err );
}

static int pip_named_do_import( int pipid,
				void **expp,
				int flag_nblk,
				const char *format,
				va_list ap ) {
  pip_task_t	     *task;
  pip_named_exptab_t *namexp;
  pip_namexp_entry_t *entry, *new = NULL;
  pip_namexp_list_t  *head;
  pip_hash_t 	hash;
  void		*address = NULL;
  char 		*name = NULL;
  int		flag_canceled = 0;
  int 		err = 0;

  if( format == NULL ) RETURN( EINVAL );
  if( ( err = pip_check_pipid( &pipid ) ) != 0 ) RETURN( err );
  if( !pip_is_alive( pipid ) ) RETURN( EACCES );
  task = pip_get_task_( pipid );

  namexp = (pip_named_exptab_t*) task->named_exptab;
  if( namexp == NULL ) RETURN( EACCES );
  if( vasprintf( &name, format, ap ) < 0 || name == NULL ) RETURN( ENOMEM );
  hash = pip_name_hash( name );

  DBGF( ">> pipid:%d  name:%s", pipid, name );
  head = pip_lock_hashtab_list( namexp, hash );
  {
    if( ( entry = pip_find_namexp( head, hash, name ) ) != NULL ) {
      if( entry->flag_exported ) { /* exported already */
	address = entry->address;
      } else {			/* somebody asked already */
	if( flag_nblk ) {
	  pip_unlock_hashtab_head( head );
	  err = ENOENT;
	  goto error;
	}
	while( 1 ) {
	  pip_unlock_hashtab_head( head );
	  if( ( err = pip_ulp_yield() ) != 0 ) goto error;
	  if( !pip_is_alive( pipid ) ) {
	    err = ECANCELED;
	    goto error;
	  }
	  (void) pip_lock_hashtab_head( head );
	  if( ( entry = pip_find_namexp( head, hash, name ) ) != NULL ) {
	    if( entry->flag_canceled ) {
	      err = ECANCELED;
	      break;
	    } else if( entry->flag_exported ) {
	      address       = entry->address;
	      flag_canceled = entry->flag_canceled;
	      break;
	    }
	  }
	}
      }
    } else if( flag_nblk ) {
      pip_unlock_hashtab_head( head );
      err = ENOENT;
      goto error;
    } else {			/* not yet expoerted nor nobody has asked, then add a query */
      if( ( new = pip_new_entry( name, hash ) ) == NULL ) {
	err = ENOMEM;
      } else {
	pip_add_namexp( head, new );
	while( 1 ) {
	  pip_unlock_hashtab_head( head );
	  if( ( err = pip_ulp_yield() ) != 0 ) goto error;
	  if( !pip_is_alive( pipid ) ) {
	    err = ECANCELED;
	    goto error;
	  }
	  (void) pip_lock_hashtab_head( head );
	  if( ( entry = pip_find_namexp( head, hash, name ) ) != NULL ) {
	    if( entry->flag_canceled ) {
	      err = ECANCELED;
	      break;
	    } else if( entry->flag_exported ) {
	      address       = entry->address;
	      flag_canceled = entry->flag_canceled;
	      break;
	    }
	  }
	}
      }
    }
  }
  pip_unlock_hashtab_head( head );

  if( flag_canceled ) {
    err = ECANCELED;
  } if( err == 0 && expp != NULL ) {
    *expp = address;
  }
  DBGF( "<< pipid:%d  name:%s", pipid, name );
  free( name );
  if( new != NULL && !new->flag_canceled ) {
    pip_del_namexp( new );
    free( new->name );
    free( new );
  }
 error:
  RETURN( err );
}

int pip_named_import( int pipid, void **expp, const char *format, ... ) {
  va_list ap;
  int err;
  va_start( ap, format );
  err = pip_named_do_import( pipid, expp, 0, format, ap );
  va_end( ap );
  RETURN( err );
}

int pip_named_tryimport( int pipid, void **expp, const char *format, ... ) {
  va_list ap;
  int err;
  va_start( ap, format );
  err = pip_named_do_import( pipid, expp, 1, format, ap );
  va_end( ap );
  RETURN( err );
}
