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

#include <pip.h>
#include <pip_blt.h>
#include <pip_internal.h>

#define PIP_HASHTAB_SZ	(1024)	/* must be power of 2 */
//#define PIP_HASHTAB_SZ	(32)	/* must be power of 2 */

typedef uint64_t 		pip_hash_t;

struct pip_named_exptab;

typedef struct pip_namexp_wait {
  pip_task_t			list;
  void				*address;
  int				err;
} pip_namexp_wait_t;

typedef struct pip_namexp_entry {
  pip_task_t			list; /* hash collision list */
  struct pip_named_exptab	*namexp;
  pip_hash_t			hashval;
  char				*name;
  void				*address;
  int				flag_exported;
  int				flag_canceled;
  pip_task_t			list_wait; /* waiting list */
  pip_task_queue_t		queue_owner;
  pip_task_queue_t		queue_others;
} pip_namexp_entry_t;

typedef struct pip_namexp_list {
  pip_task_t			list;
  pip_spinlock_t		lock;
} pip_namexp_list_t;

typedef struct pip_named_exptab {
  size_t			sz;
  pip_namexp_list_t		*hash_table;
} pip_named_exptab_t;


static void pip_add_namexp_entry( pip_namexp_list_t *head,
				  pip_namexp_entry_t *entry ) {
  PIP_LIST_ADD( (pip_task_t*) &head->list, (pip_task_t*) entry );
}

static void pip_del_namexp_entry( pip_namexp_entry_t *entry ) {
  PIP_LIST_DEL( (pip_task_t*) entry );
}

int pip_named_export_init_( pip_task_internal_t *taski ) {
  pip_named_exptab_t 	*namexp;
  pip_namexp_list_t	*hashtab;
  int			i, sz = PIP_HASHTAB_SZ;

  hashtab = (pip_namexp_list_t*)
    PIP_MALLOC( sizeof( pip_namexp_list_t ) * sz );
  memset( hashtab, 0, sizeof( pip_namexp_list_t ) * sz );
  for( i=0; i<sz; i++ ) {
    PIP_LIST_INIT( &(hashtab[i].list) );
    pip_spin_init( &(hashtab[i].lock) );
    //DBGF( "htab[%d]:%p", i, &(hashtab[i]) );
  }
  namexp = (pip_named_exptab_t*) PIP_MALLOC( sizeof( pip_named_exptab_t ) );
  if( namexp == NULL ) RETURN( ENOMEM );
  memset( namexp, 0, sizeof( pip_named_exptab_t ) );
  namexp->sz         = sz;
  namexp->hash_table = hashtab;
  taski->annex->named_exptab = (void*) namexp;
  return 0;
}

static pip_namexp_list_t*
pip_lock_hashtab_head( pip_named_exptab_t *namexp, pip_hash_t hash ) {
  int			idx = hash & ( namexp->sz - 1 );
  pip_namexp_list_t	*htab = namexp->hash_table;
  pip_namexp_list_t	*head = &(htab[idx]);
  //DBGF( "namexp:%p", namexp );
  //DBGF( "head:%p  hash:0x%lu  sz:%lu  idx:%d", head, hash, namexp->sz, idx );
  pip_spin_lock( &head->lock );
  return head;
}

static void pip_unlock_hashtab_head( pip_namexp_list_t *head ) {
  pip_spin_unlock( &head->lock );
}

static pip_hash_t
pip_name_hash( char **namep, const char *format, va_list ap ) {
  pip_hash_t	hash = 0;
  char		*name = NULL;
  int 		i;

  if( format == NULL ) {
    if( ( name = strdup( "" ) ) == NULL ) return 0;
  } else {
    if( vasprintf( &name, format, ap ) == 0 || name == NULL ) return 0;
    for( i=0; name[i]!='\0'; i++ ) {
      hash <<= 1;
      hash ^= name[i];
    }
    hash += i;
  }
  *namep = name;
  return hash;
}

static pip_namexp_entry_t*
pip_find_namexp( pip_namexp_list_t *head, pip_hash_t hash, char *name ) {
  pip_task_t		*entry;
  pip_namexp_entry_t	*name_entry;

  PIP_LIST_FOREACH( (pip_task_t*) &head->list, entry ) {
    name_entry = (pip_namexp_entry_t*) entry;
    DBGF( "entry:%p  hash:0x%lx/0x%lx  name:%s/%s",
	  name_entry, name_entry->hashval, hash, name_entry->name, name );
    if( name_entry->hashval == hash &&
	strcmp( name_entry->name, name ) == 0 ) {
      DBGF( "FOUND -- head:%p  name='%s'", head, name );
      return name_entry;
    }
  }
  DBGF( "NOT found -- head:%p  name='%s'", head, name );
  return NULL;
}

