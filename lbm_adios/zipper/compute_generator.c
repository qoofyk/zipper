/********************************************************
Copyright YUANKUN FU
Brief desc of the file: Generator thread, creat_blk, put blk to producer_ring_buffer
********************************************************/
#include "concurrent.h"

//put the buffer into ring_buffer
void producer_ring_buffer_put(GV gv, char * buffer, int* full){

  ring_buffer *rb;
  int writer_on = gv->producer_rb_p->bufsize * gv->writer_prb_thousandth / 1000;

  rb = (ring_buffer *) gv->producer_rb_p;

  pthread_mutex_lock(rb->lock_ringbuffer);
  while(1) {
    if (rb->num_avail_elements < rb->bufsize) {
      rb->buffer[rb->head] = buffer;
      //gen_check_blk(gv, rb->buffer[rb->head],gv->block_size);
      rb->head = (rb->head + 1) % rb->bufsize;
      ++rb->num_avail_elements;

      if ( rb->num_avail_elements >= writer_on ){
        pthread_cond_signal(rb->empty);
        pthread_cond_signal(gv->writer_on);
      }
      else
        pthread_cond_signal(rb->empty);

      pthread_mutex_unlock(rb->lock_ringbuffer);
      return;
    } else {
      ++(*full);
      pthread_cond_wait(rb->full, rb->lock_ringbuffer);
    }
  }
}

