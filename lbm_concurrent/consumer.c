/********************************************************
Copyright YUANKUN FU
Brief desc of the file: consumer thread
********************************************************/
#include "concurrent.h"
void simple_verify(GV gv, LV lv, char* buffer){
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

  #ifdef DEBUG_PRINT
  printf("check_rank=%d,check_blkid=%d\n", gv->computeid, check_blkid);
  fflush(stdout);
  #endif //DEBUG_PRINT

  gv->step=*(int *)(buffer+8);
  gv->CI=*(int *)(buffer+12);
  gv->CJ=*(int *)(buffer+16);
  gv->CK=*(int *)(buffer+20);
  //printf("Node %d consumer %d get computeid%d step%d i%d j%d k%d\n", gv->rank[0], lv->tid, computeid, gv->step, gv->CI, gv->CJ, gv->CK);
  for(base=0;base<gv->cubex*gv->cubey*gv->cubez*2/(CACHE*CACHE*CACHE);base++)
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

            u=sqrt(*((double *)(buffer+24+count+CACHE*CACHE*CACHE*base)));
            v=sqrt(*((double *)(buffer+24+count+8+CACHE*CACHE*CACHE*base)));
            // u=sqrt(*((double *)(buffer+24+count+i*j*k*base)));
            // v=sqrt(*((double *)(buffer+24+count+8+i*j*k*base)));

            mean_u=mean_u+u;
            mean_v=mean_v+v;

            count+=16;
          }
      mean_u=mean_u/(gv->cubex*gv->cubey*gv->cubez);
      mean_v=mean_v/(gv->cubex*gv->cubey*gv->cubez);

      // sqrt(mean_u);
      // sqrt(mean_v);
    }
    // printf("Analysis Process %d base=%d, count=%d\n",
    //   gv->rank[0], base, count);
    // fflush(stdout);
}

