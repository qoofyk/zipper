/*
 * lbm simulation code
 *
 * author luoding zhu
 * rewrite by Yuankun Fu, Jan 2018
 */
#ifndef RUN_MODULE_LBM_H
#define RUN_MODULE_LBM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lbm.h"
#include "concurrent.h"



status_t insert_zipper(GV gv);

status_t run_module_lbm(GV gv, MPI_Comm *pcomm);

#ifdef __cplusplus
}
#endif


#endif


