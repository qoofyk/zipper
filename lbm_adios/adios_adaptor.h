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


void insert_into_adios(char * file_path, char *var_name, int n, int size_one, double * buf, MPI_Comm *pcomm);

#ifdef __cplusplus
}
#endif
#endif
