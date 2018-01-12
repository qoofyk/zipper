/********************************************************
Copyright YUANKUN FU
Brief desc of the file: do thread
********************************************************/
#include "concurrent.h"

void init_lv(LV lv, int tid, GV gv) {
  lv->tid   = tid;
  lv->gv    = gv;

  lv->read_time   = 0.0;
  lv->write_time  = 0.0;
  lv->only_fwrite_time = 0.0;
  lv->only_fread_time = 0.0;
  lv->calc_time = 0.0;
  lv->ring_buffer_put_time = 0.0;
  lv->ring_buffer_get_time = 0.0;
  lv->wait = 0;
  //printf("init mutex done!\n");
}

void* compute_node_do_thread(void* v) {
  GV  gv;
  LV  lv;
  int tid;

  lv = (LV) v;
  gv = (GV) lv->gv;
  tid = lv->tid;

  // printf("Compute %d Thread %d starts running...\n", gv->rank[0], tid);
  // fflush(stdout);

  if(tid<=(gv->compute_writer_num-1)){
      //writer_thread
      compute_writer_thread(gv,lv);
  }
  else{
      //mpi_send_thread
      compute_sender_thread(gv,lv);
  }

  return NULL;
}

void* analysis_node_do_thread(void* v) {
  GV  gv;
  LV  lv;
  int tid;

  lv = (LV) v;
  gv = (GV) lv->gv;
  tid = lv->tid;
  // printf("Ana_Proc%d: Thread %d starts running...\n", gv->rank[0], tid);
  // fflush(stdout);

  if(tid<=(gv->analysis_reader_num-1)) {
     //prefetching thread
      analysis_reader_thread(gv,lv);
    }
  else if(tid >= gv->analysis_reader_num && tid<=(gv->analysis_reader_num + gv->analysis_writer_num - 1)){
      //writer_thread
      analysis_writer_thread(gv,lv);
  }
  else if(tid == gv->analysis_reader_num + gv->analysis_writer_num){
      //consumer thread
      analysis_consumer_thread(gv,lv);
  }
  else{
      //mpi_receive
      analysis_receiver_thread(gv,lv);
  }

  return NULL;
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

void msleep(double milisec){
    struct timespec req={0};
    time_t sec=(int)(milisec/1000);
    milisec=milisec-(sec*1000);
    req.tv_sec=sec;
    req.tv_nsec=(long)(milisec*1000*1000);
    while( (nanosleep(&req,&req)==-1) && (errno == EINTR))
         continue;
    // return 1;
}

void check_malloc(void * pointer){
  if(pointer == NULL) {
    perror("Malloc error!\n");
    fprintf (stderr, "at %s, line %d.\n", __FILE__, __LINE__);
    exit(1);
  }

  //check is aligned on 8 byte count
  if((uintptr_t)pointer%8 != 0){
    printf("Pointer %p is NOT ALIGNED!!!!\n", pointer);
    fflush(stdout);
  }
}

void check_MPI_success(GV gv, int errorcode){
  if(errorcode!= MPI_SUCCESS){
    perror("MPI_SEND not MPI_SUCCESS!\n");
    fprintf (stderr, "Node %d at  %s, line %d.\n", gv->rank[0], __FILE__, __LINE__);
    exit(1);
  }
}
