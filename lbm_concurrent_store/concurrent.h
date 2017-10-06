/********************************************************
Copyright YUANKUN FU
Brief desc of the file: Header
********************************************************/
#ifndef CONCURRENT_H
#define CONCURRENT_H

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

#define PRODUCER_RINGBUFFER_TOTAL_MEMORY 512*1024*1024L //1G Byte
#ifdef CONSUMER_RB_4GB
  #define CONSUMER_RINGBUFFER_TOTAL_MEMORY 4*1024*1024*1024L
#else
  #define CONSUMER_RINGBUFFER_TOTAL_MEMORY 512*1024*1024L
#endif //CONSUMER_RB_4GB

// #define DEBUG_PRINT
// #define TOTAL_FILE2PRODUCE_1GB 1024*1024*1024L

#define OPEN_USLEEP 500

#define MPI_MSG_TAG 49
#define MIX_MPI_DISK_TAG 50
#define DISK_TAG 51
#define EXIT_MSG_TAG 99

#define NOT_ON_DISK 0
#define ON_DISK 1

#define NOT_CALC 0
#define CALC_DONE 1

#define CACHE 4

/*
  * this is moved to main function arguments
#define TOTAL_FILE2PRODUCE_1GB 256

#define nx TOTAL_FILE2PRODUCE_1GB/4
#define ny TOTAL_FILE2PRODUCE_1GB/4
#define nz TOTAL_FILE2PRODUCE_1GB
*/

#define NMOMENT 8
#define TRYNUM 200

#define WRITER_COUNT 5000
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

  int   wait;
  void  *gv;
}* LV;

typedef struct gv_t {
  char* filepath;
  //LBM parameter
  int X,Y,Z;
  int step;
  int step_stop;
  int cubex,cubey,cubez;
  int CI,CJ,CK,originx,originy,originz,gi,gj,gk,computeid;
  int n_moments;

  int rank[2], size[2], namelen, color;
  char processor_name[128];
  int computer_group_size, analysis_process_num, compute_process_num; //Num in each Compute Group, Num of Analysis Node

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
  // double msleep;

  int compute_data_len;
  int analysis_data_len;

  //sender
  // int sender_all_done;
  // int mpi_send_progress_counter;  //currently how many file blocks have been sent

  //writer
  int *written_id_array;
  int send_tail;
  volatile int flag_sender_get_finalblk;
  volatile int flag_writer_get_finalblk;
  volatile int writer_exit;
  // pthread_mutex_t lock_writer_exit;
  // pthread_cond_t writer_exit;

  // receiver_thread
  char *org_recv_buffer;
  // int mpi_recv_progress_counter; // how many blocks are received

  //prefetcher thread
  // int prefetch_counter;  //currently how many file blocks have been read
  int recv_head;
  int recv_tail;
  int recv_avail;
  int *prefetch_id_array;

  volatile int recv_exit;
  volatile int reader_exit;
  // pthread_mutex_t lock_reader_exit;
  // pthread_cond_t  reader_exit;
  volatile int ana_writer_exit;

  int calc_counter;

  pthread_mutex_t lock_block_id;
  pthread_mutex_t lock_disk_id_arr;
  pthread_mutex_t lock_writer_done;
  pthread_mutex_t lock_recv_disk_id_arr;
  // pthread_mutex_t lock_prefetcher_progress;

#ifdef WRITE_ONE_FILE
  FILE *fp;
  FILE **ana_read_fp;  //analysis_proc reader open bigfile
  FILE **ana_write_fp;  //analysis_proc reader open bigfile
#endif //WRITE_ONE_FILE

  LV  all_lvs;
}* GV;

void* compute_node_do_thread(void* v);
void* analysis_node_do_thread(void* v);

void init_lv(LV lv, int tid, GV gv);
// void mark(char* buffer, int nbytes, int block_id);
double get_cur_time();
void debug_print(int myrank);
// void create_blk(char* buffer, int nbytes, int last_gen);
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

// void simple_verify(GV gv, LV lv, char* buffer);
void producer_ring_buffer_put(GV gv, char* buffer, int* num_avail_elements);

#endif
