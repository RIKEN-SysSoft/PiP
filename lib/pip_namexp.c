/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2018
 */

#define _GNU_SOURCE

#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>

#include <pip.h>
#include <pip_internal.h>
#include <pip_util.h>

#define PIP_PAUSE_LOOP		(1000)

typedef uint64_t 	pip_hash_t;

typedef struct {
  pip_list_t		list;
  pip_semaphore_t	sem_export;
  pip_semaphore_t	sem_import;
  pip_atomic_t		count_wait;
  pip_hash_t		hashval;
  const char		*name;
  void			*address;
  int			flag_exported;
  volatile  int		flag_canceled;
} pip_namexp_entry_t;

typedef struct {
  pip_spinlock_t	lock;
  pip_list_t		list;
} pip_namexp_list_t;

#define PIP_HASHTAB_SZ	(1024)

typedef struct {
  int			hashtab_sz;
  int			num_entries;
  pip_namexp_list_t	*hash_table;
  pip_list_t		list_free;
  pip_spinlock_t	lock_free;
} pip_named_exptab_t;

static pip_named_exptab_t	*pip_named_exptab = NULL;

static void
pip_add_namexp( pip_namexp_list_t *root, pip_namexp_entry_t *entry ) {
  PIP_LIST_ADD( (pip_list_t*) &root->list, (pip_list_t*) entry );
}

static void pip_del_namexp( pip_namexp_entry_t *entry ) {
  PIP_LIST_DEL( (pip_list_t*) entry );
}

int pip_named_export_init( int ntasks ) {
  pip_named_exptab_t *hashtab;
  int	i;
  int	err = 0;

  if( pip_named_exptab == NULL ) {
    pip_named_exptab = malloc( sizeof( pip_named_exptab_t ) * ntasks );
    if( pip_named_exptab == NULL ) {
      err = ENOMEM;
    } else {
      memset( pip_named_exptab, 0, sizeof( pip_named_exptab_t ) * ntasks );
      for( i=0; i<ntasks; i++ ) {
	hashtab = &pip_named_exptab[i];
	memset( (void*) hashtab, 0, sizeof( pip_named_exptab_t ) );
	hashtab->hashtab_sz = PIP_HASHTAB_SZ;
	hashtab->hash_table = malloc(sizeof(pip_namexp_list_t)*PIP_HASHTAB_SZ);
	if( hashtab->hash_table == NULL ) {
	  err = ENOMEM;
	} else {
	  for( i=0; i<PIP_HASHTAB_SZ; i++ ) {
	    pip_spin_init( &hashtab->hash_table[i].lock );
	    PIP_LIST_INIT( &hashtab->hash_table[i].list );
	  }
	}
      }
    }
    if( err != 0 ) {		/* free allocated memory */
      if( pip_named_exptab != NULL ) {
	for( i=0; i<ntasks; i++ ) {
	  hashtab = &pip_named_exptab[i];
	  if( hashtab->hash_table != NULL ) free( hashtab->hash_table );
	}
	free( pip_named_exptab );
	pip_named_exptab = NULL;
      }
    }
  }
  return err;
}

void pip_named_export_fin( int pipid ) {
  pip_named_exptab_t 	*hashtab;
  pip_namexp_list_t	*head;
  pip_namexp_entry_t	*entry;
  pip_list_t		*list;
  int 			i, j;

  hashtab = &pip_named_exptab[pipid];
  for( i=0; i<hashtab->hashtab_sz; i++ ) {
    head = &hashtab->hash_table[i];
    pip_spin_lock( &head->lock );
    {
      PIP_LIST_FOREACH( (pip_list_t*) &head->list, list ) {
	entry = (pip_namexp_entry_t*) list;
	pip_del_namexp( entry );
	if( entry->flag_exported ) { /* if true, allocated by myself */
	  free( entry );
	} else {	    /* otherwise, the entry cannot be freed */
	  int count = entry->count_wait;
	  entry->flag_canceled = 1;
	  for( j=0; j<count; j++ ) {
	    pip_semaphore_post( &entry->sem_export );
	  }
	}
      }
    }
    pip_spin_unlock( &head->lock );
  }
}

