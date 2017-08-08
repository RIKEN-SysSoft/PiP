/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2017
*/

#include <stdio.h>
#include <test.h>

class PIP {
public:
  static int xxx;
  int var;
  int pipid;

  PIP(int id) { pipid = id; };
  int foo( void ) { return var; };
  void bar( int x ) { var = x; };
};

#define MAGIC		(13579)

int PIP::xxx = MAGIC;

PIP *objs[NTASKS];

int main( int argc, char **argv ) {
  void 	*exp;
  int pipid;
  int ntasks;
  int i;
  int err;

  if( argc > 1 ) {
    ntasks = atoi( argv[1] );
    ntasks = ( ntasks > NTASKS ) ? NTASKS : ntasks;
  } else {
    ntasks = NTASKS;
  }
  for( i=0; i<ntasks; i++ ) {
    objs[i] = new PIP( i );
    objs[i]->bar( i * 100 );
  }

  exp = (void*) &objs[0];
  TESTINT( pip_init( &pipid, &ntasks, &exp, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    for( i=0; i<ntasks; i++ ) {
      pipid = i;
      err = pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
		       NULL, NULL, NULL );
      if( err != 0 ) break;
      if( i != pipid ) {
	fprintf( stderr, "pip_spawn(%d!=%d) !!!!!!\n", i, pipid );
      }
    }
    ntasks = i;
    for( i=0; i<ntasks; i++ ) TESTINT( pip_wait( i, NULL ) );
  } else {
    intptr_t id;
    int ng = 0;

    if( PIP::xxx != MAGIC ) {
      printf( "[%d] XXX=%d@%p\n", pipid, PIP::xxx, &PIP::xxx );
      ng = 1;
    }
    for( i=0; i<ntasks; i++ ) {
      if( objs[i]->foo() != i * 100 ) {
	printf( "[%d] foo(%d)=%d\n", pipid, i, objs[i]->foo() );
	ng = 1;
      }
      delete objs[i];
      objs[i] = NULL;
    }
    if( ( err = pip_get_id( PIP_PIPID_MYSELF, &id ) ) != 0 ) {
      printf( "pip_get_id()=%d\n", err );
    }
    if( !ng ) printf( "[%d:%d:%lu] I am fine\n", pipid, getpid(), (unsigned long) id );
  }
  TESTINT( pip_fin() );
}
