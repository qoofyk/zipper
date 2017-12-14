#include "run_analysis.h"

#define EACH_FLUID_NUM_DOUBLE (2)

void run_analysis(double * buf_blk, int block_size, int lp, double *sum_vx, double *sum_vy){
  char * buffer = (char *)buf_blk;
//void calc_n_moments(GV gv, char* buffer, double* sum_vx, double* sum_vy){

  //int check = *(int *)(buffer);
  double vx, vy;
  double vx_prime, vy_prime;
  double mean_vx=0, mean_vy=0;

  int i, j;
  int num_points = block_size;
  int num_double = EACH_FLUID_NUM_DOUBLE*num_points;


  //initialize
  for(i=1;i<=lp;i++){
    sum_vx[i-1]=0;
    sum_vy[i-1]=0;
    // sum_vz[i-1]=0;
  }

  //calc_mean
  for(i=0; i<num_double; i+=EACH_FLUID_NUM_DOUBLE){
    vx = ((double *)buffer)[i];
    vy = ((double *)buffer)[i+1];
    // vz = ((double *)buffer)[i+2];
    //rho = ((double *)buffer)[i+3];

    mean_vx += vx;
    mean_vy += vy;
    // mean_vz += vz;

  }
  mean_vx=mean_vx/num_points;
  mean_vy=mean_vy/num_points;
  // mean_vz=mean_vz/num_points;

  //calc_n_momnents

  for(i=0; i<num_double; i+=EACH_FLUID_NUM_DOUBLE){

    vx = ((double *)buffer)[i];
    vy = ((double *)buffer)[i+1];
    // vz = ((double *)buffer)[i+2];
    // rho = ((double *)buffer)[i+3];

    //compute Flutuation field (velocity - velocity_mean)
    vx_prime = vx - mean_vx;
    vy_prime = vy - mean_vy;
    // uz = rho * (vz - mean_vz);

    //power ^2 -- variance; power ^3 -- skewness; power ^4 -- flatness
    for(j=1;j<=lp;j++){
        sum_vx[j-1] += pow(vx_prime, j);
        sum_vy[j-1] += pow(vy_prime, j);
        // sum_vz[j-1] += pow(vz_prime, j);
    }
  }

  for(i=1;i<=lp;i++){
    sum_vx[i-1]=sum_vx[i-1]/num_points;
    sum_vy[i-1]=sum_vy[i-1]/num_points;
    // sum_uz[i-1]=sum_vz[i-1]/num_points;
  }
}

/*void run_analysis(double * buf_blk, int block_size, int lp){*/
  /*// register int i,j,k;*/
  /*//register int computeid=0;*/
  /*//int check = *(int *)(buffer);*/

  /*double * buffer = buf_blk;*/

  /*// calculate the block in once*/
  /*int cube_size = block_size;*/


  /*[>*******************original analysis by yuankun<] register double u,n_u0,n_u1;*/
  /*register double v,n_v0,n_v1;*/
  /*register double mean_u=0;*/
  /*register double mean_v=0;*/
  /*register double u1,v1;*/
  /*int loop=0;*/
  /*int base;*/
  /*double sum_u[NMOMENT];*/
  /*double sum_v[NMOMENT];*/

  /*/**/
  /*gv->computeid=*(int *)(buffer);*/
  /*// int check_blkid = *(int *)(buffer+sizeof(int));*/

  /*gv->step=*(int *)(buffer+12);*/
  /*gv->CI=*(int *)(buffer+16);*/
  /*gv->CJ=*(int *)(buffer+20);*/
  /*gv->CK=*(int *)(buffer+24);*/
  /*#ifdef DEBUG_PRINT*/
  /*printf("!!!!! Node %d consumer %d simple_verify computeid = %d, check_blkid=%d, source=%d, block_id=%d, \*/
/*step=%d, i=%d, j=%d, k=%d, gv->calc_counter=%d, *consumer_state_p=%d\n",*/
          /*gv->rank[0], lv->tid, gv->computeid, check_blkid, source, block_id,*/
          /*gv->step, gv->CI, gv->CJ, gv->CK, gv->calc_counter,*consumer_state_p);*/
  /*#endif //DEBUG_PRINT*/
  /**/

  /*//calc_mean*/
  /*// loop unrolling here*/
  /*for(base=0;base<cube_size*2;base=base+2){*/
    /*u=*((double *)(buffer+ base));*/
    /*v=*((double *)(buffer+base+1));*/
    /*[>u1=*((double *)(buffer+base + 2));<]*/
    /*[>v1=*((double *)(buffer+base + 3));<]*/
    /*mean_u=mean_u+u;*/
    /*mean_v=mean_v+v;*/
  /*}*/
  /*mean_u=mean_u/(cube_size);*/
  /*mean_v=mean_v/(cube_size);*/
  /*// mean_u=10;*/
  /*// mean_v=10;*/
  /*//calc_n_momnents*/
  /*//*/
 

/*for(i=0; i<num_double; i+=EACH_FLUID_NUM_DOUBLE){*/

    /*vx = ((double *)buffer)[i];*/
    /*vy = ((double *)buffer)[i+1];*/
    /*// vz = ((double *)buffer)[i+2];*/
    /*// rho = ((double *)buffer)[i+3];*/

    /*//compute Flutuation field (velocity - velocity_mean)*/
    /*vx_prime = vx - mean_vx;*/
    /*vy_prime = vy - mean_vy;*/
    /*// uz = rho * (vz - mean_vz);*/

    /*//power ^2 -- variance; power ^3 -- skewness; power ^4 -- flatness*/
    /*for(j=1;j<=gv->n_moments;j++){*/
        /*sum_vx[j-1] += pow(vx_prime, j);*/
        /*sum_vy[j-1] += pow(vy_prime, j);*/
        /*// sum_vz[j-1] += pow(vz_prime, j);*/
    /*}*/
  /*}*/

  /*for(i=1;i<=gv->n_moments;i++){*/
    /*sum_vx[i-1]=sum_vx[i-1]/num_points;*/
    /*sum_vy[i-1]=sum_vy[i-1]/num_points;*/
    /*// sum_uz[i-1]=sum_vz[i-1]/num_points;*/
/*}*/


  /*for(base=0;base<cube_size*2;base=base+2){*/
      /*for(loop=1;loop<=lp;loop++){*/
        /*u=*((double *)(buffer+base));*/
        /*v=*((double *)(buffer+base+1));*/
        /*u1=*((double *)(buffer+base+2));*/
        /*v1=*((double *)(buffer+base+3));*/

        /*n_u0=pow(u-mean_u,loop);*/
        /*n_u1=pow(u1-mean_u,loop);*/
        /*n_v0=pow(v-mean_v,loop);*/
        /*n_v1=pow(v1-mean_v,loop);*/
        /*sum_u[loop-1]+=n_u0+n_u1;*/
        /*sum_v[loop-1]+=n_v0+n_v1;*/
    /*}*/
  /*}*/

  /*for(loop=1;loop<=lp;loop++){*/
    /*sum_u[loop-1]=sum_u[loop-1]/(cube_size);*/
    /*sum_v[loop-1]=sum_v[loop-1]/(cube_size);*/
  /*}*/
/*}*/