int pip_named_export_fin_all( int ntasks ) {
  pip_named_exptab_t 	*hashtab;
  pip_namexp_list_t	*head;
  pip_namexp_entry_t	*entry;
  pip_list_t		*list;
  int 			i, j;
  int			err = 0;

  for( i=0; i<ntasks+1; i++ ) {
    hashtab = &pip_named_exptab[i];
    for( j=0; j<hashtab->hashtab_sz; j++ ) {
      head = &hashtab->hash_table[i];
      if( !PIP_LIST_ISEMPTY( (pip_list_t*) head->list ) ) err = EBUSY;
    }
    if( !err ) free( hashtab->hash_table );
  }
  if( !err ) {
    free( pip_named_exptab );
    pip_named_exptab = NULL;
  }
  RETURN( err );
}

pip_namexp_list_t*
pip_lock_hashtab_list( pip_named_exptab_t *hashtab, pip_hash_t hash ) {
  int			idx = hash & ( PIP_HASHTAB_SZ - 1 );
  pip_namexp_list_t	*head = &hashtab->hash_table[idx];
  pip_spin_lock( &head->lock );
  return head;
}

void pip_unlock_hashtab_list( pip_namexp_list_t *head ) {
  pip_spin_unlock( &head->lock );
}

static pip_hash_t pip_name_hash( char *name ) {
  pip_hash_t hash = 0;
  int i;
  for( i=0; name[i]!='\0'; i++ ) {
    if( isalnum( name[i] ) ) {
      hash <<= 1;
      hash ^= name[i];
    }
  }
  hash += i;
  return hash;
}

static pip_namexp_entry_t*
pip_find_namexp( pip_namexp_list_t *root, pip_hash_t hash, char *name ) {
  pip_list_t		*entry;
  pip_namexp_entry_t	*name_entry;

  DBGF( "root:%p  hash=0x%Lx  name='%s'", root, hash, name );
  PIP_LIST_FOREACH( (pip_list_t*) &root->list, entry ) {
    DBGF( "entry:%p", entry );
    name_entry = (pip_namexp_entry_t*) entry;
    if( name_entry->hashval == hash &&
	strcmp( name_entry->name, name ) == 0 ) {
      DBGF( "found" );
      return name_entry;
    }
  }
  DBGF( "NOT FOUND" );
  return NULL;
}

static pip_namexp_entry_t*
pip_new_entry( const char *name, pip_hash_t hash ) {
  pip_namexp_entry_t *entry;

  entry = (pip_namexp_entry_t*) malloc( sizeof( pip_namexp_entry_t ) );
  if( entry != NULL ) {
    PIP_LIST_INIT( &entry->list );
    pip_semaphore_init( &entry->sem_export, 0 );
    pip_semaphore_init( &entry->sem_import, 0 );
    entry->count_wait   = 0;
    entry->hashval      = hash;
    entry->name         = name;
    entry->address      = NULL;
    entry->flag_exported = 0;
  }
  return entry;
}

static pip_namexp_entry_t*
pip_new_export_entry( void *address, const char *name, pip_hash_t hash ) {
  pip_namexp_entry_t *entry = pip_new_entry( name, hash );
  if( entry != NULL ) {
    entry->address       = address;
    entry->flag_exported = 1;
  }
  return entry;
}

