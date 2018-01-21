/********************************************************
Copyright YUANKUN FU
Brief desc of the file: consumer thread
********************************************************/
#include "concurrent.h"

// #ifdef V_T
// #include <VT.h>
// static int class_id;
// static int analysis_id;
// #endif

void simple_verify(GV gv, LV lv, char* buffer, int nbytes, int source, int block_id, char* consumer_state_p){
  double atom_id, type;
  int time_step;
  double xs, ys, zs;
  double xs_sum = 0.0, ys_sum = 0.0, zs_sum = 0.0;
  int dump_lines_in_this_blk;

  // src,blkid,writer_state,consumer_state,time_step+data
  // gv->analysis_data_len = sizeof(int)*(1+1+1)+sizeof(char)*gv->block_size;

  // ((int *)(buffer))[0] -- source
  // ((int *)(buffer))[1] -- block_id
  // ((int *)(buffer))[2] -- writer_state

  int i;
  time_step = ((int *)(buffer))[3];
  dump_lines_in_this_blk = ((int *)(buffer))[4];

#ifdef DEBUG_PRINT
  printf("!!!!! Ana_Proc%d: Consumer%d simple_verify -- source=%d, block_id=%d, \
step=%d, gv->calc_counter=%d, *consumer_state_p=%d\n",
       gv->rank[0], lv->tid, source, block_id,
       time_step, gv->calc_counter, *consumer_state_p);
#endif //DEBUG_PRINT


  for (i = 0; i<(dump_lines_in_this_blk * 5); i += 5){
    atom_id = ((double *)(buffer+sizeof(int)* 4))[i];
    type = ((double *)(buffer+sizeof(int)* 4))[i+1];
    xs = ((double *)(buffer+sizeof(int)* 4))[i+2];
    ys = ((double *)(buffer+sizeof(int)* 4))[i+3];
    zs = ((double *)(buffer+sizeof(int)* 4))[i+4];

    // printf("Ana_Proc%d: atom_id=%.1f, type=%.1f, xs=%f, ys=%f, zs=%f\n",
    //   gv->rank[0], atom_id, type, xs, ys, zs);
    // fflush(stdout);

    xs_sum += xs;
    ys_sum += ys;
    zs_sum += zs;

  }


}

