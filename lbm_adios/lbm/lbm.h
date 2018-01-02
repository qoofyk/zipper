#ifndef LBM_H
#define LBM_H

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


//u_r, v, u using extern

/*
 * init lbm, allocate space fordf1, df2, df_inout
 * 
 * //@param bounds the dimenson each process working on(16x16x64), now is preset
 * @param pcomm communicator
 * @param size_one number of doubles in each line
 * @param pbuff  the buffer i need to output 
 */
status_t lbm_init(MPI_Comm *pcomm);


/*
 * advance lbm
 * 
 * @param 
 */
status_t lbm_advance_step(MPI_Comm * pcomm);

/*
 * finalize lbm
 */
status_t lbm_finalize(MPI_Comm *pcomm);




// 
/*
* example of  io routine
*/
status_t lbm_io_template(MPI_Comm *pcomm, double *buffer, size_t nlocal, size_t size_one){
    return S_OK;
}

status_t lbm_alloc_buffer(size_t nlines, size_t size_one, double **pbuffer);
status_t lbm_get_buffer(double *buffer);
status_t lbm_free_buffer(MPI_Comm *pcomm, double *buffer);

#endif
