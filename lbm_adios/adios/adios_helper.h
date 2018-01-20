/*
 * @author Feng Li, IUPUI
 * @date   2017
 */

#ifndef ADIOS_HELPER_H
#define ADIOS_HELPER_H
#ifdef __cplusplus
extern "C"{
#endif

#include <stdio.h>
#include <string.h>
#include <mpi.h>
#include "adios.h"
#include "time.h"
#include <unistd.h>
#include "transports.h"
//#include "utility.h"


/*
 * previously it's in lammps source tree
 */
//void insert_into_Adios(int n, double * buf, int step, int nsteps);
void insert_into_Adios(transport_method_t method, char *var_name, int step, int nsteps, int n, int size_one, double * buf, const char* mode,  MPI_Comm *pcomm);

/* 
 * @brief adios writting wrapper
 *
 * @param file_path file_path to write data to(not applicable to )
 * @param var_name  varable names for stating 
 * @param timestep  streaming write(-1) or current timesteps(mpiio write data into different files)
 * @param n         number of lines(data records)
 * @param size_one  number of elements in each line(record)
 * @param buffer    data buffer
 * @param mode      write mode, restart file is written is append mode
 * @param pcomm     communicator
 */
void insert_into_adios(char * file_path, char *var_name, int timestep, int n, int size_one, double * buf, const char * mode, MPI_Comm *pcomm);

#ifdef __cplusplus
}
#endif
#endif