void calc_msd(GV gv, LV lv, char* buffer, int source, int block_id, int* consumer_state_p){
  double atom_id, type;
  int time_step;
  double xtmp, ytmp, ztmp;
  double dx, dy, dz;
  int dump_lines_in_this_blk;

  // msd[0] = msd[1] = msd[2] = msd[3] = 0.0;

  // src,blkid,writer_state,consumer_state,time_step+data
  // gv->analysis_data_len = sizeof(int)*(1+1+1)+sizeof(char)*gv->block_size;

  // ((int *)(buffer))[0] -- source
  // ((int *)(buffer))[1] -- block_id
  // ((int *)(buffer))[2] -- write_state
  // ((int *)(buffer))[3] -- calc_state

  int i;
  time_step = ((int *)(buffer))[4];
  dump_lines_in_this_blk = ((int *)(buffer))[5];

  // double navfac;
  // int avflag;    // avflag = 1 if using average position as reference
  // int naverage;  // number of samples for average position

  // avflag = 0;
  // naverage = 0;

  // if (avflag) {
  //   naverage++;
  //   navfac = 1.0/(naverage+1);
  // }

#ifdef DEBUG_PRINT
  printf("Ana_Proc%d: Consumer%d calc_msd_verify -- source=%d, block_id=%d, \
time_step=%d, dump_lines_in_this_blk=%d, gv->calc_counter=%d, *consumer_state_p=%d\n",
       gv->rank[0], lv->tid, source, block_id,
       time_step, dump_lines_in_this_blk, gv->calc_counter, *consumer_state_p);
#endif //DEBUG_PRINT

  // if(time_step==0){ //current do nothing

    // for (i = 0; i<(dump_lines_in_this_blk * 5); i+=5){
    //   atom_id = ((double *)(buffer+sizeof(int)*4))[i];
    //   type = ((double *)(buffer+sizeof(int)*4))[i+1];
    //   gv->xoriginal[atom_id-1] = ((double *)(buffer+sizeof(int)*4))[i+2];
    //   gv->yoriginal[atom_id-1] = ((double *)(buffer+sizeof(int)*4))[i+3];
    //   gv->zoriginal[atom_id-1] = ((double *)(buffer+sizeof(int)*4))[i+4];

    //   gv->step0_atom_cnt++;

    //   if(gv->step0_atom_cnt == (gv->num_atom/gv->compute_process_num*gv->computer_group_size)){
    //     MPI_Allreduce(gv->xoriginal,vector,gv->num_atom,MPI_DOUBLE,MPI_SUM,world);
    //     MPI_Allreduce(gv->yoriginal,vector,gv->num_atom,MPI_DOUBLE,MPI_SUM,world);
    //     MPI_Allreduce(gv->zoriginal,vector,gv->num_atom,MPI_DOUBLE,MPI_SUM,world);
    //   }
    // }
  // }
  // else{

    for (i = 0; i<(dump_lines_in_this_blk * 5); i+=5){
      int cnt=0;

      atom_id = ((double *)(buffer+sizeof(int)*6))[i];
      type = ((double *)(buffer+sizeof(int)*6))[i+1];
      xtmp = ((double *)(buffer+sizeof(int)*6))[i+2];
      ytmp = ((double *)(buffer+sizeof(int)*6))[i+3];
      ztmp = ((double *)(buffer+sizeof(int)*6))[i+4];

      // printf("Ana_Proc%d: atom_id=%.f, type=%.f, xs=%f, ys=%f, zs=%f\n",
      //   gv->rank[0], atom_id, type, xs, ys, zs);
      // fflush(stdout);

      /*calc xoriginal*/
      dx = xtmp;
      dy = ytmp;
      dz = ztmp;

#ifdef READ_STEP0
      dx = xtmp - gv->xoriginal[(int)atom_id-1];
      dy = ytmp - gv->yoriginal[(int)atom_id-1];
      dz = ztmp - gv->zoriginal[(int)atom_id-1];
#endif //READ_STEP0

      // if( (i>49) && (i<56) ){
      //     printf("Consumer%d: i=%d, xtmp=%.3f, ytmp=%.3f, ztmp=%.3f\n",
      //       gv->rank[0], i, xtmp, ytmp, ztmp);
      //     fflush(stdout);
      // }

      // if( ((xtmp<0) || (ytmp<0) || (ztmp<0)) && (cnt<5)){
      //     printf("Consumer%d: i=%d, xtmp=%.3f, ytmp=%.3f, ztmp=%.3f\n",
      //       gv->rank[0], i, xtmp, ytmp, ztmp);
      //     fflush(stdout);
      //     cnt++;
      // }

      gv->msd[0][time_step/gv->dump_step_internal] += dx*dx;
      gv->msd[1][time_step/gv->dump_step_internal] += dy*dy;
      gv->msd[2][time_step/gv->dump_step_internal] += dz*dz;
      gv->msd[3][time_step/gv->dump_step_internal] += dx*dx + dy*dy + dz*dz;

    }

  // }

}

