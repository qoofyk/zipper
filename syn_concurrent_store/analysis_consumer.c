/********************************************************
Copyright YUANKUN FU
Brief desc of the file: consumer thread
********************************************************/
#include "do_thread.h"
// void simple_verify(GV gv, LV lv, char* buffer,  int nbytes, int source, int block_id, int* state_p){
//   int i,j;
//   double sum=0,tmp=0;
//   register int x;
//   int check = *((int *)(buffer+sizeof(int)));

//   //printf("start calc!\n");
//   //printf("before calc data is %d\n",buffer[0]);
//   //gv->computeid=*(int *)(buffer);
//   // printf("calc=%d\n", (nbytes/4-2)*gv->lp);

//   for(i = 0; i < (nbytes/4-2); i=i+4) {
//     for(j=0; j<gv->lp;j++){
//       x = *(int *)(buffer+sizeof(int)*3+i);
//       if(x!=check+i){
//         printf("!!!!!simple_verify %d Wrong! x = %d, check=%d, source=%d, block_id=%d, gv->calc_counter=%d, *state_p=%d\n",
//           i, x, check,source,block_id,gv->calc_counter,*state_p);
//         fflush(stdout);
//         break;
//       }
//       tmp = sqrt((double)x);
//     }
//     sum = sum + tmp;
//   }
// }

void simple_verify(GV gv, LV lv, char* buffer,  int nbytes, int source, int block_id, char* consumer_state_p){
  int i,j;
  double sum=0,tmp=0;
  register int x;
  int check = *((int *)(buffer));

  //printf("start calc!\n");
  //printf("before calc data is %d\n",buffer[0]);
  //gv->computeid=*(int *)(buffer);
  // printf("calc=%d\n", (nbytes/4-2)*gv->lp);

  for(i = 0; i < (nbytes/4-2); i=i+4) {
    for(j=0; j<gv->lp;j++){
      x = *(int *)(buffer+i);
      if(x!=check+i){
        printf("!!!!!simple_verify %d Wrong! x = %d, check=%d, source=%d, block_id=%d, gv->calc_counter=%d, *consumer_state_p=%d\n",
          i, x, check,source,block_id,gv->calc_counter,*consumer_state_p);
        fflush(stdout);
        return;
      }
      tmp = sqrt((double)x);
    }
    sum = sum + tmp;
  }
}

// void read_mark_remove(GV gv, char* pointer,  int nbytes){
//   char file_name[64];
//   //FILE *fp;
//   int blk_id;
//   //int computeid=0;

//   //computeid=*(int *)(pointer);
//   blk_id = *((int *)(pointer+nbytes-8));
//   sprintf(file_name,ADDRESS,gv->compute_process_num, gv->analysis_process_num,gv->computeid,gv->computeid,blk_id);
//   // sprintf(file_name,"/var/tmp/exp2_file_blk%d.data",blk_id);
//   // if(remove(file_name)== -1){
//   //   printf("Consumer cannot remove file %d!\n",blk_id);
//   // }
//   remove(file_name);
//   //printf("Consumer remove file %d Done!\n",blk_id);
// }


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
  double t0=0, t1=0,t2=0,t3=0,t4=0,t5=0;
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

    t4 = get_cur_time();
    pointer = consumer_ring_buffer_read_tail(gv,lv,&consumer_state);
    t5 = get_cur_time();
    wait_time += t5-t4;

    if (pointer != NULL) {

      if(consumer_state==NOT_CALC){
        temp_int_pointer=(int*)pointer;
        source = temp_int_pointer[0];
        block_id = temp_int_pointer[1];

        t0 = get_cur_time();
        simple_verify(gv, lv, pointer+3*sizeof(int), gv->block_size,source,block_id,&consumer_state);
        t1 = get_cur_time();
        lv->calc_time += t1 - t0;
        gv->calc_counter++;

        if(gv->rank[0]==gv->compute_process_num && gv->calc_counter%ANALSIS_COUNT==0){
          printf("Analysis Process %d Consumer %d calc_counter %d\n", gv->rank[0], lv->tid, gv->calc_counter);
          fflush(stdout);
        }

        pthread_mutex_lock(rb->lock_ringbuffer);
        pointer[9] = CALC_DONE;
        pthread_mutex_unlock(rb->lock_ringbuffer);

        #ifdef DEBUG_PRINT
        printf("NOT_CALC: Analysis %d Consumer %d finish calculating source=%d block_id=%d, flag=%d, calc_counter=%d\n",
          gv->rank[0], lv->tid, source, block_id, flag, gv->calc_counter);
        fflush(stdout);
        #endif //DEBUG_PRINT
      }
    }

    t4 = get_cur_time();
    consumer_ring_buffer_move_tail(gv,lv,&flag,pointer);
    t5 = get_cur_time();
    wait_time += t5-t4;

    if(flag==1){

      free_count++;

      #ifdef DEBUG_PRINT
      printf("*****#####-----Analysis consumer free! %d\n",free_count);
      fflush(stdout);
      #endif //DEBUG_PRINT

      free(pointer);
    }

    if(gv->calc_counter >= gv->ana_total_blks) {
      // printf("*****#####-----Analysis consumer Out! %d\n",free_count);
      // fflush(stdout);
      pthread_cond_broadcast(rb->empty);
      break;
    }
  }
  t3 = get_cur_time();

  printf("Analysis Process %d consumer%d ####total consumer time= %f,calc_time= %f, wait_time= %f, move_tail_time=%f, calc_counter=%d, free_count=%d\n",
   gv->rank[0], lv->tid, t3-t2, lv->calc_time, wait_time, move_tail_time, gv->calc_counter, free_count);
  fflush(stdout);
}
