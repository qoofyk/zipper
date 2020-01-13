/********************************************************
Copyright YUANKUN FU
Brief desc of the file: LBM consumer thread
********************************************************/
#include "concurrent.h"

#ifdef V_T
#include <VT.h>
int class_id;
int analysis_id;
#endif

void simple_verify(GV gv, LV lv, char* buffer, int nbytes, int* consumer_state_p){
  register int i,j,k;
  //register int computeid=0;
  //int check = *(int *)(buffer);
  register double u;
  register double v;
  register double mean_u=0;
  register double mean_v=0;
  register int count=0,loop=0;
  int base;

  for(base=0;base<gv->cubex*gv->cubey*gv->cubez/(CACHE*CACHE*CACHE);base++)
    for(loop=0;loop<gv->n_moments;loop++){
      count=0;
      for(i=0;i<CACHE;i++)
        for(j=0;j<CACHE;j++)
          for(k=0;k<CACHE;k++){
            // gv->gi=gv->originx+i;
            // gv->gj=gv->originy+j;
            // gv->gk=gv->originz+k;

            // u[gv->gi][gv->gj][gv->gk]=*((double *)(buffer+16+count));
            // v[gv->gi][gv->gj][gv->gk]=*((double *)(buffer+16+count+8));

            u=sqrt(*((double *)(buffer+count+CACHE*CACHE*CACHE*base)));
            v=sqrt(*((double *)(buffer+count+sizeof(double)+CACHE*CACHE*CACHE*base)));

            mean_u=mean_u+u;
            mean_v=mean_v+v;

            count+=16;
          }
      mean_u=mean_u/(gv->cubex*gv->cubey*gv->cubez);
      mean_v=mean_v/(gv->cubex*gv->cubey*gv->cubez);

      // sqrt(mean_u);
      // sqrt(mean_v);
    }
}


