/********************************************************
Copyright YUANKUN FU
Brief desc of the file: Generator thread, creat_blk, put blk to producer_ring_buffer
********************************************************/
#include "do_thread.h"

void create_blk(char* buffer,  int nbytes, int last_gen) {
   int i;

  for(i = 0; i < (nbytes/4); i=i+4) {
    *((int *)(buffer+i))=last_gen+i;
    // switch (i%4){
    //     case 0:buffer[i] = 0x02;break;
    //     case 1:buffer[i] = 0x03;break;
    //     case 2:buffer[i] = 0x04;break;
    //     case 3:buffer[i] = 0x05;break;
    //     default:printf("error\n");
    // }
  }
}

// void gen_check_blk(GV gv, char* buffer, long int nbytes){
//   long int i;
//   int j;
//   register int x;
//   register double y;

//   //printf("start calc!\n");
//   //printf("before calc data is %d\n",buffer[0]);

//   for(j=0;j<gv->lp;j++){
//     for(i = 0; i < (nbytes/4); i=i+4) {
//         x = *(int *)(buffer+i);
//         if(x!=0x05040302){
//           printf("%x\n", x);
//           printf("GEN Checking!!! Exp2 %ld\n", i);
//           printf("GEN Checking!!! Exp2 consumer_count = %ld Wrong!\n", gv->calc_counter);
//           printf("GEN Checking!!! Exp2 %ld Wrong!\n", i);
//         }
//         y = sqrt((double)x);
//     }
//   }
// }

//put the buffer into ring_buffer
void producer_ring_buffer_put(GV gv,LV lv,char * buffer){

  ring_buffer *rb;
  rb = (ring_buffer *) gv->producer_rb_p;

  pthread_mutex_lock(rb->lock_ringbuffer);
  while(1) {
    if (rb->num_avail_elements < rb->bufsize) {
      rb->buffer[rb->head] = buffer;
      //gen_check_blk(gv, rb->buffer[rb->head],gv->block_size);
      rb->head = (rb->head + 1) % rb->bufsize;
      rb->num_avail_elements++;
      pthread_cond_signal(rb->empty);
      pthread_mutex_unlock(rb->lock_ringbuffer);
      return;
    } else {
      pthread_cond_wait(rb->full, rb->lock_ringbuffer);
    }
  }
}


void generator_thread(GV gv,LV lv){
  int last_gen=0;
  double t0=0,t1=0;
  double t2=0,t3=0;
  double gen_time=0;
  char* buffer=NULL;
  // printf("Generator thread %d is running!\n",lv->tid);

  t0 = get_cur_time();

  while(1){
    pthread_mutex_lock(&gv->lock_generator);
    last_gen = gv->generator_counter++;
    pthread_mutex_unlock(&gv->lock_generator);

    if(last_gen>=gv->total_blks) break;

    buffer = (char*) malloc(sizeof(char)*gv->block_size);
    if (!buffer){
        fprintf(stderr, "Failed to malloc memory!!!!!!!!!!!!!!!!!!!!!!!!\n");
        exit(1);
    }
    t2 = get_cur_time();
    create_blk(buffer,gv->block_size, last_gen);
    t3 = get_cur_time();
    gen_time += t3-t2;
    //gen_check_blk(gv, buffer,gv->block_size);
    producer_ring_buffer_put(gv,lv,buffer);
  }
  t1 = get_cur_time();

  //free(buffer);

  printf("Generator %d gen_time = %f total time = %f\n", lv->tid, gen_time, t1-t0);
}
