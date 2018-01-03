#ifndef LBM_BUFFER_H
#define LBM_BUFFER_H
/*
 * helper functions when you want each process operates on a large chunk buffer
 *
 * Feng Li 
 * Jan 2018
 */

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
