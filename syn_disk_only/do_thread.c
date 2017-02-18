/********************************************************
Copyright YUANKUN FU
Brief desc of the file: do thread
********************************************************/
#include "do_thread.h"

void init_lv(LV lv, int tid, GV gv) {
  lv->tid   = tid;
  lv->gv    = gv;

  lv->read_time = 0;
  lv->write_time = 0;
  lv->only_fwrite_time = 0;
  lv->only_fread_time = 0;
  lv->ring_buffer_put_time = 0;
  lv->ring_buffer_get_time = 0;
  lv->calc_time = 0;
  lv->remove_time = 0;
  //printf("init mutex done!\n");
}

void* node1_do_thread(void* v) {
  GV  gv;
  LV  lv;
  int tid;

  lv = (LV) v;
  gv = (GV) lv->gv;
  tid = lv->tid;

  //printf("Thread %d starts running...\n", tid);

  if(tid <= (gv->generator_num - 1)) {
      //generator
      generator_thread(gv,lv);
  }
  else if(tid >= gv->generator_num && tid<=(gv->generator_num + gv->producer_num-1)){
      //producer
      producer_thread(gv,lv);
  }
  else{
      //mpi_send_thread
      node1_send(gv,lv);
  }

  return NULL;
}

void* node2_do_thread(void* v) {
  GV  gv;
  LV  lv;
  int tid;

  lv = (LV) v;
  gv = (GV) lv->gv;
  tid = lv->tid;
  if(tid<=(gv->prefetcher_num-1)) {
     //prefetching thread
      prefetch_thread(gv,lv);
    }
  else if(tid == gv->prefetcher_num){
      //consumer thread
      consumer_thread(gv,lv);
  }
  else{
      //mpi_receive
      node2_receive(gv,lv);
  }
  return NULL;
}

void node1_send(GV gv,LV lv){
  long int i,send_counter;
  double t0 = 0, t1 = 0, t2 = 0, t3 = 0, t4=0,t5=0,wait_lock=0;
  double send_time=0;
  // printf("Node %d Send thread %d start!\n", gv->rank[0], lv->tid);
  //int errorcode;

  t0 = get_cur_time();
  while(1){
    send_counter = 0;

    t4 = get_cur_time();
    pthread_mutex_lock(&gv->lock_producer_progress);
    if(gv->send_tail>0){
      for(i=0;i<gv->send_tail;i++){
        gv->mpi_send_block_id_array[i+1]=gv->written_id_array[i];
        send_counter++;
      }
      gv->mpi_send_block_id_array[0]=send_counter;
      gv->send_tail = 0;
    }
    pthread_mutex_unlock(&gv->lock_producer_progress);
    t5 = get_cur_time();
    wait_lock += t5-t4;

    if(send_counter > 0){
      //printf("Node 1 send a message send_counter=%d\n", send_counter);
      t2 = get_cur_time();
      // if(gv->rank[0]<2){
      //   errorcode = MPI_Send(gv->mpi_send_block_id_array, send_counter+1,MPI_INT,4,0,MPI_COMM_WORLD);    //tag?
      //   if(errorcode!= MPI_SUCCESS){
      //     printf("Node %d sender %d Error MPI Send!\n", gv->rank[0], lv->tid);
      //     exit(1);
      //   }
      // }
      // else if (gv->rank[0]>=2 && gv->rank[0]<4){
      //   errorcode = MPI_Send(gv->mpi_send_block_id_array, send_counter+1,MPI_INT,5,0,MPI_COMM_WORLD);
      //   if(errorcode!= MPI_SUCCESS){
      //     printf("Node %d sender %d Error MPI Send!\n", gv->rank[0], lv->tid);
      //     exit(1);
      //   }
      // }
      MPI_Send(gv->mpi_send_block_id_array, send_counter+1,MPI_INT,gv->rank[0]/gv->computer_group_size+gv->num_analysis_nodes*gv->computer_group_size,0,MPI_COMM_WORLD);

      t3 = get_cur_time();

      send_time += t3-t2;
      gv->mpi_send_progress_counter += send_counter;
      if(gv->mpi_send_progress_counter>=gv->total_blks) break;
    }
  }
  t1 = get_cur_time();
  printf("Node %d Send thread %d total time is %f, send_time = %f, wait_lock = %f\n",
    gv->rank[0], lv->tid, t1-t0, send_time, wait_lock);
}

