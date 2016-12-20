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
	int c, n = 0;

	while ((c = getopt(argc, argv, "p")) != -1) {
		switch (c) {
		case 'p':
			option_pip_mode = 1;
			break;
		default:
			fprintf(stderr, "Usage: dlmopen_count [-p]\n");
			exit(2);
		}
	}

	for (;;) {
		so = dlmopen(LM_ID_NEWLM, argv[0], RTLD_NOW | RTLD_LOCAL);
		if (so == NULL)
			break;
		if (++n >= MAX)
			break;
	}
	if (option_pip_mode && n > PIP_NTASKS_MAX)
		n = PIP_NTASKS_MAX;
	printf("%d\n", n);
	return 0;	
}