static pip_namexp_entry_t *
pip_new_entry( pip_named_exptab_t *namexp, char **namep, pip_hash_t hash ) {
  pip_namexp_entry_t 	*entry;

  entry = (pip_namexp_entry_t*) PIP_MALLOC( sizeof( pip_namexp_entry_t ) );
  if( entry == NULL ) return NULL;
  memset( entry, 0, sizeof( pip_namexp_entry_t ) );
  DBGF( "entry:%p  %s@0x%lx", entry, *namep, hash );
  PIP_LIST_INIT( &entry->list );
  PIP_LIST_INIT( &entry->list_wait );
  pip_task_queue_init( &entry->queue_owner, NULL );
  pip_task_queue_init( &entry->queue_others, NULL );
  entry->hashval = hash;
  entry->name    = *namep;
  *namep = NULL;
  return entry;
}

static pip_namexp_entry_t*
pip_new_export_entry( pip_named_exptab_t *namexp,
		      void *address,
		      char **namep,
		      pip_hash_t hash ) {
  pip_namexp_entry_t *entry = pip_new_entry( namexp, namep, hash );
  if( entry != NULL ) {
    entry->address = address;
    entry->flag_exported = 1;
  }
  return entry;
}

int pip_named_export( void *exp, const char *format, ... ) {
  pip_named_exptab_t *namexp;
  pip_namexp_entry_t *entry, *new;
  pip_namexp_list_t  *head = NULL;
  va_list 	ap;
  pip_hash_t 	hash;
  char 		*name = NULL;
  int 		err = 0;

  ENTER;
  va_start( ap, format );
  hash = pip_name_hash( &name, format, ap );
  if( name == NULL ) {
    err = ENOMEM;
    goto error;
  }
  DBGF( "pipid:%d  name:%s", pip_task_->pipid, name );

  namexp = (pip_named_exptab_t*) pip_task_->annex->named_exptab;
  head = pip_lock_hashtab_head( namexp, hash );
  {
    if( ( entry = pip_find_namexp( head, hash, name ) ) == NULL ) {
      /* no entry yet */
      entry = pip_new_export_entry( namexp, exp, &name, hash );
      if( entry == NULL ) {
	err = ENOMEM;
      } else {
	pip_add_namexp_entry( head, entry );
      }
    } else if( entry->flag_exported ) {
      /* already exported */
      err = EBUSY;
    } else {
      /* the entry is for query (i.e. this is the first export) */
      new = pip_new_export_entry( namexp, exp, &name, hash );
      if( new == NULL ) {
	err = ENOMEM;
      } else {
	pip_del_namexp_entry( entry );
	entry->address = exp;
	/* note: we cannot free this entry since it might be created by */
	/* another PiP task and this PiP task must free it in this case */
	pip_add_namexp_entry( head, new );
	/* resume suspended PiP tasks on this entry */
	err = pip_dequeue_and_resume_nolock_( &entry->queue_owner, NULL );
      }
    }
  }
  pip_unlock_hashtab_head( head );
  PIP_FREE( name );
 error:
  va_end( ap );
  RETURN( err );
}

