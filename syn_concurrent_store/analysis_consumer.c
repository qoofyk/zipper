/********************************************************
Copyright YUANKUN FU
Brief desc of the file: consumer thread
********************************************************/
#include "do_thread.h"

void simple_verify(GV gv, LV lv, char* buffer, int nbytes, int source, int block_id, int* consumer_state_p){
  int i, j;
  int x;
  int check = *((int *)(buffer+sizeof(int)*4));

#ifndef CONCURRENT_EXP
  double sum=0, tmp=0;
#endif //CONCURRENT_EXP

  //printf("start calc!\n");
  //printf("before calc data is %d\n",buffer[0]);
  //gv->computeid=*(int *)(buffer);
  // printf("calc=%d\n", (nbytes/4-2)*gv->lp);

  for(i=0; i<nbytes; i=i+sizeof(int)) {
    for(j=0; j<gv->lp;j++){
      x = *((int *)(buffer+sizeof(int)*4+i));

      if(x!=(check+i*i)){
        printf("!!!!!simple_verify i=%d j=%d Wrong! x=%d, check=%d, source=%d, block_id=%d, gv->calc_counter=%d, *consumer_state_p=%d, \
((int*)buffer)[0]=%d, ((int*)buffer)[1]=%d, ((int*)buffer)[2]=%d, ((int*)buffer)[3]=%d, ((int*)buffer)[4]=%d, ((int*)buffer)[5]=%d\n",
          i, j, x, check, source, block_id, gv->calc_counter, *consumer_state_p,
          ((int*)buffer)[0], ((int*)buffer)[1], ((int*)buffer)[2], ((int*)buffer)[3], ((int*)buffer)[4], ((int*)buffer)[5]);
        fflush(stdout);
        return;
      }

#ifndef CONCURRENT_EXP
      tmp = sqrt((double)x);
#endif //CONCURRENT_EXP

    }
#ifndef CONCURRENT_EXP
    sum = sum + tmp;
#endif //CONCURRENT_EXP
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

void analysis_consumer_thread(GV gv,LV lv){
  double t0=0, t1=0, t2=0, t3=0;
  char* pointer=NULL;
  double read_tail_wait_time=0, move_tail_wait_time=0;
  int source=0, block_id=0;
  int flag=0;
  int free_count=0;
  int consumer_state;
  int num_exit_flag=0;
  int remaining_elements;

  ring_buffer *rb = gv->consumer_rb_p;
  // printf("Analysis Process %d consumer thread %d is running!\n",gv->rank[0], lv->tid);
  // fflush(stdout);

  t2 = get_cur_time();
  while(1) {
    flag=0;

    t0 = get_cur_time();
    pointer = consumer_ring_buffer_read_tail(gv, lv, &consumer_state);
    t1 = get_cur_time();
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

// #ifdef DEBUG_PRINT
          if(block_id<0){
            printf("Ana_Proc%d: Consumer%d Prepare to simple_verify source=%d, block_id=%d \
gv->calc_counter=%d, consumer_state=%d\n",
              gv->rank[0], lv->tid, ((int *)pointer)[0], ((int *)pointer)[1],
              gv->calc_counter, consumer_state);
          }
// #endif //DEBUG_PRINT

          t0 = get_cur_time();
          simple_verify(gv, lv, pointer, gv->block_size, source, block_id, &consumer_state);
          t1 = get_cur_time();
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

          t0 = get_cur_time();
          consumer_ring_buffer_move_tail(gv, lv, &flag, pointer);
          t1 = get_cur_time();
          move_tail_wait_time += t1-t0;

#ifdef DEBUG_PRINT
          printf("Ana_Proc%d: Consumer%d ***PASS-MOVE-TAIL*** source=%d block_id=%d, flag=%d, calc_counter=%d\n",
            gv->rank[0], lv->tid, ((int*)pointer)[0], ((int*)pointer)[1], flag, gv->calc_counter);
          fflush(stdout);
#endif //DEBUG_PRINT

          if(flag==1){

            free_count++;

#ifdef DEBUG_PRINT
            printf("Ana_Proc%d: *****#####-----Analysis consumer Prepare to free! num_blk_free=%d source=%d block_id=%d\n",
              gv->rank[0], free_count, ((int*)pointer)[0], ((int*)pointer)[1]);
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

        t0 = get_cur_time();
        consumer_ring_buffer_move_tail(gv, lv, &flag, pointer);
        t1 = get_cur_time();
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

    if (num_exit_flag >= gv->computer_group_size){

#ifdef DEBUG_PRINT
      printf("Ana_Proc%d: Consumer prepare to exit!\n", gv->rank[0]);
      fflush(stdout);
#endif //DEBUG_PRINT

      //set ana_writer exit
      gv->ana_writer_done=1;

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
  t3 = get_cur_time();

  printf("Ana_Proc%04d: Consumer T_total_consumer=%.3f, T_calc=%.3f, T_read_tail_wait=%.3f, \
T_move_tail_wait=%.3f, gv->calc_counter=%d, free_count=%d, num_avail_elements=%d\n",
       gv->rank[0], t3 - t2, lv->calc_time, read_tail_wait_time, move_tail_wait_time, gv->calc_counter, free_count, remaining_elements);
  fflush(stdout);
}