void perform_msd_reduce(GV gv){
  MPI_Status status;
  int ana_rk_start, ana_rk_end, i;
  ana_rk_start = gv->compute_process_num;
  // ana_rk_end   = gv->compute_process_num + gv->analysis_process_num - 1;

  // printf("Ana%d Consumer: Lammps+DataBroker enter perform_msd_reduce\n", gv->rank[0]);
  // fflush(stdout);

  if(gv->analysis_process_num>1){
    MPI_Reduce(gv->msd[3], gv->msd[4], gv->total_num_dump_steps, MPI_DOUBLE, MPI_SUM, 0, gv->mycomm);

#ifdef PRINT_MSD_EACH_STEP
    if(gv->rank[0]==ana_rk_start)
      for(i=0; i<(gv->total_num_dump_steps); i++){
        printf("Consumer%d: Lammps+Zipper time_step=%d, msd=%.3f\n", gv->rank[0], (i+1)*gv->dump_step_internal, gv->msd[4][i]/gv->num_atom);
        fflush(stdout);
      }
#endif //PRINT_MSD_EACH_STEP
  }
}


//Consumer
char* consumer_ring_buffer_read_tail(GV gv, LV lv, int* consumer_state_p){
  char* pointer;

  ring_buffer *rb = gv->consumer_rb_p;

  pthread_mutex_lock(rb->lock_ringbuffer);
  while(1) {

    if (rb->num_avail_elements > 0) {
      pointer = rb->buffer[rb->tail];
      *consumer_state_p = ((int*)pointer)[3];
      pthread_mutex_unlock(rb->lock_ringbuffer);
      return pointer;
    }
    else {

#ifdef DEBUG_PRINT
    printf("Ana_Proc%d: Consumer%d Prepare to Sleep! rb->num_avail_elements=%d, rb->tail=%d\n", gv->rank[0], lv->tid, rb->num_avail_elements, rb->tail);
    fflush(stdout);
#endif //DEBUG_PRINT

      lv->wait++;
      pthread_cond_wait(rb->empty, rb->lock_ringbuffer);

#ifdef DEBUG_PRINT
    printf("Ana_Proc%d: Consumer%d Wake up! rb->num_avail_elements=%d, rb->tail=%d\n", gv->rank[0], lv->tid, rb->num_avail_elements, rb->tail);
    fflush(stdout);
#endif //DEBUG_PRINT

    }
  }
}

void consumer_ring_buffer_move_tail(GV gv, LV lv, int* flag_p, char* pointer){

  ring_buffer *rb = gv->consumer_rb_p;
  // int n=0;

  if(pointer!=NULL){
      while(1){

        pthread_mutex_lock(rb->lock_ringbuffer);

        if( ((int*)pointer)[2] == ON_DISK){

            rb->tail = (rb->tail + 1) % rb->bufsize;
            rb->num_avail_elements--;

#ifdef DEBUG_PRINT
            printf("Ana_Proc%d: Consumer%d has moved tail on source=%d, block_id=%d, write_state=%d, calc_state=%d, tail=%d, num_avail_elements=%d\n",
              gv->rank[0], lv->tid, ((int*)pointer)[0], ((int*)pointer)[1], ((int*)pointer)[2], ((int*)pointer)[3], rb->tail, rb->num_avail_elements);
            fflush(stdout);
#endif //DEBUG_PRINT
            pthread_cond_signal(rb->full);      //wake up receiver put
            pthread_cond_signal(rb->new_tail);  //wake up ana_writer
            pthread_mutex_unlock(rb->lock_ringbuffer);

            // the one who last know the state == both_done will free the pointer, in case of the last error case
            *flag_p=1;
            return;

        }
        pthread_mutex_unlock(rb->lock_ringbuffer);

// #ifdef DEBUG_PRINT
//         n++;
//         if(n%1000==0){
//           printf("Ana_Proc%d: Consumer%d move tail n=%d\n",
//               gv->rank[0], lv->tid, n);
//           fflush(stdout);
//         }
// #endif //DEBUG_PRINT
    }
  }
}

