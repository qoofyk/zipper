/*
 * @author Feng Li, IUPUI
 * @date   2017
 */
#include <fcntl.h>                                
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h> 
#include "adios_helper.h"
#include "stdio.h"
#include "stdlib.h"

// each proc will get different number of lines in lammps
//#define PRECISE


// for lammps, each line has different lines
void insert_into_Adios(transport_method_t transport, char *var_name, int step, int nsteps, int n, int size_one, double * buf, const char* mode,  MPI_Comm *pcomm){
//void insert_into_Adios(int n, double * buf, int step, int nsteps){
	char        filename [256];
	int         rank, size, i, j;
    int ntotal;
	int         NX; // this should be a

    double t0, t1;
    
    //int size_one = SIZE_ONE;
    // prepare sending buffer
    double * t = (double *)malloc(n*size_one*sizeof(double));
	//double      t[NX*SIZE_ONE];

	/* ADIOS variables declarations for matching gwrite_temperature.ch */
	uint64_t    adios_groupsize, adios_totalsize;
	int64_t     adios_handle;

	MPI_Comm    comm = *pcomm;
	MPI_Comm_rank (comm, &rank);
	MPI_Comm_size (comm, &size);

    //transport = get_current_transport();
    uint8_t transport_major = get_major(transport);
    uint8_t transport_minor = get_minor(transport);

    // save nlines in all processes
    int lb;
#ifdef PRECISE
    //each process generate different number of lines
    int nlines_all[size];
    MPI_Allgather(&n, 1, MPI_INT, nlines_all,1, MPI_INT, comm);
    MPI_Allreduce(&n, &ntotal, 1, MPI_INT, MPI_SUM, comm);
    
    lb = 0;
    for(i = 0; i< rank; i++){
        lb+=nlines_all[i];
    }
#else
    printf("[warning]: use estimate lines \n");
    ntotal=n*size;
    lb = rank*size;
#endif
    NX=ntotal;


    if(transport_major == ADIOS_DISK){
        // disk need to write to different file
	    sprintf(filename, "atom_%d.bp", step);
    }
    else if(transport_major == ADIOS_STAGING){
	    sprintf(filename, "atom.bp");
    }

    if(rank == 0){
        printf("\n\n********************************\n");
        printf("rank %d:step %d start to write\n", rank, step);
    }
    // a copy here, in case n!= nmax
    for (i = 0; i < n; i++){
        /*for(j = 0; j<SIZE_ONE; j++){*/
            ////t[i*SIZE_ONE + j] = 100*timestep + rank*NX + i + 0.1*j;
            //t[i*SIZE_ONE + j] = buf[i*SIZE_ONE +j];
        /*}*/
        t[i*size_one] = buf[i*size_one];
        t[i*size_one+1] = buf[i*size_one+1];
        t[i*size_one+2] = buf[i*size_one+2];
        t[i*size_one+3] = buf[i*size_one+3];
        t[i*size_one+4] = buf[i*size_one+4];

    }
    t0 = MPI_Wtime();

    adios_open (&adios_handle, "atom", filename, "w", comm);
    //printf("lb = %d, n = %d, NX = %d, size = %d, rank = %d \n",lb, n, NX, size, rank);
    //#include "gwrite_temperature.ch"
    //#include "gwrite_atom.ch"
    
    adios_groupsize = 4 \
                    + 4 \          
                    + 4 \          
                    + 4 \          
                    + 8 * (n) * (size_one);         
    adios_group_size (adios_handle, adios_groupsize, &adios_totalsize);
    adios_write (adios_handle, "NX", &NX);
    adios_write (adios_handle, "lb", &lb);
    adios_write (adios_handle, "n", &n);
    adios_write (adios_handle, "size_one", &size_one);
    adios_write (adios_handle, "atom", t);

    // end of gwrite_atom

    adios_close (adios_handle);

    /**** use index file to keep track of current step *****/
    if(transport_major == ADIOS_DISK){
        int fd; 
        char step_index_file[256];
        int time_stamp;
        
        if(rank == 0){
            if(step == nsteps - 1){
               time_stamp = -2;
            }else{
               time_stamp = step;
            }// flag read from producer

            sprintf(step_index_file, "stamp.file");

            printf("step index file in %s \n", step_index_file);

            fd = open(step_index_file, O_WRONLY|O_CREAT|O_SYNC, S_IRWXU);
            if(fd < 0){
                perror("indexfile not opened");
                exit(-1);
            }
            else{
                flock(fd, LOCK_EX);
                write(fd,  &time_stamp,  sizeof(int));
                flock(fd, LOCK_UN);
                printf("write stamp %d at %lf", time_stamp, MPI_Wtime());
                close(fd);
            }
        }
        // wait until all process finish writes and 
        MPI_Barrier(comm);
    }
    t1 = MPI_Wtime();
    if(rank == 0){
        printf("rank %d: write completed in %.3lfs, current time %.3lf\n", rank, t1-t0, t1);
    }
    if(t){
        free(t);
    }

}


void insert_into_adios(char * file_path, char *var_name,int timestep, int n, int size_one, double * buf, const char* mode,  MPI_Comm *pcomm){
    char        filename [256];
    int         rank, size;
    int         NX;
    
    //int size_one = SIZE_ONE;
    // prepare sending buffer
    //double * t =  buf;
    //double      t[NX*SIZE_ONE];

    /* ADIOS variables declarations for matching gwrite_temperature.ch */
    //uint64_t    adios_groupsize, adios_totalsize;
    int64_t     adios_handle;

    MPI_Comm    comm = *pcomm;
    MPI_Comm_rank (comm, &rank);
    MPI_Comm_size (comm, &size);

    // nlines of all processes
    NX = size*n;
    
    // lower bound of my line index
    int lb;
    lb = rank*n;

    if(timestep <0){
        sprintf(filename, "%s/%s.bp", file_path, var_name);
    }
    else{
        sprintf(filename, "%s/%s_%d.bp", file_path, var_name, timestep);
    }

    //printf("rank %d: start to write\n", rank);
    
    adios_open (&adios_handle, var_name, filename, mode, comm);
#ifdef debug
    printf("rank %d: file %s opened\n", rank, filename);
#endif
    /* generated by gpp.py*/
    //#include "gwrite_atom.ch"
    /*
    adios_groupsize = 4 \
                    + 4 \
                    + 4 \
                    + 4 \
                    + 8 * (n) * (size_one);
    adios_group_size (adios_handle, adios_groupsize, &adios_totalsize);
    */
    adios_write (adios_handle, "NX", &NX);
    adios_write (adios_handle, "lb", &lb);
    adios_write (adios_handle, "n", &n);
    adios_write (adios_handle, "size_one", &size_one);
    adios_write (adios_handle, var_name, buf);
    adios_close (adios_handle);

    /*
    if(rank ==0){
        printf("groupsize = %ld, adios totalsize = %ld\n",adios_groupsize, adios_totalsize);
    }
    */
}

