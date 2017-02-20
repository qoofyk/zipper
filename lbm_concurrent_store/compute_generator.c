/********************************************************
Copyright YUANKUN FU
Brief desc of the file: Generator thread, creat_blk, put blk to producer_ring_buffer
********************************************************/
#include "concurrent.h"

// void create_blk(char* buffer,  int nbytes, int block_id) {
//    int i;

//   for(i = 0; i < (nbytes/4); i=i+4) {
//     *((int *)(buffer+i))=block_id+i;
//   }
// }

// void mark(char* buffer, int nbytes, int block_id){
//   //*((long int *)&(buffer[nbytes-8]))=block_id;
//   *((int *)(buffer+nbytes-8))=block_id;
// }

//put the buffer into ring_buffer
void producer_ring_buffer_put(GV gv,char * buffer){

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


// void generator_thread(GV gv,LV lv){
//   int block_id=0;
//   double t0=0,t1=0,t2=0,t3=0;
//   char* buffer=NULL;
//   printf("Generator thread %d is running!\n",lv->tid);

//   t2 = get_cur_time();

//   while(1){
//     pthread_mutex_lock(&gv->lock_block_id);
//     block_id = gv->data_id++;
//     pthread_mutex_unlock(&gv->lock_block_id);

//     if(block_id>=gv->cpt_total_blks) break;

//     t0 = get_cur_time();
//     buffer = (char*) malloc(gv->compute_data_len);
//     check_malloc(buffer);
//     create_blk(buffer,gv->block_size, block_id);
//     mark(buffer,gv->block_size, block_id);
//     t1 = get_cur_time();
//     lv->gen_time += t1 - t0;

//     producer_ring_buffer_put(gv,lv,buffer);

//   }
//   t3 = get_cur_time();

//   printf("Generator %d create_time=%f, total time is %f\n", lv->tid, lv->gen_time,t3-t2 );
// }
