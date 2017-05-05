#include "ds_adaptor.h"
//#define USE_SAME_LOCK

#define debug_1
void get_common_buffer(int timestep,int ndim, int bounds[6], int rank, MPI_Comm * p_gcomm,char * var_name, void **p_buffer,size_t elem_size, double *p_time_used){

    printf("\n ** prepare to get, ndim = %d\n", ndim);
    // how many number of elements are actually written
    //int num_elems;
    char msg[STRING_LENGTH];
    double t1, t2;
    int ret_get = -1;

    int num_points;
    
    uint64_t lb[3] = {0}, ub[3] = {0};
    /*
    lb[0] = 0;
    ub[0] = num_points - 1;
    */
    lb[0] = bounds[0];
    lb[1] = bounds[1];
    lb[2] = bounds[2];
    //y
    ub[0] = bounds[3];
    //x
    ub[1] = bounds[4];
    ub[2] = bounds[5];

    num_points = (bounds[3]-bounds[0]+1)*(bounds[4]- bounds[1]+1)*(bounds[5]- bounds[2]+1);

    // Define the dimensionality of the data to be received 
    //int ndim = 3;

    char lock_name[STRING_LENGTH];
#ifdef USE_SAME_LOCK
    snprintf(lock_name, STRING_LENGTH, "%s_lock", var_name);
#else
    snprintf(lock_name, STRING_LENGTH, "%s_lock_t_%d",var_name, timestep%20);
#endif

#ifdef debug_1
    printf("lb: (%d, %d  %d), hb(%d, %d, %d), elem_size %zu bytes\n", bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5], elem_size);
#endif

    sprintf(msg, "try to acquired the read lock %s", lock_name);
    my_message(msg, rank, LOG_WARNING);

    dspaces_lock_on_read(lock_name, p_gcomm);

    sprintf(msg, "get the read lock");
    my_message(msg, rank, LOG_WARNING);

    // read all regions in once
    t1 = MPI_Wtime();
#ifdef RAW_DSPACES
    ret_get = dspaces_get(var_name, timestep, elem_size, ndim, lb, ub, *p_buffer);
#elif defined(RAW_DIMES)
    ret_get = dimes_get(var_name, timestep, elem_size, ndim, lb, ub, *p_buffer);
//#else
//#error("either dspaces or dimes")
#endif
    t2 = MPI_Wtime();

    sprintf(msg, "try to unlock the read lock");
    my_message(msg, rank, LOG_WARNING);


    // now we can release region lock
    dspaces_unlock_on_read(lock_name, p_gcomm);
    sprintf(msg, "release the read lock");
    my_message(msg, rank, LOG_WARNING);

    if(ret_get != 0){
        printf("get varaible %s err,  error number %d \n", var_name, ret_get);
        exit(-1);
    }else{
        sprintf(msg, "read %d elem from dspaces, each has %zu bytes", num_points, elem_size);
        my_message(msg, rank, LOG_WARNING);
    }

    *p_time_used = t2-t1;
    
}

void put_common_buffer(int timestep,int ndim, int bounds[6], int rank, MPI_Comm * p_gcomm,char * var_name, void  **p_buffer,size_t elem_size, double *p_time_used){

    printf("\n ** prepare to put, ndim = %d\n", ndim);
    // how many number of elements are actually written
    //int num_elems;
    char msg[STRING_LENGTH];
    double t1, t2;
    int ret_put = -1;

    int num_points;
    
    uint64_t lb[3] = {0}, ub[3] = {0};
    /*
    lb[0] = 0;
    ub[0] = num_points - 1;
    */
    lb[0] = bounds[0];
    lb[1] = bounds[1];
    lb[2] = bounds[2];
    //y
    ub[0] = bounds[3];
    //x
    ub[1] = bounds[4];
    ub[2] = bounds[5];

    num_points = (bounds[3]-bounds[0]+1)*(bounds[4]- bounds[1]+1)*(bounds[5]- bounds[2]+1);

    char lock_name[STRING_LENGTH];
#ifdef USE_SAME_LOCK
    snprintf(lock_name, STRING_LENGTH, "%s_lock", var_name);
#else
    snprintf(lock_name, STRING_LENGTH, "%s_lock_t_%d",var_name, timestep%20);
#endif

#ifdef debug_1
    printf("lb: (%d, %d  %d), hb(%d, %d, %d), elem_size %zu bytes\n", bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5], elem_size);
#endif

    sprintf(msg, "try to acquired the write lock %s", lock_name);
    my_message(msg, rank, LOG_WARNING);

    dspaces_lock_on_write(lock_name, p_gcomm);

    sprintf(msg, "get the write lock");
    my_message(msg, rank, LOG_WARNING);

    // write all data in once
    t1 = MPI_Wtime();
#ifdef RAW_DSPACES
    ret_put = dspaces_put(var_name, timestep, elem_size, ndim, lb, ub, *p_buffer);
#elif defined(RAW_DIMES)
    ret_put = dimes_put(var_name, timestep, elem_size, ndim, lb, ub, *p_buffer);
//#else
//#error("either dimes or dataspaces should be defined")
#endif
    //int sync_ok = dspaces_put_sync();
    int sync_ok = 0;
    t2 = MPI_Wtime();

    // now we can release region lock
    dspaces_unlock_on_write(lock_name, p_gcomm);
    sprintf(msg, "release the write lock");
    my_message(msg, rank, LOG_WARNING);

    if(ret_put != 0){
        perror("put err:");
        printf("put varaible %s err,  error number %d \n", var_name, ret_put);
        exit(-1);
    }
    else if(sync_ok != 0){
        perror("put err:");
        exit(-1);
    }
    else{
        sprintf(msg, "write %d elem to dspaces, each has %zu bytes", num_points, elem_size);
        my_message(msg, rank, LOG_WARNING);
    }
    *p_time_used = t2-t1;
}

/*
void get_common_buffer_unblocking(int timestep, int ndim, int bounds[6], int rank, MPI_Comm * p_gcomm,char * var_name, void **p_buffer,size_t elem_size, double *p_time_used){

    printf("\n ** prepare to get, ndim = %d\n", ndim);
    // how many number of elements are actually written
    //int num_elems;
    char msg[STRING_LENGTH];
    double t1, t2;
    int ret_get = -1;

    int num_points;
    
    uint64_t lb[3] = {0}, ub[3] = {0};
    lb[0] = bounds[0];
    lb[1] = bounds[1];
    lb[2] = bounds[2];
    //y
    ub[0] = bounds[3];
    //x
    ub[1] = bounds[4];
    ub[2] = bounds[5];

    num_points = (bounds[3]-bounds[0]+1)*(bounds[4]- bounds[1]+1)*(bounds[5]- bounds[2]+1);


#ifdef debug_1
    printf("lb: (%d, %d  %d), hb(%d, %d, %d), elem_size %zu bytes\n", bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5], elem_size);
#endif

    //dspaces_barrier();
    sprintf(msg, "bypass the read lock");
    my_message(msg, rank, LOG_WARNING);

    // read all regions in once
    t1 = MPI_Wtime();
    ret_get = dspaces_get(var_name, timestep, elem_size, ndim, lb, ub, *p_buffer);
    t2 = MPI_Wtime();

    if(ret_get != 0){
        perror("get err:");
        printf("get varaible %s err,  error number %d \n", var_name, ret_get);
    }else{
        sprintf(msg, "read %d elem from dspaces, each has %zu bytes", num_points, elem_size);
        my_message(msg, rank, LOG_WARNING);
    }

    *p_time_used = t2-t1;
    
}
*/
