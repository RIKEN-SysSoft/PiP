/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019, 2020$
 * $PIP_VERSION: Version 1.2.0$
 * $PIP_license$
 */

#include <string.h>
#include <libgen.h>

#include <test.h>

static int test_pip_init( char **argv, char *base )
{
  int pipid, ntasks, rv;
  void *exp;

  ntasks = NTASKS;
  exp = NULL;
  rv = pip_init( &pipid, &ntasks, &exp, 0 );
  if ( rv != 0 ) {
    fprintf( stderr, "pip_init: %s", strerror( rv ) );
    return EXIT_FAIL;
  }
  
  return EXIT_PASS;
}

static int test_pip_init_preload( char **argv, char *base )
{
  int pipid, ntasks, rv;
  void *exp;

  ntasks = NTASKS;
  exp = NULL;
  rv = pip_init( &pipid, &ntasks, &exp, PIP_MODE_PROCESS_PRELOAD );
  if ( rv != 0 ) {
    fprintf( stderr, "pip_init: %s", strerror( rv ) );
    return EXIT_FAIL;
  }
  
  return EXIT_PASS;
}

static int test_twice( char **argv, char *base )
{
  int pipid, ntasks, rv;
  void *exp;

  ntasks = NTASKS;
  exp = NULL;
  rv = pip_init( &pipid, &ntasks, &exp, 0 );
  if ( rv != 0 ) {
    fprintf( stderr, "pip_init/1: %s", strerror( rv ) );
    return EXIT_FAIL;
  }
  
  ntasks = NTASKS;
  exp = NULL;
  rv = pip_init( &pipid, &ntasks, &exp, 0 );
  if ( rv != EBUSY ) {
    fprintf( stderr, "pip_init/2: %s", strerror( rv ) );
    return EXIT_FAIL;
  }

  return EXIT_PASS;
}

static int test_ntask_is_zero( char **argv, char *base )
{
  int pipid, ntasks, rv;
  void *exp;

  ntasks = 0;
  exp = NULL;
  rv = pip_init( &pipid, &ntasks, &exp, 0 );
  if ( rv != EINVAL ) {
    fprintf( stderr, "pip_init: %s", strerror( rv ) );
    return EXIT_FAIL;
  }
  
  return EXIT_PASS;
}

static int test_ntask_too_big( char **argv, char *base )
{
  int pipid, ntasks, rv;
  void *exp;

  ntasks = PIP_NTASKS_MAX + 1;
  exp = NULL;
  rv = pip_init( &pipid, &ntasks, &exp, 0 );
  if ( rv != EOVERFLOW ) {
    fprintf( stderr, "pip_init: %s", strerror( rv ) );
    return EXIT_FAIL;
  }
  
  return EXIT_PASS;
}

static int test_invalid_opts( char **argv, char *base )
{
  int pipid, ntasks, rv;
  void *exp;

  ntasks = 0;
  exp = NULL;
  rv = pip_init( &pipid, &ntasks, &exp, ~PIP_VALID_OPTS );
  if ( rv != EINVAL ) {
    fprintf( stderr, "pip_init: %s", strerror( rv ) );
    return EXIT_FAIL;
  }
  
  return EXIT_PASS;
}

static int test_both_pthread_process( char **argv, char *base )
{
  int pipid, ntasks, rv;
  void *exp;

  ntasks = 0;
  exp = NULL;
  rv = pip_init( &pipid, &ntasks, &exp, PIP_MODE_PTHREAD | PIP_MODE_PROCESS );
  if ( rv != EINVAL ) {
    fprintf( stderr, "pip_init: %s", strerror( rv ) );
    return EXIT_FAIL;
  }
  
  return EXIT_PASS;
}

static int test_both_preload_clone( char **argv, char *base )
{
  int pipid, ntasks, rv;
  void *exp;

  ntasks = 0;
  exp = NULL;
  rv = pip_init( &pipid, &ntasks, &exp,
		 PIP_MODE_PROCESS_PRELOAD | PIP_MODE_PROCESS_PIPCLONE );
  if ( rv != EINVAL ) {
    fprintf( stderr, "pip_init: %s", strerror( rv ) );
    return EXIT_FAIL;
  }
  
  return EXIT_PASS;
}

static int test_pip_task_unset( char **argv, char *base )
{
  int pipid, ntasks, rv;
  void *exp;

#ifndef DO_NOT_TEST
  char *env_pip_root;

  unsetenv( "PIP_TASK" );
#endif

  ntasks = 1;
  exp = NULL;
  rv = pip_init( &pipid, &ntasks, &exp, 0);

#ifndef DO_NOT_TEST
  env_pip_root = getenv( "PIP_ROOT" );
  if( env_pip_root != NULL ) {
    if( rv == EPERM )
      return EXIT_PASS;

    fprintf( stderr, "PIP_ROOT=\"%s\", but pip_init: %s",
	     env_pip_root, strerror( rv ) );
    return EXIT_FAIL;
  }

  if ( rv != 0 ) {
    fprintf( stderr, "pip_init: %s", strerror( rv ) );
    return EXIT_FAIL;
  }
#endif
  
  if( pipid == PIP_PIPID_ROOT ) {
    pipid = 0;
    rv = pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
		    NULL, NULL, NULL );
    if( rv != 0 ) {
      fprintf( stderr, "pip_spawn: %s", strerror( rv ) );
      return EXIT_FAIL;
    }
    rv = pip_wait( 0, NULL );
    if( rv != 0 ) {
      fprintf( stderr, "pip_wait: %s", strerror( rv ) );
      return EXIT_FAIL;
    }
    rv = pip_fin();
    if( rv != 0 ) {
      fprintf( stderr, "pip_fin: %s", strerror( rv ) );
      return EXIT_FAIL;
    }
    printf( "Hello, pip root task is fine !!\n" );
  } else {
    printf( "<%d> Hello, I am fine !!\n", pipid );
  }

  return EXIT_PASS;
}


int main( int argc, char **argv ) {
  char *base = basename( argv[0] );
  int i;

  static struct {
    char *name;
    int (*func)( char **argv, char *base );
  } tab[] = {
    { "pip_init", test_pip_init },
    { "pip_init.preload", test_pip_init_preload },
    { "pip_init.twice", test_twice },
    { "pip_init.ntask_is_zero", test_ntask_is_zero },
    { "pip_init.ntask_too_big", test_ntask_too_big },
    { "pip_init.invalid_opts", test_invalid_opts },
    { "pip_init.both_pthread_process", test_both_pthread_process },
    { "pip_init.both_preload_clone", test_both_preload_clone },
    { "pip_init.pip_task_unset", test_pip_task_unset },
  };

  for( i = 0; i < sizeof( tab ) / sizeof( tab[0] ); i++ ) {
    if( strcmp( tab[i].name, base ) == 0 )
      exit( tab[i].func( argv, base ) );
  }

  fprintf( stderr, "%s: unknown test type\n", base);
  return EXIT_UNTESTED;
}