void calc_n_moments(GV gv, LV lv, char* buffer){
  // register int i,j,k;
  //register int computeid=0;
  //int check = *(int *)(buffer);
  register double u,n_u0,n_u1;
  register double v,n_v0,n_v1;
  register double mean_u=0.0;
  register double mean_v=0.0;
  register double u1,v1;
  int loop=0;
  int base,block_id=0;
  double sum_u[NMOMENT];
  double sum_v[NMOMENT];

  gv->computeid=*(int *)(buffer);
  // int check_blkid = *(int *)(buffer+sizeof(int));

  #ifdef DEBUG_PRINT
  printf("check_rank=%d,check_blkid=%d\n", gv->computeid, check_blkid);
  fflush(stdout);
  #endif //DEBUG_PRINT

  block_id=*(int *)(buffer+4);
  gv->step=*(int *)(buffer+8);
  gv->CI=*(int *)(buffer+12);
  gv->CJ=*(int *)(buffer+16);
  gv->CK=*(int *)(buffer+20);
  //printf("Node %d consumer %d get computeid%d step%d i%d j%d k%d\n", gv->rank[0], lv->tid, computeid, gv->step, gv->CI, gv->CJ, gv->CK);

  //calc_mean
  for(base=0;base<gv->cubex*gv->cubey*gv->cubez*2;base=base+4){
    u=*((double *)(buffer+24+base*8));
    v=*((double *)(buffer+24+base*8+8));
    u1=*((double *)(buffer+24+base*8+16));
    v1=*((double *)(buffer+24+base*8+32));

    // if(u>0 || v>0 || u1>0 || v1>0){
    //   printf("Analysis Process %d u=%f, v=%f, u1=%f, v1=%f, mean_u=%f, mean_v=%f\n",
    //     gv->rank[0], u, v, u1, v1, mean_u, mean_v);
    //   fflush(stdout);
    // }

    mean_u=mean_u+u+u1;
    mean_v=mean_v+v+v1;
  }
  mean_u=mean_u/(gv->cubex*gv->cubey*gv->cubez);
  mean_v=mean_v/(gv->cubex*gv->cubey*gv->cubez);

  // printf("Analysis Process %d blockid=%d mean_u=%f, mean_v=%f\n",
  //     gv->rank[0], block_id, mean_u, mean_v);
  // fflush(stdout);

  //calc_n_momnents

  for(base=0;base<gv->cubex*gv->cubey*gv->cubez*2;base=base+4){
      for(loop=1;loop<=gv->lp;loop++){
        u=*((double *)(buffer+24+base*8));
        v=*((double *)(buffer+24+base*8+8));
        u1=*((double *)(buffer+24+base*8+16));
        v1=*((double *)(buffer+24+base*8+32));

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


// void read_mark_remove(GV gv, char* pointer,  int nbytes){
//   char file_name[64];
//   //FILE *fp;
//   int blk_id;
//   //int computeid=0;

//   //computeid=*(int *)(pointer);
//   blk_id = *((int *)(pointer+nbytes-8));
//   sprintf(file_name,ADDRESS,gv->num_compute_nodes, gv->num_analysis_nodes,gv->computeid,gv->computeid,blk_id);
//   // sprintf(file_name,"/var/tmp/exp2_file_blk%d.data",blk_id);
//   // if(remove(file_name)== -1){
//   //   printf("Consumer cannot remove file %d!\n",blk_id);
//   // }
//   remove(file_name);
//   //printf("Consumer remove file %d Done!\n",blk_id);
// }


//Consumer
char* ring_buffer_get(GV gv,LV lv){
  char* pointer;

  ring_buffer *rb;

  rb = (ring_buffer *) gv->consumer_rb_p;

  pthread_mutex_lock(rb->lock_ringbuffer);
  while(1) {
    if (rb->num_avail_elements > 0) {
      pointer = rb->buffer[rb->tail];
      rb->tail = (rb->tail + 1) % rb->bufsize;
      rb->num_avail_elements--;
      pthread_cond_signal(rb->full);
      pthread_mutex_unlock(rb->lock_ringbuffer);
      return pointer;
    } else {
      pthread_cond_wait(rb->empty, rb->lock_ringbuffer);
    }
  }
}

void consumer_thread(GV gv,LV lv){
  double t0=0, t1=0,t2=0,t3=0,t6=0,t7=0;
  char* pointer=NULL;
  double wait_time=0;
  int* temp_int_pointer;
  int source=0,block_id=0;
  // double nt1=0,nt2=0;
  // printf("Node %d consumer thread %d is running!\n",gv->rank[0], lv->tid);

  t2 = get_cur_time();
  while(1) {

    t6 = get_cur_time();
    pointer = ring_buffer_get(gv,lv);
    t7 = get_cur_time();
    wait_time += t7-t6;

    if (pointer == NULL) {
      pthread_exit(NULL);
    }
    else{
      temp_int_pointer=(int*)pointer;
      source = temp_int_pointer[0];
      block_id = temp_int_pointer[1];

      t0 = get_cur_time();
      // simple_verify(gv, lv, pointer);
      calc_n_moments(gv, lv, pointer);
      t1 = get_cur_time();
      lv->calc_time += t1 - t0;
      // printf("Analysis Process%d current Block %d calc_time=%f\n",
      //   gv->rank[0], block_id, t1-t0);
      // fflush(stdout);

      // t4 = get_cur_time();
      // read_mark_remove(gv, pointer, gv->block_size);
      // t5 = get_cur_time();
      // lv->remove_time += t5 - t4;

      gv->calc_counter++;

      if(gv->calc_counter%10000==0)
        printf("Ana Process %d Consumer %d calc_counter %d\n", gv->rank[0], lv->tid, gv->calc_counter);

      free(pointer);
    }

    if(gv->calc_counter >= gv->ana_total_blks) break;
  }
  t3 = get_cur_time();

  printf("Analysis Process %d ###LBM No-keep### total_consumer_time=%f,calc_time=%f, wait_time=%f, calc_counter=%d\n",
   gv->rank[0], t3-t2, lv->calc_time, wait_time, gv->calc_counter);
  fflush(stdout);
}
