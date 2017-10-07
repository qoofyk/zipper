/********************************************************
Copyright YUANKUN FU
Brief desc of the file: Generator thread, creat_blk, put blk to producer_ring_buffer
********************************************************/
#include "do_thread.h"

void create_blk(char* buffer, int nbytes, int block_id, int computation_lp) {
  int i, j;

  //double sum=0, tmp=0;

  for(j=0; j<computation_lp; j++){
    //sum=0;
    for(i=0; i<nbytes; i=i+sizeof(int)){
      *((int *)(buffer+i)) = block_id+i*i;
      //tmp = sqrt(block_id+i*i);
      //sum += tmp;
    }
  }
}

void mark(char* buffer, int nbytes, int block_id){
  //*((long int *)&(buffer[nbytes-8]))=block_id;
  *((int *)(buffer+nbytes-8))=block_id;
}

//put the buffer into ring_buffer
void producer_ring_buffer_put(GV gv, LV lv, char * buffer, int* num_avail_elements){

  ring_buffer *rb;
  rb = (ring_buffer *) gv->producer_rb_p;

  pthread_mutex_lock(rb->lock_ringbuffer);
  while(1) {
    if (rb->num_avail_elements < rb->bufsize) {
      rb->buffer[rb->head] = buffer;
      //gen_check_blk(gv, rb->buffer[rb->head],gv->block_size);
      rb->head = (rb->head + 1) % rb->bufsize;
      *num_avail_elements = ++rb->num_avail_elements;
      pthread_cond_broadcast(rb->empty);
      // pthread_cond_signal(rb->empty);
      pthread_mutex_unlock(rb->lock_ringbuffer);
      return;
    } else {
      lv->wait++;
      pthread_cond_wait(rb->full, rb->lock_ringbuffer);
    }
  }
}


void compute_generator_thread(GV gv,LV lv){
  int block_id=0;
  double t0=0, t1=0, t2=0, t3=0;
  char* buffer=NULL;
  int num_avail_elements=0, full=0;
  // printf("Generator thread %d is running!\n",lv->tid);

  t2 = MPI_Wtime();

  while(1){
    pthread_mutex_lock(&gv->lock_block_id);
    block_id = gv->data_id++;
    pthread_mutex_unlock(&gv->lock_block_id);

    if(block_id>=gv->cpt_total_blks) {
      // generate exit message
      buffer = (char*) malloc(sizeof(char)*gv->compute_data_len);
      check_malloc(buffer);

      ((int *)buffer)[0]= EXIT_BLK_ID;
        // ((int *)buffer)[1]= -1;
        // ((int *)buffer)[2]= -1;

#ifdef DEBUG_PRINT
    printf("Comp_Proc%d: Syn generate the EXIT block_id=%d with total_blks=%d\n",
      gv->rank[0], ((int *)buffer)[0], gv->data_id);
    fflush(stdout);
#endif //DEBUG_PRINT

      producer_ring_buffer_put(gv, lv, buffer, &num_avail_elements);

      if(num_avail_elements == gv->producer_rb_p->bufsize)
        full++;

      break;
    }

    t0 = MPI_Wtime();
    buffer = (char*) malloc(gv->compute_data_len);
    // check_malloc(buffer);
    create_blk(buffer, gv->block_size, block_id, gv->computation_lp);
    // mark(buffer,gv->block_size, block_id);

    //usleep(gv->utime); //sleep for microseconds
    t1 = MPI_Wtime();
    lv->gen_time += t1-t0;

    producer_ring_buffer_put(gv, lv, buffer, &num_avail_elements);

    if(num_avail_elements == gv->producer_rb_p->bufsize)
      full++;

  }
  t3 = MPI_Wtime();

  printf("Comp_Proc%04d: Generator%d, T_total=%.3lf, T_create=%.3lf, full=%d, wait=%d\n",
    gv->rank[0], lv->tid, t3-t2, lv->gen_time, full, lv->wait);
}
