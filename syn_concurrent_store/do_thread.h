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
#include <errno.h>

#define PRODUCER_RINGBUFFER_TOTAL_MEMORY 1024*1024*1024L //1G Byte
#define CONSUMER_RINGBUFFER_TOTAL_MEMORY 1024*1024*1024L

// #define DEBUG_PRINT
#define TOTAL_FILE2PRODUCE_1GB 1024*1024*1024L
// #define ADDRESS "/N/dc2/scratch/fuyuan/microbenchmark/cid%02dthread%dblk%d.d"
#define ADDRESS "/N/dc2/scratch/fuyuan/store/syn_concurrent_store/mbexp%03dvs%03d/cid%03d/cid%03dblk%d.d"
// /N/dc2/scratch/fuyuan//concurrent/v1/mbexp001vs001/cid000

#define MPI_MSG_TAG 49
#define MIX_MPI_DISK_TAG 50
#define DISK_TAG 51

// #define BLANK 99
// #define READ_DONE 90

// #define BLANK_CALC_DONE 80
// #define READ_CALC_DONE 70
// #define ANALYSIS_WRITTEN_DONE 60
// #define CALC_ANALYSIS_WRITTEN_BOTH_DONE 50
// #define DELETED 40

#define WRITER_COUNT 2000
#define ANALSIS_COUNT 1000

#define CACHE 4
#define TRYNUM 100

#define NOT_ON_DISK 0
#define ON_DISK 1

#define NOT_CALC 0
#define CALC_DONE 1

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
  double gen_time;
  double read_time;
  double only_fread_time;
  double write_time;
  double only_fwrite_time;
  double calc_time;
  double ring_buffer_put_time;
  double ring_buffer_get_time;
  void  *gv;
}* LV;

typedef struct gv_t {
  int rank[2], size[2], namelen, color;
  char processor_name[128];
  int computer_group_size,analysis_process_num,compute_process_num; //Num in each Compute Group, Num of Analysis Node
  int computeid;
  ring_buffer* producer_rb_p;
  ring_buffer* consumer_rb_p;

  int data_id;
  int compute_generator_num;
  int compute_writer_num;  //num of producer
  int analysis_reader_num;  //num of prefetcher
  int analysis_writer_num;

  int cpt_total_blks;
  int ana_total_blks;
  int writer_thousandth;
  int writer_blk_num;
  int reader_blk_num;
  int sender_blk_num;
  int analysis_writer_blk_num;
  int block_size;
  long total_file;
  int lp;
  // double msleep;

  int compute_data_len;
  int analysis_data_len;

  //sender
  int sender_all_done;
  int mpi_send_progress_counter;  //currently how many file blocks have been sent
  int id_get;

  //writer
  int* written_id_array;
  int send_tail;
  int writer_done;
  int disk_id;
  int writer_quit;

  // receiver_thread
  char * org_recv_buffer;
  int mpi_recv_progress_counter; // how many blocks are received
  //prefetcher thread
  // int prefetch_counter;  //currently how many file blocks have been read
  int recv_tail;
  int * prefetch_id_array;
  int ana_progress;

  int calc_counter;

  int utime;

  pthread_mutex_t lock_block_id;
  pthread_mutex_t lock_id_get;
  pthread_mutex_t lock_writer_progress;
  pthread_mutex_t lock_writer_done;
  pthread_mutex_t lock_writer_quit;
  pthread_mutex_t lock_recv;
  // pthread_mutex_t lock_prefetcher_progress;

  LV  all_lvs;
}* GV;

void* compute_node_do_thread(void* v);
void* analysis_node_do_thread(void* v);

void init_lv(LV lv, int tid, GV gv);
void mark(char* buffer, int nbytes, int block_id);
double get_cur_time();
void debug_print(int myrank);
void create_blk(char* buffer,  int nbytes, int last_gen);
void check_malloc(void * pointer);
void check_MPI_success(GV gv, int errorcode);

void compute_generator_thread(GV gv,LV lv);
void compute_sender_thread(GV gv,LV lv);
void compute_writer_thread(GV gv,LV lv);
void analysis_writer_thread(GV gv,LV lv);
void analysis_receiver_thread(GV gv,LV lv);
void analysis_reader_thread(GV gv,LV lv);
void analysis_consumer_thread(GV gv,LV lv);
void msleep(double milisec);

// void simple_verify(GV gv, LV lv, char* buffer,  int nbytes);
