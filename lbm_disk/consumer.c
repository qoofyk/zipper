/********************************************************
Copyright YUANKUN FU
Brief desc of the file: consumer thread
********************************************************/
#include "do_thread.h"

void simple_calc(GV gv, LV lv, char* buffer,  int nbytes){
  register int i,j,k;
  //register int computeid=0;
  //int check = *(int *)(buffer);
  register double u;
  register double v;
  register double mean_u=0;
  register double mean_v=0;
  register int count=0,loop=0;
  int base,block_id;
  //register double y;

  //printf("start calc!\n");
  //printf("before calc data is %d\n",buffer[0]);

  // for(j=0;j<gv->lp;j++){
  //   for(i = 0; i < (nbytes/4-2); i=i+4) {
  //       x = *(int *)(buffer+i);
  //       if(x!=check+i){
  //         printf("Exp2 %d Wrong!\n", i);
  //         printf("x = %x\n", x);
  //         printf("Exp2 consumer_count = %d Wrong!\n", gv->calc_counter);
  //       }
  //       // x = *(int *)(buffer+i);
  //       // if(x!=0x05040302){
  //       //   printf("%x\n", x);
  //       //   printf("Exp2 %d\n", i);
  //       //   printf("Exp2 consumer_count = %d Wrong!\n", gv->calc_counter);
  //       // }
  //       //y = sqrt((double)x);
  //       sqrt((double)x);
  //   }
  // }
  gv->computeid=*(int *)(buffer);
  block_id=*(int *)(buffer+4);
  gv->step=*(int *)(buffer+8);
  gv->CI=*(int *)(buffer+12);
  gv->CJ=*(int *)(buffer+16);
  gv->CK=*(int *)(buffer+20);
  //printf("Node %d consumer %d get computeid%d step%d i%d j%d k%d\n", gv->rank[0], lv->tid, computeid, gv->step, gv->CI, gv->CJ, gv->CK);
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

            u=sqrt(*((double *)(buffer+24+count+CACHE*CACHE*CACHE*base)));
            v=sqrt(*((double *)(buffer+24+count+8+CACHE*CACHE*CACHE*base)));

            mean_u=mean_u+u;
            mean_v=mean_v+v;

            count+=16;
          }
      mean_u=mean_u/(gv->cubex*gv->cubey*gv->cubez);
      mean_v=mean_v/(gv->cubex*gv->cubey*gv->cubez);

      // sqrt(mean_u);
      // sqrt(mean_v);
    }


  // for(loop=0;loop<gv->lp;loop++){
  //   count=0;
  //   for(i=0;i<gv->cubex;i++)
  //     for(j=0;j<gv->cubey;j++)
  //       for(k=0;k<gv->cubez;k++){
  //         gv->gi=gv->originx+i;
  //         gv->gj=gv->originy+j;
  //         gv->gk=gv->originz+k;

  //         // u[gv->gi][gv->gj][gv->gk]=*((double *)(buffer+16+count));
  //         // v[gv->gi][gv->gj][gv->gk]=*((double *)(buffer+16+count+8));

  //         u=*((double *)(buffer+20+count));
  //         v=*((double *)(buffer+20+count+8));

  //         mean_u=mean_u+u;
  //         mean_v=mean_v+v;

  //         count+=16;
  //       }
  //   mean_u=mean_u/(gv->cubex*gv->cubey*gv->cubez);
  //   mean_v=mean_v/(gv->cubex*gv->cubey*gv->cubez);

  //   sqrt(mean_u);
  //   sqrt(mean_v);
  // }
}

// void calc_n_moments(GV gv, LV lv, char* buffer){
//   register int i,j,k;
//   //register int computeid=0;
//   //int check = *(int *)(buffer);
//   register double u,n_u;
//   register double v,n_v;
//   register double mean_u=0;
//   register double mean_v=0;
//   register int count=0,loop=0;
//   int base;

//   // gv->computeid=*(int *)(buffer);
//   // int check_blkid = *(int *)(buffer+sizeof(int));

//   // #ifdef DEBUG_PRINT
//   // printf("check_rank=%d,check_blkid=%d\n", gv->computeid, check_blkid);
//   // fflush(stdout);
//   // #endif //DEBUG_PRINT

//   gv->computeid=*(int *)(buffer);
//   gv->step=*(int *)(buffer+4);
//   gv->CI=*(int *)(buffer+8);
//   gv->CJ=*(int *)(buffer+12);
//   gv->CK=*(int *)(buffer+16);
//   //printf("Node %d consumer %d get computeid%d step%d i%d j%d k%d\n", gv->rank[0], lv->tid, computeid, gv->step, gv->CI, gv->CJ, gv->CK);

//   //calc_mean
//   for(base=0;base<gv->cubex*gv->cubey*gv->cubez/(CACHE*CACHE*CACHE);base++){
//     count=0;
//       for(i=0;i<CACHE;i++)
//         for(j=0;j<CACHE;j++)
//           for(k=0;k<CACHE;k++){
//             // gv->gi=gv->originx+i;
//             // gv->gj=gv->originy+j;
//             // gv->gk=gv->originz+k;