int pip_named_export( void *exp, const char *format, ... ) {
  va_list ap;
  va_start( ap, format );
  pip_named_exptab_t *hashtab = &pip_named_exptab[pip_task->pipid];
  pip_namexp_entry_t *entry, *new;
  pip_namexp_list_t  *root;
  pip_hash_t 	hash;
  char 		*name;
  int 		err = 0;

  DBG;
  name = NULL;
  if( vasprintf( &name, format, ap ) < 0 || name == NULL ) RETURN( ENOMEM );
  hash = pip_name_hash( name );

  root = pip_lock_hashtab_list( hashtab, hash );
  {
    if( ( entry = pip_find_namexp( root, hash, name ) ) == NULL ) {
      if( ( entry = pip_new_export_entry( exp, name, hash ) ) == NULL ) {
	err = ENOMEM;
      } else {
	pip_add_namexp( root, entry );
      }
    } else if( !entry->flag_exported ) {
      entry->address = exp;
      if( ( new = pip_new_export_entry( exp, name, hash ) ) == NULL ) {
	err = ENOMEM;
      } else {
	int i, count = entry->count_wait;
	pip_del_namexp( entry );
	pip_add_namexp( root, new );
	for( i=0; i<count; i++ ) {
	  pip_semaphore_post( &entry->sem_export );
	}
      }
    } else {
      free( name );
      err = EBUSY;
    }
  }
  pip_unlock_hashtab_list( root );
  RETURN( err );
}

int pip_check_pipid( int *pipidp );
pip_task_t *pip_get_task_( int pipid );

int pip_named_import( int pipid, void **expp, const char *format, ... ) {
  va_list ap;
  va_start( ap, format );

  pip_named_exptab_t *hashtab;
  pip_namexp_entry_t *entry;
  pip_namexp_list_t  *root;
  pip_task_t	*task;
  pip_hash_t 	hash;
  char 		*name;
  int 		err = 0;

  if( ( err = pip_check_pipid( &pipid ) ) != 0 ) RETURN( err );
  if( ( err = pip_is_alive( pipid )     ) != 0 ) RETURN( err );
  task = pip_get_task_( pipid );
  hashtab = &pip_named_exptab[task->pipid];

  name = NULL;
  if( vasprintf( &name, format, ap ) < 0 || name == NULL ) RETURN( ENOMEM );
  hash = pip_name_hash( name );

  root = pip_lock_hashtab_list( hashtab, hash );
  {
    if( ( entry = pip_find_namexp( root, hash, name ) ) != NULL ) {
      free( name );
      if( entry->flag_exported ) { /* exported already */
	if( expp != NULL ) *expp = (void*) entry->address;
      } else {			/* somebody asked already */
	pip_unlock_hashtab_list( root );

	(void) pip_atomic_add_and_fetch( &entry->count_wait, 1 );
	pip_semaphore_wait( &entry->sem_export );
	*expp = entry->address;
	if( pip_atomic_sub_and_fetch( &entry->count_wait, 1 ) == 0 ) {
	  /* I am the last one wating, so I will let entry's creator */
	  /* so that the creator can free the entry                  */
	  pip_semaphore_post( &entry->sem_import );
	}
	return err;
      }
    } else {			/* not yet expoerted nor nobody ask */
      if( ( entry = pip_new_entry( name, hash ) ) == NULL ) {
	err = ENOMEM;
      } else {
	pip_add_namexp( root, entry );
	pip_unlock_hashtab_list( root );

	pip_atomic_add_and_fetch( &entry->count_wait, 1 );
	pip_semaphore_wait( &entry->sem_export );
	if( !entry->flag_canceled ) {
	  *expp = entry->address;
	} else {
	  err = ECANCELED;
	}
	(void) pip_atomic_sub_and_fetch( &entry->count_wait, 1 );
	/* since this entry is created by me, I have to free it */
	/* before free, wait for all the other guys' acks       */
	pip_semaphore_wait( &entry->sem_import );
	/* OK, now I can free it */
	free( name );
	pip_semaphore_fin( &entry->sem_export );
	pip_semaphore_fin( &entry->sem_import );
	free( entry );
	return err;
      }
    }
  }
  pip_unlock_hashtab_list( root );
  DBG;

  RETURN( err );
}