void calc_n_moments(GV gv, char* buffer, double* sum_vx, double* sum_vy){
// void calc_n_moments(GV gv, char* buffer){

  double vx, vy;
  double vx_prime, vy_prime;
  double mean_vx=0, mean_vy=0;
  // double sum_vx[NMOMENT], sum_vy[NMOMENT];

  int i, j;
  int num_points = gv->cubex*gv->cubey*gv->cubez;
  int num_double = EACH_FLUID_NUM_DOUBLE*num_points;

  //initialize
  for(i=1;i<=gv->n_moments;i++){
    sum_vx[i-1]=0;
    sum_vy[i-1]=0;
    // sum_vz[i-1]=0;
  }

  //calc_mean
  for(i=0; i<num_double; i+=EACH_FLUID_NUM_DOUBLE){
    vx = ((double *)buffer)[i];
    vy = ((double *)buffer)[i+1];
    // vz = ((double *)buffer)[i+2];
    //rho = ((double *)buffer)[i+3];

    mean_vx += vx;
    mean_vy += vy;
    // mean_vz += vz;

  }
  mean_vx=mean_vx/num_points;
  mean_vy=mean_vy/num_points;
  // mean_vz=mean_vz/num_points;

  //calc_n_momnents

  for(i=0; i<num_double; i+=EACH_FLUID_NUM_DOUBLE){

    vx = ((double *)buffer)[i];
    vy = ((double *)buffer)[i+1];
    // vz = ((double *)buffer)[i+2];
    // rho = ((double *)buffer)[i+3];

    //compute Flutuation field (velocity - velocity_mean)
    vx_prime = vx - mean_vx;
    vy_prime = vy - mean_vy;
    // uz = rho * (vz - mean_vz);

    //power ^2 -- variance; power ^3 -- skewness; power ^4 -- flatness
    for(j=1;j<=gv->n_moments;j++){
        sum_vx[j-1] += pow(vx_prime, j);
        sum_vy[j-1] += pow(vy_prime, j);
        // sum_vz[j-1] += pow(vz_prime, j);
    }
  }

  for(i=1;i<=gv->n_moments;i++){
    sum_vx[i-1]=sum_vx[i-1]/num_points;
    sum_vy[i-1]=sum_vy[i-1]/num_points;
    // sum_uz[i-1]=sum_vz[i-1]/num_points;
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
            pthread_cond_broadcast(rb->full);      //wake up receiver put and reader put
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
  double t0=0, t1=0, t2=0, t3=0;
  char* pointer=NULL;
  double read_tail_wait_time=0, move_tail_wait_time=0;
  int source=0, block_id=0;
  int flag=0;
  int free_count=0;
  int consumer_state;
  int num_exit_flag=0;
  int remaining_elements;
  double sum_vx[NMOMENT], sum_vy[NMOMENT];

  ring_buffer *rb = gv->consumer_rb_p;
  // printf("Analysis Process %d consumer thread %d is running!\n",gv->rank[0], lv->tid);
  // fflush(stdout);

#ifdef V_T
  VT_classdef( "Analysis", &class_id );
  VT_funcdef("ANL", class_id, &analysis_id);
  //VT_funcdef("GETBUF", class_id, &get_buffer_id);
#endif

  t2 = MPI_Wtime();
  while(1) {
    flag=0;

    t0 = MPI_Wtime();
    pointer = consumer_ring_buffer_read_tail(gv, lv, &consumer_state);
    t1 = MPI_Wtime();
    read_tail_wait_time += t1-t0;

    if(pointer != NULL) {

      source = ((int*)pointer)[0];
      block_id = ((int*)pointer)[1];

// #ifdef DEBUG_PRINT
//       printf("Ana_Proc%d: Consumer%d ***GET A pointer*** source=%d block_id=%d, calc_counter=%d\n",
//         gv->rank[0], lv->tid, ((int*)pointer)[0], ((int*)pointer)[1], gv->calc_counter);
//       fflush(stdout);
// #endif //DEBUG_PRINT

      if(block_id != EXIT_BLK_ID){

        if(consumer_state == NOT_CALC){

          gv->step = ((int *)pointer)[4];
          gv->CI   = ((int *)pointer)[5];
          gv->CJ   = ((int *)pointer)[6];
          gv->CK   = ((int *)pointer)[7];

// #ifdef DEBUG_PRINT
          if(block_id<0){
            printf("Ana_Proc%d: Consumer%d Prepare to calc_n_moments source=%d, block_id=%d \
step=%d, i=%d, j=%d, k=%d, gv->calc_counter=%d, consumer_state=%d\n",
              gv->rank[0], lv->tid, ((int *)pointer)[0], ((int *)pointer)[1],
              ((int *)pointer)[4], ((int *)pointer)[5], ((int *)pointer)[6],
              ((int *)pointer)[7], gv->calc_counter, consumer_state);
          }
// #endif //DEBUG_PRINT

          t0 = MPI_Wtime();
#ifdef V_T
      VT_begin(analysis_id);
#endif
          // simple_verify(gv, lv, pointer, gv->block_size, &consumer_state);
          // calc_n_moments(gv, lv, pointer+sizeof(int)*8, &consumer_state);
          calc_n_moments(gv, pointer+sizeof(int)*8, sum_vx, sum_vy);
          // calc_n_moments(gv, pointer+sizeof(int)*8);
#ifdef V_T
      VT_end(analysis_id);
#endif
          t1 = MPI_Wtime();
          lv->calc_time += t1 - t0;
          gv->calc_counter++;

          pthread_mutex_lock(rb->lock_ringbuffer);
          ((int *)pointer)[3] = CALC_DONE;
          pthread_mutex_unlock(rb->lock_ringbuffer);

#ifdef DEBUG_PRINT
          if(gv->calc_counter%ANALSIS_COUNT==0){
            printf("Ana_Proc%d: Consumer%d calc_counter %d\n", gv->rank[0], lv->tid, gv->calc_counter);
            fflush(stdout);
          }

          printf("Ana_Proc%d: Consumer%d ***PASS-Assign-CALC_DONE*** calculating source=%d block_id=%d, flag=%d, calc_counter=%d\n",
            gv->rank[0], lv->tid, ((int*)pointer)[0], ((int*)pointer)[1], flag, gv->calc_counter);
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

          if(flag==1){

            free_count++;

#ifdef DEBUG_PRINT
            printf("Ana_Proc%d: *****#####-----Analysis consumer Prepare to free! num_blk_free=%d\n", gv->rank[0], free_count);
            fflush(stdout);
#endif //DEBUG_PRINT

            free(pointer);

#ifdef DEBUG_PRINT
            printf("Ana_Proc%d: *****#####-----Analysis consumer Successfully free! num_blk_free=%d\n", gv->rank[0], free_count);
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
        printf("Ana_Proc%d: Consumer get a EXIT_BLK_ID from source=%d and Prepare to free it\n", gv->rank[0], ((int *)pointer)[0]);
        fflush(stdout);
#endif //DEBUG_PRINT

        t0 = MPI_Wtime();
        consumer_ring_buffer_move_tail(gv, lv, &flag, pointer);
        t1 = MPI_Wtime();
        move_tail_wait_time += t1-t0;

        free(pointer);

        num_exit_flag++;

#ifdef DEBUG_PRINT
        printf("Ana_Proc%d: Consumer get a EXIT_BLK_ID from source=%d! num_exit_flag=%d\n", gv->rank[0], source, num_exit_flag);
        fflush(stdout);
#endif //DEBUG_PRINT

      }
    }
    else{
      printf("Ana_Proc%d: Consumer Error -- Get a NULL pointer\n", gv->rank[0]);
    }

    if(num_exit_flag >= gv->computer_group_size){

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
  t3 = MPI_Wtime();

  printf("Ana_Proc%04d: Consumer%d T_total=%.3f, T_calc=%.3f, T_rd_tail_wt=%.3f, \
T_mv_tail_wt=%.3f, calc_cnt=%d, empty_wait=%d\n",
       gv->rank[0], lv->tid, t3-t2, lv->calc_time, read_tail_wait_time, move_tail_wait_time, gv->calc_counter, lv->wait);
  fflush(stdout);
}
