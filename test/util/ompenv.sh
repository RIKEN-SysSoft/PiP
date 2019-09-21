#!/bin/sh
nth=$1;
shift;
OMP_NUM_THREADS=$nth $@;
