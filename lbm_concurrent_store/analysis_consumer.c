/********************************************************
Copyright YUANKUN FU
Brief desc of the file: consumer thread
********************************************************/
#include "concurrent.h"
void simple_verify(GV gv, LV lv, char* buffer, int nbytes, int source, int block_id, char* consumer_state_p){
  register int i,j,k;
  //register int computeid=0;
  //int check = *(int *)(buffer);
  register double u;
  register double v;
  register double mean_u=0;
  register double mean_v=0;
  register int count=0,loop=0;
  int base;

  gv->computeid=*(int *)(buffer);
  int check_blkid = *(int *)(buffer+sizeof(int));

  gv->step=*(int *)(buffer+12);
  gv->CI=*(int *)(buffer+16);
  gv->CJ=*(int *)(buffer+20);
  gv->CK=*(int *)(buffer+24);
  #ifdef DEBUG_PRINT
  printf("!!!!! Node %d consumer %d simple_verify computeid = %d, check_blkid=%d, source=%d, block_id=%d, \
step=%d, i=%d, j=%d, k=%d, gv->calc_counter=%d, *consumer_state_p=%d\n",
          gv->rank[0], lv->tid, gv->computeid, check_blkid, source, block_id,
          gv->step, gv->CI, gv->CJ, gv->CK, gv->calc_counter,*consumer_state_p);
  #endif //DEBUG_PRINT

  for(base=0;base<gv->cubex*gv->cubey*gv->cubez/(CACHE*CACHE*CACHE);base++)
    for(loop=0;loop<gv->lp;loop++){
      count=0;
      for(i=0;i<CACHE;i++)
        for(j=0;j<CACHE;j++)
          for(k=0;k<CACHE;k++){
            // gv->gi=gv->originx+i;
            // gv->gj=gv->originy+j;
            // gv->gk=gv->originz+k;

            // u[gv->gi][gv->gj][gv->gk]=*((double *)(buffer+16+count));
            // v[gv->gi][gv->gj][gv->gk]=*((double *)(buffer+16+count+8));

            u=sqrt(*((double *)(buffer+28+count+CACHE*CACHE*CACHE*base)));
            v=sqrt(*((double *)(buffer+28+count+8+CACHE*CACHE*CACHE*base)));

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

void calc_n_moments(GV gv, LV lv, char* buffer, char* consumer_state_p){
  // register int i,j,k;
  //register int computeid=0;
  //int check = *(int *)(buffer);
  register double u,n_u0,n_u1;
  register double v,n_v0,n_v1;
  register double mean_u=0;
  register double mean_v=0;
  register double u1,v1;
  int loop=0;
  int base;
  double sum_u[NMOMENT];
  double sum_v[NMOMENT];

  gv->computeid=*(int *)(buffer);
  // int check_blkid = *(int *)(buffer+sizeof(int));

  gv->step=*(int *)(buffer+12);
  gv->CI=*(int *)(buffer+16);
  gv->CJ=*(int *)(buffer+20);
  gv->CK=*(int *)(buffer+24);
  #ifdef DEBUG_PRINT
  printf("!!!!! Node %d consumer %d simple_verify computeid = %d, check_blkid=%d, source=%d, block_id=%d, \
step=%d, i=%d, j=%d, k=%d, gv->calc_counter=%d, *consumer_state_p=%d\n",
          gv->rank[0], lv->tid, gv->computeid, check_blkid, source, block_id,
          gv->step, gv->CI, gv->CJ, gv->CK, gv->calc_counter,*consumer_state_p);
  #endif //DEBUG_PRINT

  //calc_mean
  for(base=0;base<gv->cubex*gv->cubey*gv->cubez;base=base+4){
    u=*((double *)(buffer+28+base*8));
    v=*((double *)(buffer+28+base*8+8));
    u1=*((double *)(buffer+28+base*8+16));
    v1=*((double *)(buffer+28+base*8+32));
    mean_u=mean_u+u+u1;
    mean_v=mean_v+v+v1;
  }
  mean_u=mean_u/(gv->cubex*gv->cubey*gv->cubez);
  mean_v=mean_v/(gv->cubex*gv->cubey*gv->cubez);
  // mean_u=10;
  // mean_v=10;
  //calc_n_momnents

  for(base=0;base<gv->cubex*gv->cubey*gv->cubez;base=base+4){
      for(loop=1;loop<=gv->lp;loop++){
        u=*((double *)(buffer+28+base*8));
        v=*((double *)(buffer+28+base*8+8));
        u1=*((double *)(buffer+28+base*8+16));
        v1=*((double *)(buffer+28+base*8+32));

        n_u0=pow(u-mean_u,loop);
        n_u1=pow(u1-mean_u,loop);
        n_v0=pow(v-mean_v,loop);
        n_v1=pow(v1-mean_v,loop);
        sum_u[loop-1]+=n_u0+n_u1;
        sum_v[loop-1]+=n_v0+n_v1;
    }
  }

  for(loop=1;loop<=gv->lp;loop++){
    sum_u[loop-1]=sum_u[loop-1]/(gv->cubex*gv->cubey*gv->cubez);
    sum_v[loop-1]=sum_v[loop-1]/(gv->cubex*gv->cubey*gv->cubez);
  }
}

//Consumer
char* consumer_ring_buffer_read_tail(GV gv,LV lv,char* consumer_state_p){
  char* pointer;
  // int* temp_int_pointer;
  ring_buffer *rb = gv->consumer_rb_p;

  pthread_mutex_lock(rb->lock_ringbuffer);
  while(1) {
    if (rb->num_avail_elements > 0) {
      pointer = rb->buffer[rb->tail];
      // temp_int_pointer = (int*) pointer;
      *consumer_state_p = pointer[9];
      pthread_mutex_unlock(rb->lock_ringbuffer);
      return pointer;
    }
    else {
      pthread_cond_wait(rb->empty, rb->lock_ringbuffer);
      // printf("Analysis %d consumer %d wake up!\n", gv->rank[0], lv->tid);
      // fflush(stdout);
    }
  }
}

void consumer_ring_buffer_move_tail(GV gv,LV lv,int* flag_p, char* pointer){
  // int* temp_int_pointer;
  ring_buffer *rb = gv->consumer_rb_p;

  // int state;

  if(pointer!=NULL){
      while(1){
        pthread_mutex_lock(rb->lock_ringbuffer);
      if(pointer[8] == ON_DISK){
        rb->tail = (rb->tail + 1) % rb->bufsize;
        rb->num_avail_elements--;

        pthread_cond_signal(rb->full);
        pthread_mutex_unlock(rb->lock_ringbuffer);
        // the one who last know the state == both_done will free the pointer, in case of the last error case
          *flag_p=1;
        return;
      }
      pthread_mutex_unlock(rb->lock_ringbuffer);
    }
  }
}

void analysis_consumer_thread(GV gv,LV lv){
  double t0=0, t1=0,t2=0,t3=0,t6=0,t7=0;
  char* pointer=NULL;
  double wait_time=0,move_tail_time=0;
  int* temp_int_pointer;
  int source=0,block_id=0;
  int flag=0;
  int free_count=0;
  char consumer_state;
  ring_buffer *rb = gv->consumer_rb_p;
  // printf("Analysis Process %d consumer thread %d is running!\n",gv->rank[0], lv->tid);
  // fflush(stdout);

  t2 = get_cur_time();
  while(1) {
    flag=0;
    t6 = get_cur_time();
    pointer = consumer_ring_buffer_read_tail(gv,lv,&consumer_state);
    t7 = get_cur_time();
    wait_time += t7-t6;

    if (pointer != NULL) {

      if(consumer_state==NOT_CALC){
        temp_int_pointer=(int*)pointer;
        source = temp_int_pointer[0];
        block_id = temp_int_pointer[1];

        t0 = get_cur_time();
        // simple_verify(gv, lv, pointer, gv->block_size,source,block_id,&consumer_state);
        calc_n_moments(gv, lv, pointer,&consumer_state);
        t1 = get_cur_time();
        lv->calc_time += t1 - t0;
        gv->calc_counter++;

        pthread_mutex_lock(rb->lock_ringbuffer);
        pointer[9] = CALC_DONE;
        pthread_mutex_unlock(rb->lock_ringbuffer);

        if(gv->calc_counter%ANALSIS_COUNT==0){
          printf("Analysis Process %d Consumer %d calc_counter %d\n", gv->rank[0], lv->tid, gv->calc_counter);
          fflush(stdout);
        }

        #ifdef DEBUG_PRINT
        printf("NOT_CALC: Analysis %d Consumer %d finish calculating source=%d block_id=%d, flag=%d, calc_counter=%d\n",
          gv->rank[0], lv->tid, source, block_id, flag, gv->calc_counter);
        fflush(stdout);
        #endif //DEBUG_PRINT
      }
    }

    consumer_ring_buffer_move_tail(gv,lv,&flag,pointer);

    if(flag==1){

      free_count++;

      #ifdef DEBUG_PRINT
      printf("*****#####-----Analysis consumer free! %d\n",free_count);
           fflush(stdout);
      #endif //DEBUG_PRINT

      free(pointer);
    }

    if(gv->calc_counter >= gv->ana_total_blks) break;
  }
  t3 = get_cur_time();

  printf("Analysis Process %d consumer ####Exp2 consumer total ####total consumer time= %f, \
calc_time= %f, wait_time= %f, move_tail_time=%f, my_count=%d, free_count=%d\n",
   gv->rank[0], t3-t2, lv->calc_time, wait_time, move_tail_time, gv->calc_counter, free_count);
  fflush(stdout);
}