void analysis_consumer_thread(GV gv, LV lv){
  double t0 = 0, t1 = 0, t2 = 0, t3 = 0;
  char* pointer = NULL;
  double read_tail_wait_time=0, move_tail_wait_time=0, msd_reduce_time=0;
  int source = 0, block_id = 0;
  int flag = 0;
  int free_count = 0;
  int consumer_state;
  int num_exit_flag = 0;
  int remaining_elements;

  ring_buffer *rb = gv->consumer_rb_p;
  // printf("Ana_Proc%d: consumer%d is running!\n",gv->rank[0], lv->tid);
  // fflush(stdout);

// #ifdef V_T
//   VT_classdef( "Analysis", &class_id );
//   VT_funcdef("ANL", class_id, &analysis_id);
//   //VT_funcdef("GETBUF", class_id, &get_buffer_id);
// #endif

  t2 = MPI_Wtime();
  while (1) {

    flag = 0;

    t0 = MPI_Wtime();
    pointer = consumer_ring_buffer_read_tail(gv, lv, &consumer_state);
    t1 = MPI_Wtime();
    read_tail_wait_time += t1 - t0;

    if(pointer != NULL) {

      source = ((int*)pointer)[0];
      block_id = ((int*)pointer)[1];

#ifdef DEBUG_PRINT
     printf("Ana_Proc%d: Consumer%d ***GET A pointer*** source=%d block_id=%d, calc_counter=%d\n",
        gv->rank[0], lv->tid, ((int*)pointer)[0], ((int*)pointer)[1], gv->calc_counter);
     fflush(stdout);
#endif //DEBUG_PRINT

      if(block_id != EXIT_BLK_ID){

        if (consumer_state == NOT_CALC){

          if(block_id<0){
            printf("Fatal Error! Ana_Proc%d: Consumer%d Prepare to calc_n_moments source=%d, block_id=%d <0\
step=%d, dump_lines_per_blk=%d, gv->calc_counter=%d, consumer_state=%d\n",
              gv->rank[0], lv->tid, ((int *)pointer)[0], ((int *)pointer)[1],
              ((int *)pointer)[4], ((int *)pointer)[5], gv->calc_counter, consumer_state);
          }

#ifdef DEBUG_PRINT
          printf("Ana_Proc%d: Consumer%d <Prepare calc_n_moments> source=%d, block_id=%d \
step=%d, dump_lines_per_blk=%d, gv->calc_counter=%d, consumer_state=%d\n",
          gv->rank[0], lv->tid, ((int *)pointer)[0], ((int *)pointer)[1],
          ((int *)pointer)[4], ((int *)pointer)[5], gv->calc_counter, consumer_state);
#endif //DEBUG_PRINT

          t0 = MPI_Wtime();
// #ifdef V_T
//       VT_begin(analysis_id);
// #endif
          // simple_verify(gv, lv, pointer, gv->block_size, source, block_id, &consumer_state);
          calc_msd(gv, lv, pointer, source, block_id, &consumer_state);
// #ifdef V_T
//       VT_end(analysis_id);
// #endif
          t1 = MPI_Wtime();
          lv->calc_time += t1 - t0;
          gv->calc_counter++;

          pthread_mutex_lock(rb->lock_ringbuffer);
          ((int *)pointer)[3] = CALC_DONE;
          pthread_mutex_unlock(rb->lock_ringbuffer);

#ifdef DEBUG_PRINT
          if (gv->calc_counter%ANALSIS_COUNT == 0){
            printf("Ana_Proc%d: Consumer%d calc_counter %d\n", gv->rank[0], lv->tid, gv->calc_counter);
            fflush(stdout);
          }

          printf("Ana_Proc%d: --NOT_CALC-- Consumer%d finish calculating source=%d block_id=%d, flag=%d, calc_counter=%d\n",
            gv->rank[0], lv->tid, source, block_id, flag, gv->calc_counter);
          fflush(stdout);
#endif //DEBUG_PRINT

          t0 = MPI_Wtime();
          consumer_ring_buffer_move_tail(gv, lv, &flag, pointer);
          t1 = MPI_Wtime();
          move_tail_wait_time += t1-t0;

#ifdef DEBUG_PRINT
          printf("Ana_Proc%d: Consumer%d ***PASS-MOVE-TAIL*** source=%d block_id=%d, flag=%d, calc_counter=%d\n",
            gv->rank[0], lv->tid, ((int*)pointer)[0], ((int*)pointer)[1], flag, gv->calc_counter);
          fflush(stdout);
#endif //DEBUG_PRINT

          if(flag == 1){

            free_count++;

#ifdef DEBUG_PRINT
            printf("Ana_Proc%d: *****#####-----Analysis consumer Prepare to free! block_id=%d num_blk_free=%d\n", gv->rank[0], block_id, free_count);
            fflush(stdout);
#endif //DEBUG_PRINT

            free(pointer);

#ifdef DEBUG_PRINT
            printf("Ana_Proc%d: *****#####-----Analysis consumer Successfully free block_id=%d num_blk_free=%d\n", gv->rank[0], block_id, free_count);
            fflush(stdout);
#endif //DEBUG_PRINT

          }

        }
        else{
          printf("Ana_Proc%d: Consumer%d !!!---GET ERROR BLK with CALC_DONE---!!!\n", gv->rank[0], free_count);
          fflush(stdout);
        }

      }
      else{ //get the final EXIT_BLK_ID

        source = ((int *)pointer)[0];

#ifdef DEBUG_PRINT
        printf("Ana_Proc%d: Consumer --GET-- a EXIT_BLK_ID from source=%d and Prepare to free it\n", gv->rank[0], ((int *)pointer)[0]);
        fflush(stdout);
#endif //DEBUG_PRINT

        t0 = MPI_Wtime();
        consumer_ring_buffer_move_tail(gv, lv, &flag, pointer);
        t1 = MPI_Wtime();
        move_tail_wait_time += t1-t0;

        free(pointer);

        num_exit_flag++;

#ifdef DEBUG_PRINT
        printf("Ana_Proc%d: Consumer --MOVE & FREE-- a EXIT_BLK_ID from source=%d! num_exit_flag=%d\n", gv->rank[0], source, num_exit_flag);
        fflush(stdout);
#endif //DEBUG_PRINT

      }
    }
    else{
      printf("Ana_Proc%d: Consumer Error -- Get a NULL pointer\n", gv->rank[0]);
    }

    if (num_exit_flag >= gv->computer_group_size){

#ifdef DEBUG_PRINT
      printf("Ana_Proc%d: Consumer prepare to exit!\n", gv->rank[0]);
      fflush(stdout);
#endif //DEBUG_PRINT

      //set ana_writer exit
      gv->ana_writer_exit=1;

      pthread_mutex_lock(rb->lock_ringbuffer);
      remaining_elements=rb->num_avail_elements;
      pthread_cond_signal(rb->empty); //wake up potential asleep A_writer
      // pthread_cond_signal(rb->new_tail);
      pthread_mutex_unlock(rb->lock_ringbuffer);

#ifdef DEBUG_PRINT
      printf("Ana_Proc%d: Consumer prepare to exit! With ring_buffer num_avail_elements=%d\n",
        gv->rank[0], remaining_elements);
      fflush(stdout);
#endif //DEBUG_PRINT

      break;
    }
  }

  //Reduce all time step result
  t0 = MPI_Wtime();
  perform_msd_reduce(gv);
  t1 = MPI_Wtime();
  msd_reduce_time = t1-t0;

  t3 = MPI_Wtime();

 printf("Ana_Proc%04d: Consumer%d T_total=%.3f, T_calc=%.3f, T_rd_tail_wt=%.3f, \
T_mv_tail_wt=%.3f, T_reduce=%.3f, calc_cnt=%d, empty_wait=%d\n",
       gv->rank[0], lv->tid, t3-t2, lv->calc_time, read_tail_wait_time, move_tail_wait_time, msd_reduce_time, gv->calc_counter, lv->wait);
  fflush(stdout);
}
