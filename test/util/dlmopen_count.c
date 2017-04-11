#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <pip.h>

#define MAX 1024

int
main(int argc, char **argv)
{
	void *so;
	int option_pip_mode = 1;
	int option_verbose  = 0;
	int c, n = 0;

	while ((c = getopt(argc, argv, "pv")) != -1) {
		switch (c) {
		case 'p':
			option_pip_mode = 1;
			break;
		case 'v':
			option_verbose = 1;
			break;
		default:
			fprintf(stderr, "Usage: dlmopen_count [-p] [-v]\n");
			exit(2);
		}
	}

	for (;;) {
		so = dlmopen(LM_ID_NEWLM, argv[0], RTLD_NOW | RTLD_LOCAL);
		if (so == NULL) {
		  if( option_verbose ) {
		    fprintf( stderr, "dlmopen(): %s\n", dlerror() );
		  }
		  break;
		}
		if (++n >= MAX)
			break;
	}
	if( option_verbose ) {
	  fprintf( stderr, "DL_NNS=%d  PIP_NATSKS_MAX=%d\n",
		   n, PIP_NTASKS_MAX );
	}
	if (option_pip_mode && n > PIP_NTASKS_MAX)
		n = PIP_NTASKS_MAX;
	printf("%d\n", n);

	return 0;
}
