/*
  * $RIKEN_copyright: 2018 Riken Center for Computational Sceience, 
  * 	  System Software Devlopment Team. All rights researved$
  * $PIP_VERSION: Version 1.0$
  * $PIP_license: Redistribution and use in source and binary forms,
  * with or without modification, are permitted provided that the following
  * conditions are met:
  * 
  *     Redistributions of source code must retain the above copyright
  *     notice, this list of conditions and the following disclaimer.
  * 
  *     Redistributions in binary form must reproduce the above copyright
  *     notice, this list of conditions and the following disclaimer in
  *     the documentation and/or other materials provided with the
  *     distribution.
  * 
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  * 
  * The views and conclusions contained in the software and documentation
  * are those of the authors and should not be interpreted as representing
  * official policies, either expressed or implied, of the FreeBSD Project.$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2016
*/

#ifndef PVAS_H
#define PVAS_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

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
