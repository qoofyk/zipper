#include "run_analysis.h"

void run_analysis(double * buf_blk, int block_size, int lp){
  // register int i,j,k;
  //register int computeid=0;
  //int check = *(int *)(buffer);

  double * buffer = buf_blk;

  // calculate the block in once
  int cube_size = block_size;


  /********************original analysis by yuankun*/ register double u,n_u0,n_u1;
  register double v,n_v0,n_v1;
  register double mean_u=0;
  register double mean_v=0;
  register double u1,v1;
  int loop=0;
  int base;
  double sum_u[NMOMENT];
  double sum_v[NMOMENT];

  /*
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
  */

  //calc_mean
  // loop unrolling here
  for(base=0;base<cube_size;base=base+4){
    u=*((double *)(buffer+ base));
    v=*((double *)(buffer+base+1));
    u1=*((double *)(buffer+base + 2));
    v1=*((double *)(buffer+base + 3));
    mean_u=mean_u+u+u1;
    mean_v=mean_v+v+v1;
  }
  mean_u=mean_u/(cube_size);
  mean_v=mean_v/(cube_size);
  // mean_u=10;
  // mean_v=10;
  //calc_n_momnents

  for(base=0;base<cube_size;base=base+4){
      for(loop=1;loop<=lp;loop++){
        u=*((double *)(buffer+base));
        v=*((double *)(buffer+base+1));
        u1=*((double *)(buffer+base+2));
        v1=*((double *)(buffer+base+3));

        n_u0=pow(u-mean_u,loop);
        n_u1=pow(u1-mean_u,loop);
        n_v0=pow(v-mean_v,loop);
        n_v1=pow(v1-mean_v,loop);
        sum_u[loop-1]+=n_u0+n_u1;
        sum_v[loop-1]+=n_v0+n_v1;
    }
  }

  for(loop=1;loop<=lp;loop++){
    sum_u[loop-1]=sum_u[loop-1]/(cube_size);
    sum_v[loop-1]=sum_v[loop-1]/(cube_size);
  }
}


