/*
 * stencil_par.h
 *
 *  Created on: Jan 4, 2012
 *      Author: htor
 */

#ifndef STENCIL_PAR_H_
#define STENCIL_PAR_H_

#ifndef PIP
#include "mpi.h"
#else
#include <pip.h>
#endif

#include "../eval.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// row-major order
#define ind(i,j) (j)*(bx+2)+(i)

#endif /* STENCIL_PAR_H_ */
