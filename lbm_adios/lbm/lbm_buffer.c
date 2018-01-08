#include "lbm.h"
#include "lbm_buffer.h"

extern double only_lbm_time, init_lbm_time; // timer can also be placed outside
extern double u_r;
extern double u[nx][ny][nz],v[nx][ny][nz];

static int    rank, nprocs; // will initialize in alloc_buffer


status_t lbm_alloc_buffer(MPI_Comm *pcomm, size_t nlocal, size_t size_one, double **pbuff){
        /* alloc io buffer */
        MPI_Comm comm = *pcomm;
        MPI_Comm_rank (comm, &rank);
        MPI_Comm_size (comm, &nprocs);

		*pbuff = (double *)malloc(nlocal*sizeof(double)*size_one);
		if(NULL == *pbuff) return S_FAIL;
        if(rank == 0){
            printf("[LBM INFO]: io buffer allocated\n");
        }
        return S_OK;
}

status_t lbm_get_buffer(double *buffer){		
    int gi, gj, gk;
    int count=0;

    for(gi = 0; gi < nx; gi++){
        for(gj = 0; gj < ny; gj++){
            for(gk = 0; gk < nz; gk++){
                *((double *)(buffer+count))=u_r*u[gi][gj][gk];
                *((double *)(buffer+count+1))=u_r*v[gi][gj][gk];
#ifdef debug_1
                printf("(%d %d %d), u_r=_%lf u=%lf v= %lf\n", gi, gj, gk, u_r, u[gi][gj][gk],v[gi][gj][gk]);
#endif
                count+=2;
            }
        }
    }

    return S_OK;
}

status_t lbm_free_buffer(MPI_Comm *pcomm, double *buffer){
    MPI_Comm comm = *pcomm;
	double global_t_cal=0;
	MPI_Reduce(&only_lbm_time, &global_t_cal, 1, MPI_DOUBLE, MPI_MAX, 0, comm);

	if(rank == 0){
		printf("t_prepare:%f s, max t_cal %f s\n", init_lbm_time, global_t_cal);
	}

	// MPI_Barrier(comm1d);

	if(buffer){
		free(buffer);
		//printf("io buffer freed\n");
	}
    return S_OK;
}

status_t lbm_io_template(MPI_Comm *pcomm, double *buffer, size_t nlocal, size_t size_one){
    return S_OK;
}


