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


#ifndef filesize2produce
#define filesize2produce (256)
#endif

#define nx (filesize2produce/4)
#define ny (filesize2produce/4)
#define nz (filesize2produce)


//u_r, v, u using extern

/*
 * init lbm, allocate space fordf1, df2, df_inout
 * 
 * //@param bounds the dimenson each process working on(16x16x64), now is preset
 * @param pcomm pointer to communicator
 * @step_stop how many steps
 */
status_t lbm_init(MPI_Comm *pcomm, int step_stop);


/*
 * advance lbm
 * 
 * @param  pcomm pointer to communicator
 */
status_t lbm_advance_step(MPI_Comm * pcomm);

/*
 * finalize lbm
 *
 * @param pcomm pointer to communicator
 */
status_t lbm_finalize(MPI_Comm *pcomm);


#endif


