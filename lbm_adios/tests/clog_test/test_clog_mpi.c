/*
 * @brief  test clog 
 *
 * @author Feng Li, lifen@iupui.edu, IUPUI
 * @date   2017
 */

#define CLOG_MAIN
#include "clog_test_utility.h"
#include <stdio.h>
#include <mpi.h>


int main(int argc, char *argv[]){
    MPI_Init(&argc, &argv);

    MPI_Comm comm = MPI_COMM_WORLD;
    int         rank, nprocs;
    MPI_Comm_rank (comm, &rank);
    MPI_Comm_size (comm, &nprocs);


    /*
     * init log
     */
    int r;

    char log_path[256];
    sprintf(log_path,"%d.clog", rank);
    printf("rank %d: i will write log to %s\n", rank,log_path);
    r = clog_init_path(MY_LOGGER, log_path);
    if (r != 0) {
      fprintf(stderr, "Logger initialization failed.\n");
      return 1;
    }
    clog_info(CLOG(MY_LOGGER), "Hello, world!");

    /*
    * try a log from another .c file
    */
    try_clog_error();
    clog_free(MY_LOGGER);
    return 0;
}


