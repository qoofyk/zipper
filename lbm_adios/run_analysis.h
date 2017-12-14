#ifndef RUN_ANALYSIS_H
#define RUN_ANALYSIS_H
#ifdef __cplusplus
extern "C"{
#endif

#include <stdio.h>
#include <string.h>
#include <mpi.h>
#include "adios.h"
#include "time.h"
#include <math.h>
#include <unistd.h>
#define USE_ADIOS

#define SIZE_ONE (2)

// maximum moments?
#define NMOMENT 8



/*
 * calculate moment of given block
 * input 
 *      block_size, number of positions(each position will have two double values)
 *      nl: number of moments
 */
void run_analysis(double* buf_blk, int block_size ,int lp,double *sum_vx, double *sum_vy);


#ifdef __cplusplus
}
#endif
#endif