void node2_receive(GV gv,LV lv){
  int j,recv_counter=0;
  double t0=0, t1=0,t2=0,t3=0,t4=0,t5=0, wait_lock=0;
  double receive_time=0;
  MPI_Status status;
  int messageind=0;
  //int errorcode;

  // printf("Node %d Receiveing thread %d Start receive!\n",gv->rank[0], lv->tid);

  t0 = get_cur_time();
  while(1){
    ///statistics!!!!
    t2 = get_cur_time();
    // if(gv->rank[0]==4){
    //     errorcode = MPI_Recv(gv->mpi_recv_block_id_array, 5*gv->total_blks*2+1,MPI_INT,MPI_ANY_SOURCE,0,MPI_COMM_WORLD, &status);
    //     if(errorcode!= MPI_SUCCESS){
    //       printf("Node %d sender %d Error MPI receive!\n", gv->rank[0], lv->tid);
    //       exit(1);
    //     }
    // }
    // else if(gv->rank[0]==5){
    //     errorcode = MPI_Recv(gv->mpi_recv_block_id_array, 5*gv->total_blks*2+1,MPI_INT,MPI_ANY_SOURCE,0,MPI_COMM_WORLD, &status);
    //     if(errorcode!= MPI_SUCCESS){
    //       printf("Node %d sender %d Error MPI receive!\n", gv->rank[0], lv->tid);
    //       exit(1);
    //     }
    // }
    MPI_Recv(gv->mpi_recv_block_id_array, gv->total_blks/gv->computer_group_size+1,MPI_INT,MPI_ANY_SOURCE,0,MPI_COMM_WORLD, &status);

    t3 = get_cur_time();
    receive_time += t3-t2;

    recv_counter=gv->mpi_recv_block_id_array[0];
    //printf("Node %d receive source %d messageind %d recv_counter = %d\n", gv->rank[0], status.MPI_SOURCE, messageind, recv_counter);
    messageind++;

    // statistic !!!!!!!!
    t4 = get_cur_time();
    pthread_mutex_lock(&gv->lock_recv);

    //number of items, in terms of items, items havs how many bytes,*
    for(j=0;j<recv_counter;j++){
      gv->recv_block_id_array[gv->recv_tail]=status.MPI_SOURCE;
      gv->recv_block_id_array[gv->recv_tail+1]=gv->mpi_recv_block_id_array[j+1];
      // gv->recv_block_id_array[gv->recv_tail+2]=gv->mpi_recv_block_id_array[j+2];
      // gv->recv_block_id_array[gv->recv_tail+3]=gv->mpi_recv_block_id_array[j+3];
      // gv->recv_block_id_array[gv->recv_tail+4]=gv->mpi_recv_block_id_array[j+4];
      gv->recv_tail+=2;
      //printf("After copy the mpi recv id array, Node %d Receive thread recv_tail = %d\n", gv->rank[0], gv->recv_tail);
    }
    gv->recv_progress_counter += recv_counter;
    pthread_mutex_unlock(&gv->lock_recv);
    t5 = get_cur_time();
    wait_lock += t5-t4;

    //printf("Node 2 RECEIVE thread %d using recv_progress_counter = %d\n", lv->tid, gv->recv_progress_counter);

    if(gv->recv_progress_counter>=gv->total_blks) {
      //printf("Node 2 receive break!\n");
      break;
    }
  }
  t1 = get_cur_time();
  printf("Node %d Receive thread %d total time is %f, receive_time = %f, wait_lock = %f, messageind= %d\n",
    gv->rank[0], lv->tid, t1-t0, receive_time, wait_lock, messageind);
}

double get_cur_time() {
  struct timeval   tv;
  struct timezone  tz;
  double cur_time;

  gettimeofday(&tv, &tz);
  cur_time = tv.tv_sec + tv.tv_usec / 1000000.0;
  //printf("%f\n",cur_time);

  return cur_time;
}

void debug_print(int myrank) {
  char hostname[128];
  int pid;

  gethostname(hostname, strlen(hostname));
  pid = getpid();
  printf("Greeting from process %d(%s:%d)!\n", myrank, hostname, pid);
  fflush(stdout);
  sleep(10);
}
