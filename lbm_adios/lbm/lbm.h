#ifndef RUN_LBM_H
#define RUN_LBM_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <pthread.h>
#include <math.h>
#include <mpi.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

#define SIZE_ONE (2) // two doubles in each line

#ifndef S_OK
typedef int status_t;
#define S_OK (0)
#define S_FAIL (-1)
#endif

/*
 * init lbm, allocate space fordf1, df2, df_inout
 * 
 * @param bounds the dimenson each process working on(16x16x64)
 * @param pcomm communicator
 * @param size_one number of doubles in each line
 * @param pbuff  the buffer i need to output 
 */
status_t lbm_init(int bounds[3], MPI_Comm *pcomm, size_t size_one,  double **buffer);
/*
 * advance lbm
 * 
 * @param 
 */
status_t lbm_advance_step(MPI_Comm * pcomm, double *buffer);

/*
* example of  io routine
*/
status_t lbm_io_template(MPI_Comm *pcomm, double *buffer){
    return S_OK;
}


/*
 * finalize lbm
 */
status_t lbm_finalize(MPI_Comm *pcomm, double *buffer);



// this must be included
/*
 * this is moved to main function arguments
#define TOTAL_FILE2PRODUCE_1GB 256 
#define nx TOTAL_FILE2PRODUCE_1GB/4
#define ny TOTAL_FILE2PRODUCE_1GB/4
#define nz TOTAL_FILE2PRODUCE_1GB
*/

// run lbm 
// input:
//      step_stop 
//      dims_cube(number of element in each dimension of a cube)
// data layout
//      there will be  X*Y*Z cubes each has cubex*cubey*cube z elememts,
//      each cell will have two double values
//      data is send once for each cube
//void run_lbm(char *filepath, int step_stop, int dims_cube[3], MPI_Comm * pcomm);

//double get_cur_time();

//void check_malloc(void * pointer);
#endif
