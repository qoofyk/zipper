/*
 * lammps simulation code
 *
 * rewrite by Yuankun, Jan 2018
 */
#ifndef RUN_MODULE_LAMMPS_H
#define RUN_MODULE_LAMMPS_H


#ifdef __cplusplus
extern "C" {
#endif

#include "concurrent.h"


//status_t insert_zipper(void* gv, double **x, int nlocal, int step);
//status_t run_module_lammps(int argc, char **argv, GV gv, MPI_Comm *pcomm);
status_t run_module_lammps (int argc, char *argv[], GV gv, MPI_Comm *pcomm);

#ifdef __cplusplus
}
#endif


#endif


