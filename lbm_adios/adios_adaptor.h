/*
 * @author Feng Li, IUPUI
 * @date   2017
 */
#ifndef ADIOS_ADAPTOR_H
#define ADIOS_ADAPTOR_H
#ifdef __cplusplus
extern "C"{
#endif

#include <stdio.h>
#include <string.h>
#include <mpi.h>
#include "adios.h"
#include "time.h"
#include <unistd.h>
#include "utility.h"


/* 
 * writing data using adios
 * input:
 *      file_path,
 *          file_path to write data to(not applicable to )
 *      var_name,
 *          varable names for stating 
 *      timestep
 *          -1 for streaming write
 *          otherwise, is the current timesteps(mpiio write data into different files)
 *      n,
 *          number of lines
 *      size_one
 *          number of elements in each line
 *      buffer
 *          data buffer
 *      mode
 *          write mode, restart file is written is append mode
 *      pcomm
 *          communicator
 */
void insert_into_adios(char * file_path, char *var_name, int timestep, int n, int size_one, double * buf, const char * mode, MPI_Comm *pcomm);

#ifdef __cplusplus
}
#endif
#endif
