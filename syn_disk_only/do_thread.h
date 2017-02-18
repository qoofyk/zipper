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

#define PRODUCER_RINGBUFFER_TOTAL_MEMORY 1024*1024*1024L //1G Byte
#define CONSUMER_RINGBUFFER_TOTAL_MEMORY 1024*1024*1024L

#define TOTAL_FILE2PRODUCE_1GB 1024*1024*1024L
#define TOTAL_COMPUTE_NODE 32

#define ADDRESS "/N/dc2/scratch/fuyuan/syn/syn%03dvs%03d/exp2/cid%03d/2cid%03d"
// #define nx TOTAL_FILE2PRODUCE_1GB/TOTAL_COMPUTE_NODE
// #define ny TOTAL_FILE2PRODUCE_1GB/TOTAL_COMPUTE_NODE
// #define nz TOTAL_FILE2PRODUCE_1GB/TOTAL_COMPUTE_NODE
#define CACHE 4
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
  int computeid;
  int generator_num;
  int producer_num;  //num of producer
  int prefetcher_num;  //num of prefetcher
  int lp;
  long int id_counter;
  int generator_counter;
  long int progress_counter;  //currently how many file blocks have been generated
  long int prefetch_counter;  //currently how many file blocks have been read
  long int calc_counter;
  long int total_blks;
  int block_size;
  int * written_id_array;
  //int producer_microsecond;
  //int consumer_microsecond;
  ring_buffer* producer_rb_p;
  ring_buffer* consumer_rb_p;
  pthread_mutex_t lock_generator;
  pthread_mutex_t lock_blk_id;
  pthread_mutex_t lock_producer_progress;
  pthread_mutex_t lock_recv;
  pthread_mutex_t lock_prefetcher_progress;
  // MPI_Status status;
  long int send_tail;
  int * mpi_send_block_id_array;
  int mpi_send_progress_counter;
  long int recv_tail;
  int * mpi_recv_block_id_array;
  long int mpi_recv_progress_counter;
  int * recv_block_id_array;
  long int recv_progress_counter;
  LV  all_lvs;
}* GV;

void init_lv(LV lv, int tid, GV gv);

void* node1_do_thread(void* v);
void* node2_do_thread(void* v);

double get_cur_time();

void generator_thread(GV gv,LV lv);
void producer_thread(GV gv,LV lv);
void prefetch_thread(GV gv,LV lv);
void consumer_thread(GV gv,LV lv);
void node1_send(GV gv,LV lv);
void node2_receive(GV gv,LV lv);
void debug_print(int myrank);
//void producer_ring_buffer_put(GV gv,char * buffer);