//             // u[gv->gi][gv->gj][gv->gk]=*((double *)(buffer+16+count));
//             // v[gv->gi][gv->gj][gv->gk]=*((double *)(buffer+16+count+8));

//             u=*((double *)(buffer+20+count+CACHE*CACHE*CACHE*base));
//             v=*((double *)(buffer+20+count+8+CACHE*CACHE*CACHE*base));

//             mean_u=mean_u+u;
//             mean_v=mean_v+v;

//             count+=16;
//           }
//   }
//   mean_u=mean_u/(gv->cubex*gv->cubey*gv->cubez);
//   mean_v=mean_v/(gv->cubex*gv->cubey*gv->cubez);

//   //calc_n_moments
//   for(loop=1;loop<=gv->lp;loop++){
//     for(base=0;base<gv->cubex*gv->cubey*gv->cubez/(CACHE*CACHE*CACHE);base++){
//       count=0;
//       for(i=0;i<CACHE;i++)
//         for(j=0;j<CACHE;j++)
//           for(k=0;k<CACHE;k++){
//             // gv->gi=gv->originx+i;
//             // gv->gj=gv->originy+j;
//             // gv->gk=gv->originz+k;

//             // u[gv->gi][gv->gj][gv->gk]=*((double *)(buffer+16+count));
//             // v[gv->gi][gv->gj][gv->gk]=*((double *)(buffer+16+count+8));

//             u=*((double *)(buffer+24+count+CACHE*CACHE*CACHE*base));
//             v=*((double *)(buffer+24+count+8+CACHE*CACHE*CACHE*base));

//             n_u=pow(u-mean_u,loop);
//             n_v=pow(v-mean_v,loop);

//             count+=16;
//           }
//     }
//   }
// }

void calc_n_moments(GV gv, LV lv, char* buffer){
  // register int i,j,k;
  //register int computeid=0;
  //int check = *(int *)(buffer);
  register double u,n_u0,n_u1;
  register double v,n_v0,n_v1;
  register double mean_u=0;
  register double mean_v=0;
  register double u1,v1;
  int loop=0;
  int base,block_id=0;
  double sum_u[NMOMENT];
  double sum_v[NMOMENT];

  gv->computeid=*(int *)(buffer);
  // int check_blkid = *(int *)(buffer+sizeof(int));

  block_id=*(int *)(buffer+4);
  gv->step=*(int *)(buffer+8);
  gv->CI=*(int *)(buffer+12);
  gv->CJ=*(int *)(buffer+16);
  gv->CK=*(int *)(buffer+20);
  #ifdef DEBUG_PRINT
  printf("!!!!! Node %d consumer %d simple_verify computeid = %d, source=%d, block_id=%d, \
gv->calc_counter=%d\n",
          gv->rank[0], lv->tid, gv->computeid, source, block_id,
          gv->calc_counter);
  #endif //DEBUG_PRINT

  //calc_mean
  for(base=0;base<gv->cubex*gv->cubey*gv->cubez*2;base=base+4){
    u=*((double *)(buffer+24+base*8));
    v=*((double *)(buffer+24+base*8+8));
    u1=*((double *)(buffer+24+base*8+16));
    v1=*((double *)(buffer+24+base*8+32));
    mean_u=mean_u+u+u1;
    mean_v=mean_v+v+v1;
  }
  mean_u=mean_u/(gv->cubex*gv->cubey*gv->cubez);
  mean_v=mean_v/(gv->cubex*gv->cubey*gv->cubez);
  // mean_u=10;
  // mean_v=10;
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
//   //int blk_id;
//   //int computeid=0;

//   //computeid=*(int *)(pointer);

//   sprintf(file_name,ADDRESS,gv->computeid,gv->step,gv->CI,gv->CJ,gv->CK);
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

  // printf("Analysis Process %d consumer thread %d is running!\n",gv->rank[0], lv->tid);

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
      t0 = get_cur_time();
      // simple_calc(gv, lv, pointer,gv->block_size);
      calc_n_moments(gv, lv, pointer);
      t1 = get_cur_time();
      lv->calc_time += t1 - t0;

      // t4 = get_cur_time();
      // read_mark_remove(gv, pointer, gv->block_size);
      // t5 = get_cur_time();
      // lv->remove_time += t5 - t4;

      gv->calc_counter++;

      if(gv->calc_counter%10000==0)
        printf("Analysis Process %d Consumer %d calc_counter %d\n", gv->rank[0], lv->tid, gv->calc_counter);

      free(pointer);
    }

    if(gv->calc_counter >= gv->ana_total_blks) break;
  }
  t3 = get_cur_time();

  printf("Analysis Process %d consumer calc_time =%f, wait_time =%f, total_consumer_time =%f\n",
    gv->rank[0], lv->calc_time, wait_time, t3-t2);
  // printf("Node %d consumer %d remove_time is %f\n", gv->rank[0], lv->tid, lv->remove_time);
}
