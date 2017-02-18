/********************************************************
Copyright YUANKUN FU
Brief desc of the file: Header
********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <pthread.h>
#include <math.h>
#include <mpi.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

// #define DEBUG_PRINT
#define MPI_MSG_TAG 49
#define MPI_DISK_TAG 50

#define PRODUCER_RINGBUFFER_TOTAL_MEMORY 512*1024*1024L //1G Byte
#define CONSUMER_RINGBUFFER_TOTAL_MEMORY 512*1024*1024L

// #define TOTAL_COMPUTE_NODE 32

#define ADDRESS "/N/dc2/scratch/fuyuan/LBM/LBM%03dvs%03d/cid%03d/2lbm_cid%03d"

// #define TOTAL_FILE2PRODUCE_1GB 1024/2

// #define nx TOTAL_FILE2PRODUCE_1GB/4
// #define ny TOTAL_FILE2PRODUCE_1GB/4
// #define nz TOTAL_FILE2PRODUCE_1GB/4

// one step one producer one 1GB
#define TOTAL_FILE2PRODUCE_1GB 256

#define nx TOTAL_FILE2PRODUCE_1GB/4 //128
#define ny TOTAL_FILE2PRODUCE_1GB/4 //128
#define nz TOTAL_FILE2PRODUCE_1GB   //512

// #define ny TOTAL_FILE2PRODUCE_1GB
// #define nz TOTAL_FILE2PRODUCE_1GB/4

// #define nx 64
// #define ny 64
// #define nz 64

#define CACHE 4
#define NMOMENT 8

#define TRYNUM 20

typedef struct {
  char** buffer; //array of pointers
  int bufsize; //num of buffer in ringbuffer
  int head;
  int tail;
  int num_avail_elements;
  pthread_mutex_t *lock_ringbuffer;
  pthread_cond_t *full;
  pthread_cond_t *empty;
} ring_buffer;

typedef struct lv_t {
  int tid;
  char* buffer;
  double read_time;
  double only_fread_time;
  double write_time;
  double only_fwrite_time;
  double ring_buffer_put_time;
  double ring_buffer_get_time;
  double calc_time;
  double remove_time;
  //double start_time;
  //double end_time;
  void  *gv;
}* LV;

typedef struct gv_t {
  int rank[2], size[2], namelen, color;
  char processor_name[MPI_MAX_PROCESSOR_NAME];
  int computer_group_size,num_compute_nodes,num_analysis_nodes; //Num in each Compute Group, Num of Analysis Node
  int X,Y,Z;
  int step;
  int step_stop;
  int cubex,cubey,cubez;
  int CI,CJ,CK,originx,originy,originz,gi,gj,gk,computeid;
  //int generator_num;
  int producer_num;  //num of producer
  int prefetcher_num;  //num of prefetcher
  int lp;
  int data_id;

  // int progress_counter;  //currently how many file blocks have been generated
  int prefetch_counter;  //currently how many file blocks have been read
  int calc_counter;

  int cpt_total_blks;
  int ana_total_blks;

  int compute_data_len;
  int analysis_data_len;

  int block_size;
  int * written_id_array;
  //int producer_microsecond;
  //int consumer_microsecond;
  ring_buffer* producer_rb_p;
  ring_buffer* consumer_rb_p;
  // pthread_mutex_t lock_generator;
  pthread_mutex_t lock_blk_id;
  pthread_mutex_t lock_producer_progress;
  pthread_mutex_t lock_recv;
  pthread_mutex_t lock_prefetcher_progress;
  // MPI_Status status;
  int send_tail;
  int * mpi_send_block_id_array;
  int mpi_send_progress_counter;
  int recv_tail;
  int * mpi_recv_block_id_array;
  int mpi_recv_progress_counter;
  int * prefetch_id_array;
  int recv_progress_counter;
  LV  all_lvs;
}* GV;

void init_lv(LV lv, int tid, GV gv);

void* node1_do_thread(void* v);
void* node2_do_thread(void* v);

double get_cur_time();

// void generator_thread(GV gv,LV lv);
void producer_thread(GV gv,LV lv);
void prefetch_thread(GV gv,LV lv);
void consumer_thread(GV gv,LV lv);
void node1_send(GV gv,LV lv);
void node2_receive(GV gv,LV lv);
void producer_ring_buffer_put(GV gv,char * buffer);
