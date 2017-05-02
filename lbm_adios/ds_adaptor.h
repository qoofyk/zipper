
#ifdef __cplusplus
extern "C" {
#endif

#include "dataspaces.h"
#include "dimes_interface.h"
#include "utility.h"
//#include "region_def.h"
#include "string.h"
#include "mpi.h"
#include "stdlib.h"
#include <stdio.h>

//#include <mpi.h>

/*
 * read raw velocity and pres buffer from dataspaces
 * INPUT:
 *  timestep:
 *      int
 *  extra_info
 *      dimension info
 *  rank
 *  p_gcomm, var_name_vel,_var_name_pres
 *      required by dsput
 *
 * OUTPUT
 *  p_buffer
 *      receiving buffer
 */

// this will get all vel and pres data
void get_common_buffer(int timestep,int ndim, int bounds[6], int rank, MPI_Comm * p_gcomm,char * var_name, void **p_buffer,size_t elem_size, double *p_time_used);
 
void put_common_buffer(int timestep, int ndim, int bounds[6], int rank, MPI_Comm * p_gcomm,char * var_name, void **p_buffer,size_t elem_size, double *p_time_used);

//void get_common_buffer_unblocking(int timestep,int ndim, int bounds[6], int rank, MPI_Comm * p_gcomm,char * var_name, void **p_buffer,size_t elem_size, double *p_time_used);

#ifdef __cplusplus
}
#endif
