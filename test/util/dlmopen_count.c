#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>

#define MAX 1000

int
main(int argc, char **argv)
{
	void *so;
	int n = 0;

	for (;;) {
		so = dlmopen(LM_ID_NEWLM, argv[0], RTLD_NOW | RTLD_LOCAL);
		if (so == NULL)
			break;
		if (++n >= MAX)
			break;
	}
	printf("%d\n", n);
	return 0;	
}
