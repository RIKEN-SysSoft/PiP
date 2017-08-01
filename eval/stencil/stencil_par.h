/*
 * stencil_par.h
 *
 *  Created on: Jan 4, 2012
 *      Author: htor
 */

#ifndef STENCIL_PAR_H_
#define STENCIL_PAR_H_

#ifdef AH
#ifndef PIP
#include "mpi.h"
#else
#include <pip.h>
#endif
#endif

#include <mpi.h>
#include <pip.h>

#include "../eval.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#endif /* STENCIL_PAR_H_ */