static int pip_named_import_( int pipid,
			      void **expp,
			      int flag_nblk,
			      const char *format,
			      va_list ap ) {
  void pip_unlock_hashtab( void *arg ) {
    pip_namexp_list_t  	*head = (pip_namexp_list_t*) arg;
    pip_unlock_hashtab_head( head );
  }
  pip_task_internal_t	*taski;
  pip_named_exptab_t 	*namexp;
  pip_namexp_entry_t 	*entry;
  pip_namexp_list_t  	*head;
  pip_namexp_wait_t	wait;
  pip_namexp_wait_t	*waitp;
  pip_task_t		*list, *next;
  pip_hash_t 		hash;
  void			*address = NULL;
  char 			*name = NULL;
  int 			n, err = 0;

  ENTER;
  if( ( err = pip_check_pipid_( &pipid ) ) != 0 ) RETURN( err );
  taski = pip_get_task_( pipid );
  namexp = (pip_named_exptab_t*) taski->annex->named_exptab;
  ASSERT( namexp == NULL );

  hash = pip_name_hash( &name, format, ap );
  if( name == NULL ) RETURN( ENOMEM );

  DBGF( "pipid:%d  name:%s  hash:0x%lx", pipid, name, hash );
  head = pip_lock_hashtab_head( namexp, hash );
  {
    if( ( entry = pip_find_namexp( head, hash, name ) ) != NULL ) {
      if( entry->flag_exported ) { /* exported already */
	address = entry->address;
      } else {
	/* already queried, but not yet exported */
	if( flag_nblk ) {
	  err = EAGAIN;
	} else {
	  PIP_LIST_ADD( &entry->list_wait, (pip_task_t*) &wait );
	  pip_suspend_and_enqueue_nolock_( &entry->queue_others,
					   pip_unlock_hashtab,
					   (void*) head );
	  /* now, it is exported */
	  /* note that the lock is unlocked !! */
	  err     = wait.err;
	  address = wait.address;
	  goto nounlock;
	}
      }
    } else if( flag_nblk ) {	/* no entry yet */
      err = EAGAIN;
    } else {
      if( ( entry = pip_new_entry( namexp, &name, hash ) ) == NULL ) {
	err = ENOMEM;
      } else {
	pip_add_namexp_entry( head, entry );
	/* add query entry and suspend until it is exported */
	/* when the query is enqueued, lock in unlocked */
	pip_suspend_and_enqueue_nolock_( &entry->queue_owner,
					 (void*) pip_unlock_hashtab,
					 (void*) head );
	/* now, it is exported */
	/* note that the lock is unlocked !! */
	if( entry->flag_canceled ) {
	  err = ECANCELED;
	} else {
	  address = entry->address;
	}
	PIP_LIST_FOREACH_SAFE( &entry->list_wait, list, next ) {
	  waitp = (pip_namexp_wait_t*) list;
	  waitp->err     = err;
	  waitp->address = address;
	}
	n = PIP_TASK_ALL;
	err = pip_dequeue_and_resume_N_nolock_(&entry->queue_others,NULL,&n);

	PIP_FREE( entry->name );
	PIP_FREE( entry );
	goto nounlock;
      }
    }
  }
  pip_unlock_hashtab_head( head );
 nounlock:
  PIP_FREE( name );
  if( !err ) *expp = address;
  RETURN( err );
}

int pip_named_import( int pipid, void **expp, const char *format, ... ) {
  va_list ap;
  int err;
  va_start( ap, format );
  err = pip_named_import_( pipid, expp, 0, format, ap );
  va_end( ap );
  RETURN( err );
}

int pip_named_tryimport( int pipid, void **expp, const char *format, ... ) {
  va_list ap;
  int err;
  va_start( ap, format );
  err = pip_named_import_( pipid, expp, 1, format, ap );
  va_end( ap );
  RETURN( err );
}

void pip_named_export_fin_( pip_task_internal_t *taski ) {
  pip_named_exptab_t	*namexp;
  pip_task_t		*list, *next;
  int 			i, err;

  ENTER;
  DBGF( "PIPID:%d", taski->pipid );
  namexp = (pip_named_exptab_t*) taski->annex->named_exptab;
  if( namexp != NULL ) {
    for( i=0; i<namexp->sz; i++ ) {
      pip_namexp_list_t	*htab = namexp->hash_table;
      pip_namexp_list_t	*head = &(htab[i]);
      pip_spin_lock( &head->lock );
      PIP_LIST_FOREACH_SAFE( (pip_task_t*) &head->list, list, next ) {
	pip_namexp_entry_t *entry = (pip_namexp_entry_t*) list;
	pip_del_namexp_entry( entry );
	if( entry->flag_exported ) {
	  PIP_FREE( entry->name );
	  PIP_FREE( entry );
	} else {
	  /* this is a query entry, it must be free()ed by the query task */
	  entry->flag_canceled = 1;
	  err = pip_dequeue_and_resume_nolock_( &entry->queue_owner, NULL );
	  ASSERT( err );
	}
      }
      pip_spin_unlock( &head->lock );
    }
  }
  RETURNV;
}

void pip_named_export_fin_all_( void ) {
  int i;

  DBGF( "pip_root->ntasks:%d", pip_root_->ntasks );
  for( i=0; i<pip_root_->ntasks; i++ ) {
    DBGF( "PiP task: %d", i );
    PIP_FREE( pip_root_->tasks[i].annex->named_exptab );
    pip_root_->tasks[i].annex->named_exptab = NULL;
  }
  DBGF( "PiP root" );
  (void) pip_named_export_fin_( pip_root_->task_root );
  PIP_FREE( pip_root_->task_root->annex->named_exptab );
  pip_root_->task_root->annex->named_exptab = NULL;
  RETURNV;
}
