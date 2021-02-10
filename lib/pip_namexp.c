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
 * $PIP_VERSION: Version 2.0.0$
 *
 * $Author: Atsushi Hori (R-CCS)
 * Query:   procinproc-info+noreply@googlegroups.com
 * User ML: procinproc-users+noreply@googlegroups.com
 * $
 */

#include <pip/pip_internal.h>
#include <pip/pip_list.h>

#define PIP_HASHTAB_SZ	(1024)	/* must be power of 2 */
//#define PIP_HASHTAB_SZ	(32)	/* must be power of 2 */

typedef uint64_t 		pip_hash_t;

struct pip_named_exptab;

typedef struct pip_namexp_wait {
  pip_list_t			list;
  pip_sem_t			semaphore;
  void				*address;
  int				err;
} pip_namexp_wait_t;

typedef struct pip_namexp_entry {
  pip_list_t			list; /* hash collision list */
  struct pip_named_exptab	*namexp;
  pip_hash_t			hashval;
  char				*name;
  void				*address;
  int				flag_exported;
  int				flag_canceled;
  pip_sem_t			semaphore;
  pip_list_t			list_wait; /* waiting list */
  pip_list_t			queue_others;
} pip_namexp_entry_t;

typedef struct pip_namexp_list {
  pip_list_t			list;
  pip_spinlock_t		lock;
} pip_namexp_list_t;

typedef struct pip_named_exptab {
  volatile int			flag_closed;
  size_t			sz;
  pip_namexp_list_t		*hash_table;
} pip_named_exptab_t;


static void pip_add_namexp_entry( pip_namexp_list_t *head,
				  pip_namexp_entry_t *entry ) {
  PIP_LIST_ADD( (pip_list_t*) &head->list, (pip_list_t*) entry );
}

