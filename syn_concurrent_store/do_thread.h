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
#include <stdint.h>

#ifdef ADD_PAPI
#include <papi.h>
#include "papi_test.h"
#endif //ADD_PAPI

#define PRODUCER_RINGBUFFER_TOTAL_MEMORY 512*1024*1024L //1G Byte, 512MB

#ifdef CONSUMER_RB_1GB
  #define CONSUMER_RINGBUFFER_TOTAL_MEMORY 1024*1024*1024L
#else
  #define CONSUMER_RINGBUFFER_TOTAL_MEMORY 512*1024*1024L
#endif //CONSUMER_RB_1GB

// #define DEBUG_PRINT
// #define TOTAL_FILE2PRODUCE_1GB 1024*1024*1024L
// #define ADDRESS "/N/dc2/scratch/fuyuan/store/syn_concurrent_store/mbexp%03dvs%03d/cid%03d/cid%03dblk%d.d"

#ifdef COMET
#ifdef WRITE_ONE_FILE
    #define ADDRESS "/oasis/scratch/comet/qoofyk/temp_project/syn/%04dv%04d/cid%04d/cid%04d"
#else
    #define ADDRESS "/oasis/scratch/comet/qoofyk/temp_project/syn/%04dv%04d/cid%04d/cid%04dblk%d"
#endif //WRITE_ONE_FILE
#endif //COMET

#ifdef BRIDGES
#ifdef WRITE_ONE_FILE
    #define ADDRESS "/pylon5/cc4s86p/qoofyk/syn/%04dv%04d/cid%04d/cid%04d"
#else
    #define ADDRESS "/pylon5/cc4s86p/qoofyk/syn/%04dv%04d/cid%04d/cid%04dblk%d"
#endif //WRITE_ONE_FILE
#define OPEN_USLEEP 100
#endif //BRIDGES

#ifdef BIGRED
#ifdef WRITE_ONE_FILE
    #define ADDRESS "/N/dc2/scratch/fuyuan/syn/%04dv%04d/cid%04d/lbm_cid%04d"
#else
    #define ADDRESS "/N/dc2/scratch/fuyuan/syn/%04dv%04d/cid%04d/lbm_cid%04dblk%d"
#endif //WRITE_ONE_FILE
#define OPEN_USLEEP 100
#endif //BIGRED

#define MPI_MSG_TAG 49
#define MIX_MPI_DISK_TAG 50
#define DISK_TAG 51
#define EXIT_MSG_TAG 99

#define NOT_ON_DISK 0
#define ON_DISK 1

#define NOT_CALC 0
#define CALC_DONE 1

#define CACHE 4
#define TRYNUM 10000

#define WRITER_COUNT 2000
#define ANALSIS_COUNT 1000

#define EXIT_BLK_ID -1

typedef struct {
  char** buffer; //array of pointers
  int bufsize; //num of buffer in ringbuffer
  volatile int head;
  volatile int tail;
  volatile int num_avail_elements;
  pthread_mutex_t *lock_ringbuffer;
  pthread_cond_t *full;
  pthread_cond_t *empty;
  pthread_cond_t *new_tail;
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
  int computer_group_size, analysis_process_num, compute_process_num; //Num in each Compute Group, Num of Analysis Node
  // int computeid;
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
  int writer_prb_thousandth;
  int reader_blk_num;
  int sender_blk_num;
  int analysis_writer_blk_num;
  int block_size;
  double total_file;

  int utime;
  int lp;
  int computation_lp;

  int compute_data_len;
  int analysis_data_len;

  //sender
  int sender_all_done;
  int mpi_send_progress_counter;  //currently how many file blocks have been sent
  // int id_get;

  //writer
  int* written_id_array;
  int send_tail;
  int flag_sender_get_finalblk;
  int flag_writer_get_finalblk;

  // receiver_thread
  char* org_recv_buffer;
  int mpi_recv_progress_counter; // how many blocks are received

  //prefetcher thread
  // int prefetch_counter;  //currently how many file blocks have been read
  int recv_tail;
  int * prefetch_id_array;
  int ana_reader_done;
  int ana_writer_done;

  int calc_counter;

  pthread_mutex_t lock_block_id;
  pthread_mutex_t lock_writer_progress;
  pthread_mutex_t lock_writer_done;
  pthread_mutex_t lock_recv;
  // pthread_mutex_t lock_prefetcher_progress;

#ifdef WRITE_ONE_FILE
  FILE *fp;       //compute_proc open 1 bigfile
  FILE **ana_fp;  //analysis_proc open several bigfile
#endif //WRITE_ONE_FILE

  LV  all_lvs;
}* GV;

void* compute_node_do_thread(void* v);
void* analysis_node_do_thread(void* v);

void init_lv(LV lv, int tid, GV gv);
// void mark(char* buffer, int nbytes, int block_id);
double get_cur_time();
void debug_print(int myrank);
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
