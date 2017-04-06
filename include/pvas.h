/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2016
*/

#ifndef PVAS_H
#define PVAS_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <sys/types.h>
#include <pip.h>

/* not being used, just for compatibility */
typedef struct pvas_common_info {
  int fd;             // file descriptor of the file to be mapped
  off_t offset;       // offset in the file
  size_t size;        // size of the mapping
  unsigned long addr; // should be NULL
} pvas_common_info_t;

#ifdef __cplusplus
extern "C" {
#endif

  int pvas_create(pvas_common_info_t info[], int n, int *pvd);
  int pvas_destroy(int pvd);
  int pvas_spawn(int pvd, int *pvid, char *filename,
		 char **argv, char **envp, intptr_t *pid);
  int pvas_spawn_setaffinity(int pvd, int *pvid, char *filename,
			     char **argv, char **envp, intptr_t *pid, int cpu);
  int pvas_get_pvid(int *pvid);
  int pvas_ealloc(size_t esize);
  int pvas_get_export_info(int pvid, void **addr, size_t *size);
  int pvas_get_ntask(int *n);
  int pvas_get_ncommon(int *nc);
  int pvas_get_common_info(int n, pvas_common_info_t *info);

#ifdef __cplusplus
}
#endif

#endif

#endif /* PVAS_H */