void pip_named_export_init( pip_task_t *task ) {
  pip_named_exptab_t 	*namexp;
  pip_namexp_list_t	*hashtab;
  int			i, sz = PIP_HASHTAB_SZ;

  DBGF( "PIPID:%d", task->pipid );
  hashtab = (pip_namexp_list_t*)
    malloc( sizeof( pip_namexp_list_t ) * sz );
  ASSERT( hashtab == NULL );
  memset( hashtab, 0, sizeof( pip_namexp_list_t ) * sz );
  for( i=0; i<sz; i++ ) {
    PIP_LIST_INIT( &(hashtab[i].list) );
    pip_spin_init( &(hashtab[i].lock) );
    //DBGF( "htab[%d]:%p", i, &(hashtab[i]) );
  }
  namexp = (pip_named_exptab_t*) malloc( sizeof( pip_named_exptab_t ) );
  ASSERT( namexp == NULL );
  memset( namexp, 0, sizeof( pip_named_exptab_t ) );
  namexp->sz         = sz;
  namexp->hash_table = hashtab;
  task->named_exptab = (void*) namexp;
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
  pip_list_t		*entry;
  pip_namexp_entry_t	*name_entry;

  DBGF( "head:%p  name:%s", head, name );
  PIP_LIST_FOREACH( (pip_list_t*) &head->list, entry ) {
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
pip_new_entry( pip_named_exptab_t *namexp, char *name, pip_hash_t hash ) {
  pip_namexp_entry_t 	*entry;

  entry = (pip_namexp_entry_t*) malloc( sizeof( pip_namexp_entry_t ) );
  if( entry == NULL ) return NULL;
  memset( entry, 0, sizeof( pip_namexp_entry_t ) );
  DBGF( "entry:%p  %s@0x%lx", entry, name, hash );
  PIP_LIST_INIT( &entry->list );
  PIP_LIST_INIT( &entry->list_wait );
  PIP_LIST_INIT( &entry->queue_others );
  pip_sem_init( &entry->semaphore );
  entry->hashval = hash;
  entry->name    = name;
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
  if( !pip_is_initialized() ) RETURN( EPERM );

  va_start( ap, format );
  hash = pip_name_hash( &name, format, ap );
  ASSERT( name == NULL );
  DBGF( "pipid:%d  name:%s  exp:%p", pip_task->pipid, name, exp );

  namexp = (pip_named_exptab_t*) pip_task->named_exptab;
  ASSERTD( namexp == NULL );

  head = pip_lock_hashtab_head( namexp, hash );
  {
    if( ( entry = pip_find_namexp( head, hash, name ) ) == NULL ) {
      /* no entry yet */
      entry = pip_new_entry( namexp, name, hash );
      if( entry == NULL ) {
	err = ENOMEM;
      } else {
	name = NULL;		/* not to free since name is in use */
	entry->address       = exp;
	entry->flag_exported = 1;
	pip_add_namexp_entry( head, entry );
      }
    } else {			/* found */
      if( entry->flag_exported ) {
	/* already exported */
	err = EBUSY;
      } else {
	/* this is a query entry */
	new = pip_new_entry( namexp, name, hash );
	if( new == NULL ) {
	  err = ENOMEM;
	} else {
	  name = NULL;		/* not to free since name is in use */
	  /* replace the quesy entry with the exported entry */
	  PIP_LIST_DEL( (pip_list_t*) entry );
	  entry->address     = exp;
	  new->address       = exp;
	  new->flag_exported = 1;
	  /* note: we cannot free this entry since it might be created by */
	  /* another PiP task and this PiP task must free it in this case */
	  pip_add_namexp_entry( head, new );
	  /* resume suspended PiP tasks on this entry */
	  pip_sem_post( &entry->semaphore );
	}
      }
    }
  }
  pip_unlock_hashtab_head( head );
  free( name );

  va_end( ap );
  RETURN( err );
}

static int pip_do_named_import( int pipid,
				void **expp,
				int flag_nblk,
				const char *format,
				va_list ap ) {
  pip_task_t		*task;
  pip_named_exptab_t 	*namexp;
  pip_namexp_entry_t 	*entry;
  pip_namexp_list_t  	*head;
  pip_namexp_wait_t	*waitp;
  pip_list_t		*list, *next;
  pip_hash_t 		hash;
  void			*address = NULL;
  char 			*name = NULL;
  int 			err = 0;

  ENTER;
  if( !pip_is_initialized() ) RETURN( EPERM );
  if( ( err = pip_check_pipid( &pipid ) ) != 0 ) RETURN( err );
  task = pip_get_task_( pipid );
  namexp = (pip_named_exptab_t*) task->named_exptab;

  hash = pip_name_hash( &name, format, ap );
  if( name == NULL ) RETURN( ENOMEM );

  DBGF( "pipid:%d  name:%s  hash:0x%lx", pipid, name, hash );
  head = pip_lock_hashtab_head( namexp, hash );
  {
    if( ( entry = pip_find_namexp( head, hash, name ) ) != NULL ) {
      if( entry->flag_exported ) { /* exported already */
	address = entry->address;
      } else {		   /* already queried, but not yet exported */
	if( flag_nblk ) {
	  err = EAGAIN;
	} else {
	  pip_namexp_wait_t	wait;

	  memset( &wait, 0, sizeof(wait) );
	  PIP_LIST_INIT( &wait.list );
	  PIP_LIST_ADD( &entry->list_wait, &wait.list );
	  pip_sem_init( &wait.semaphore );
	  pip_unlock_hashtab_head( head );
	  pip_sem_wait( &wait.semaphore );
	  /* now, it is exported */
	  err     = wait.err;
	  address = wait.address;
	  DBGF( "address:%p  err:%d", address, err );
	  goto nounlock;
	}
      }
    } else {			/* not found */
      if( namexp->flag_closed ) {
	err = ECANCELED;
      } else if( pipid == pip_task->pipid ) {
	err = EDEADLK;
      } else if( flag_nblk ) {	/* no entry yet */
	err = EAGAIN;
      } else {
	entry = pip_new_entry( namexp, name, hash );
	if( entry == NULL ) {
	  err = ENOMEM;
	} else {
	  name = NULL;		/* not to free since name is in use */
	  pip_add_namexp_entry( head, entry );
	  /* add query entry and suspend until it is exported */
	  pip_unlock_hashtab_head( head );
	  pip_sem_wait( &entry->semaphore );
	  /* now, it is exported */
	  if( entry->flag_canceled ) {
	    err = ECANCELED;
	  } else {
	    address = entry->address;
	  }
	  DBGF( "address:%p", address );

	  PIP_LIST_FOREACH_SAFE( &entry->list_wait, list, next ) {
	    PIP_LIST_DEL( list );
	    waitp = (pip_namexp_wait_t*) list;
	    waitp->err     = err;
	    waitp->address = address;
	    pip_sem_post( &waitp->semaphore );
	  }
	  free( entry->name );
	  free( entry );
	  goto nounlock;
	}
      }
    }
  }
  pip_unlock_hashtab_head( head );
 nounlock:
  free( name );
  if( !err ) {
    DBGF( "exp:%p", address );
    if( expp != NULL ) *expp = address;
  }
  RETURN( err );
}

int pip_named_import( int pipid, void **expp, const char *format, ... ) {
  va_list ap;
  int err;
  va_start( ap, format );
  err = pip_do_named_import( pipid, expp, 0, format, ap );
  va_end( ap );
  RETURN( err );
}

int pip_named_tryimport( int pipid, void **expp, const char *format, ... ) {
  va_list ap;
  int err;
  va_start( ap, format );
  err = pip_do_named_import( pipid, expp, 1, format, ap );
  va_end( ap );
  RETURN( err );
}

void pip_named_export_fin( pip_task_t *task ) {
  pip_named_exptab_t	*namexp;
  pip_list_t		*list, *next;
  int 			i;

  ENTER;
  DBGF( "PIPID:%d", task->pipid );
  namexp = (pip_named_exptab_t*) task->named_exptab;
  if( namexp != NULL ) {
    namexp->flag_closed = 1;
    for( i=0; i<namexp->sz; i++ ) {
      pip_namexp_list_t	*htab = namexp->hash_table;
      pip_namexp_list_t	*head = &(htab[i]);
      pip_spin_lock( &head->lock );
      PIP_LIST_FOREACH_SAFE( (pip_list_t*) &head->list, list, next ) {
	pip_namexp_entry_t *entry = (pip_namexp_entry_t*) list;
	PIP_LIST_DEL( (pip_list_t*) entry );
	if( !entry->flag_exported ) {
	  entry->flag_canceled = 1;
	  pip_sem_post( &entry->semaphore );
	}
      }
      pip_spin_unlock( &head->lock );
    }
  }
  RETURNV;
}

void pip_named_export_fin_all( void ) {
  pip_task_t 		*task, *root;
  pip_named_exptab_t  	*namexp;
  int i;

  ASSERTD( pip_task != pip_root->task_root );
  DBGF( "pip_root->ntasks:%d", pip_root->ntasks );
  for( i=0; i<pip_root->ntasks; i++ ) {
    DBGF( "PiP task: %d", i );
    task  = &pip_root->tasks[i];
    namexp = task->named_exptab;
    free( namexp->hash_table );
    free( namexp );
    task->named_exptab = NULL;
  }
  root = pip_root->task_root;
  (void) pip_named_export_fin( root );
  namexp = root->named_exptab;
  free( namexp->hash_table );
  free( namexp );
  root->named_exptab = NULL;
  RETURNV;
}
