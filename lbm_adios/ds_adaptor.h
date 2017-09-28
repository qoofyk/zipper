/*
 * @brief simple dspaces/dimes wrapper 
 *
 * @author Feng Li, IUPUI
 * @date   2017
 */
#ifndef DS_ADAPTOR_H
#define DS_ADAPTOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dataspaces.h"
#include "dimes_interface.h"
//#include "region_def.h"
#include "string.h"
#include "mpi.h"
#include "stdlib.h"
#include <stdio.h>
//#define DS_MAX_VERSION (20)

#include "transports.h"

//#include <mpi.h>

/*
 * @brief get data from dspaces/dimes
 *
 * @param transport_minor   tranport methid, 0 for dspaces, 1 for dimes
 * @param timestep  current timstep
 * @param ndim      number of dimensions
 * @param bounds    boundry in each dimension
 * @param rank      rank of current proc
 * @param var_name  variable name 
 * @param p_buffer  data buffer
 * @param elem_size element size
 * @param p_time_used timer
 */
void get_common_buffer(uint8_t transport_minor, int timestep,int ndim, int bounds[6], int rank, char * var_name, void **p_buffer,size_t elem_size, double *p_time_used);
 
/*
 * @brief put data to dspaces/dimes
 *
 * @param transport_minor   tranport methid, 0 for dspaces, 1 for dimes
 * @param timestep  current timstep
 * @param ndim      number of dimensions
 * @param bounds    boundry in each dimension
 * @param rank      rank of current proc
 * @param var_name  variable name 
 * @param p_buffer  data buffer
 * @param elem_size element size
 * @param p_time_used timer
 */

void put_common_buffer(uint8_t transport_minor, int timestep, int ndim, int bounds[6], int rank, char * var_name, void **p_buffer,size_t elem_size, double *p_time_used);

//void get_common_buffer_unblocking(int timestep,int ndim, int bounds[6], int rank, MPI_Comm * p_gcomm,char * var_name, void **p_buffer,size_t elem_size, double *p_time_used);

#ifdef __cplusplus
}
#endif

#endif
