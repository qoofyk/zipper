#include "concurrent.h"

status_t insert_zipper(GV gv, double **x, int nlocal, int step){
  int i,j;            //i: current line
  double xs, ys, zs;  //atom coordinate
  double atom_id, type;
  int total_num_double, num_blk_cur_step;
  char* buffer;
  double t0, t1, producer_ring_buffer_put_time=0.0;
  int size_one = SIZE_ONE;

  total_num_double = nlocal * size_one;
  num_blk_cur_step = ceil(((double)nlocal)/((double)gv->dump_lines_per_blk));


  if(gv->rank[0]==0){
    printf("timestep=%d\n", step);
    fflush(stdout);
#ifdef DEBUG_PRINT
    printf("Comp_Proc%04d: timestep=%d dump %d lines, generate %d Zipper_Blks with size_one=%d\n",
      gv->rank[0], step, nlocal, num_blk_cur_step, size_one);
    fflush(stdout);
#endif //DEBUG_PRINT
  }

  int cur_line=0; //current line in zipper block
  for(i=0; i<nlocal; i++){

    if(cur_line%gv->dump_lines_per_blk == 0){

      buffer = (char*) malloc(sizeof(char)*gv->compute_data_len);
      check_malloc(buffer);

      ((int *)buffer)[0] = gv->data_id++;
      ((int *)buffer)[1] = step;
      ((int *)buffer)[2] = gv->dump_lines_per_blk;
      j=0;  //buffer offset set to zero

// #ifdef DEBUG_PRINT
//       printf("Comp_Proc%d: Lammps generate block_id=%d, timestep=%d, cur_line=%d\n",
//         me, ((int *)buffer)[0], update->ntimestep, cur_line);
//       fflush(stdout);
// #endif //DEBUG_PRINT
    }



    ((double *)(buffer+sizeof(int)*3))[j]   = i;        //atom_id, need to specify in future
    ((double *)(buffer+sizeof(int)*3))[j+1] = 1;        //type
    ((double *)(buffer+sizeof(int)*3))[j+2] = x[i][0];  //xs
    ((double *)(buffer+sizeof(int)*3))[j+3] = x[i][1];  //ys
    ((double *)(buffer+sizeof(int)*3))[j+4] = x[i][2];  //zs


    j+=5;
    cur_line++;

    if( (cur_line)%gv->dump_lines_per_blk==0 || cur_line>=nlocal ){

      ((int *)buffer)[2] = j/5;  //how many lines are dumped into this block

#ifdef DEBUG_PRINT
      printf("Comp_Proc%d: Lammps put a block_id=%d, timestep=%d with lines %d into PRB\n",
        me, ((int *)buffer)[0], step, ((int *)buffer)[2]);
      fflush(stdout);
#endif //DEBUG_PRINT

      t0 = MPI_Wtime();

      producer_ring_buffer_put(gv,buffer);

      t1 = MPI_Wtime();
      producer_ring_buffer_put_time += t1-t0;
    }

  }

  gv->dump_step_cnt++;

#ifdef DEBUG_PRINT
  if(gv->data_id%200 == 0){
    printf("Comp_Proc%d: Lammps has created %d data_broker_blks, gv->dump_step_cnt=%d\n",
      gv->rank[0], gv->data_id, gv->dump_step_cnt);
    fflush(stdout);
  }
#endif //DEBUG_PRINT

  if(cur_line != nlocal)
    printf("Error: insert_into_DataBroker Missing original data\n");

  return S_OK;
}

status_t generate_exit_msg(GV gv){
  char* buffer;

  double producer_ring_buffer_put_time=0.0;
  double t0, t1;

  //generate the exit message
  if(gv->dump_step_cnt>=gv->total_num_dump_steps){

    buffer = (char*) malloc(sizeof(char)*gv->compute_data_len);
    check_malloc(buffer);

    ((int *)buffer)[0] = EXIT_BLK_ID;
    // ((int *)buffer)[1]= -1;
    // ((int *)buffer)[2]= -1;

#ifdef DEBUG_PRINT
    printf("Comp_Proc%d: Lammps generate the EXIT block_id=%d in timestep=%d with total_blks %d\n",
      gv->rank[0], ((int *)buffer)[0], step, gv->data_id);
    fflush(stdout);
#endif //DEBUG_PRINT

    t0 = MPI_Wtime();
    producer_ring_buffer_put(gv, buffer);
    t1 = MPI_Wtime();
    producer_ring_buffer_put_time += t1-t0;

#ifdef DEBUG_PRINT
    printf("Comp_Proc%d: Lammps finished put the EXIT block_id=%d in timestep=%d with total_blks %d, T_put=%.3f\n",
      gv->rank[0], ((int *)buffer)[0], step, gv->data_id, producer_ring_buffer_put_time);
    fflush(stdout);
#endif //DEBUG_PRINT
  }

  return S_OK;
}

