#include "concurrent.h"

//put the buffer into ring_buffer
void ring_buffer_put(GV gv,LV lv,char * buffer){

  ring_buffer *rb;

  rb = (ring_buffer *) gv->consumer_rb_p;

  pthread_mutex_lock(rb->lock_ringbuffer);
  while(1) {
    if (rb->num_avail_elements < rb->bufsize) {
      rb->buffer[rb->head] = buffer;
      rb->head = (rb->head + 1) % rb->bufsize;
      rb->num_avail_elements++;
      pthread_cond_signal(rb->empty);
      pthread_mutex_unlock(rb->lock_ringbuffer);
      return;
    } else {
      pthread_cond_wait(rb->full, rb->lock_ringbuffer);
    }
  }
}

void read_blk(GV gv, LV lv,int last_gen_rank, int last_file_position, char* buffer, FILE *fp){
  double t0=0, t1=0;
  int error=-1;
  int i=0;

  while((error==-1) && (i<TRYNUM)){
    error=fseek(fp, last_file_position*gv->block_size, SEEK_SET);
      if(error==-1){
        if(i==TRYNUM-1){
          perror("Analysis Process fseek error:");
          printf("Warning: Analysis Process %d fseek error last_file_position=%d,*fp=%x\n",
            gv->rank[0], last_file_position,*fp);
          fflush(stdout);
        }
        i++;
        usleep(20000);
      }
  }


  t0 = get_cur_time();
  fread(buffer, gv->block_size, 1, fp);
  if(ferror(fp)==-1){
    perror("fread error:");
    fflush(stdout);
  }
  fflush(fp);
  t1 = get_cur_time();
  lv->only_fread_time += t1 - t0;
}

void reader_thread(GV gv,LV lv) {

  int last_gen_rank=0, block_id;
  int last_file_position = 0, read_file_cnt=0;
  double t0=0,t1=0,t2=0,t3=0;
  char* new_buffer=NULL;
  char flag=0;

  char file_name[128];
  FILE **fp=NULL;
  char* open;
  int i=0,j=0;

  fp = (FILE**)malloc(sizeof(FILE*)*gv->computer_group_size);
  open = (char*)malloc(sizeof(char)*gv->computer_group_size);
  for(j=0;j<gv->computer_group_size;j++){
    fp[j] = NULL;
    open[j] = 0;
  }
  // printf("Analysis Node %d Reader thread %d is running!\n",gv->rank[0], lv->tid);

  t2 = get_cur_time();

  // new_buffer = (char*) malloc(sizeof(char)*gv->block_size);

  if(gv->reader_blk_num>0){
    while(1){

      pthread_mutex_lock(&gv->lock_recv);
      if(gv->ana_progress>=gv->ana_total_blks){
        pthread_mutex_unlock(&gv->lock_recv);
        break;
      }

      if(gv->recv_tail>0){
        flag = 1;
        //printf("Prefetcher %d read recv_tail = %d\n", lv->tid, gv->recv_tail);
        last_gen_rank = gv->prefetch_id_array[gv->recv_tail-2]; // get a snapshot of which block has been generated
        last_file_position = gv->prefetch_id_array[gv->recv_tail-1];

        // if(gv->prefetch_counter%1000==0)
        //   printf("!!!!!!!!!Node %d Prefetcher %d get lock, prefetch_counter=%ld, last_gen_rank = %d, step = %d, CI=%d, CJ=%d, CK=%d\n", gv->rank[0], lv->tid, gv->prefetch_counter,last_gen_rank, step, CI, CJ, CK);
        gv->recv_tail-=2;
        //printf("Now, Node %d Prefetcher %d minus tail=%d\n", gv->rank[0],lv->tid,gv->recv_tail);
        // gv->prefetch_counter++;
      }

      pthread_mutex_unlock(&gv->lock_recv);


      if(flag == 1){

        new_buffer = (char*) malloc(gv->block_size+(1+1)*sizeof(int));

        *(int *)(new_buffer)=last_gen_rank;

        // fopen file
        for(j=0;j<gv->computer_group_size;j++){
          if(open[j]==0 && (last_gen_rank)==((gv->rank[0]-gv->num_compute_nodes)*gv->computer_group_size+j)){
            open[j]=1;
            // #define ADDRESS "/N/dc2/scratch/fuyuan/LBM/LBM%03dvs%03d/cid%03d/2lbm_cid%03d"
            sprintf(file_name,ADDRESS,gv->num_compute_nodes, gv->num_analysis_nodes,last_gen_rank, last_gen_rank);
            // sprintf(file_name,"/var/tmp/exp2_file_blk%d.data",blk_id);
            while((fp[last_gen_rank%gv->computer_group_size]==NULL) && (i<TRYNUM)){
              fp[last_gen_rank%gv->computer_group_size]=fopen(file_name,"rb+");
              if(fp[last_gen_rank%gv->computer_group_size]==NULL){
                if(i==TRYNUM-1){
                  perror("Reader fopen error: ");
                  printf("Warning: Analysis Process %d Reader %d read empty file last_gen_rank=%d, last_file_position=%d\n",
                    gv->rank[0], lv->tid, last_gen_rank, last_file_position);
                  fflush(stdout);
                }

                i++;
                usleep(5000);
              }
            }
          }
        }


        t0 = get_cur_time();
        read_blk(gv, lv, last_gen_rank, last_file_position,
          new_buffer+(1+1)*sizeof(int),fp[last_gen_rank%gv->computer_group_size]);    //read file block to buffer memory
        t1 = get_cur_time();
        block_id=*((int*)(new_buffer+(1+1)*sizeof(int)+gv->block_size-8));
        *(int *)(new_buffer+sizeof(int)) = block_id;
        // printf("Ana %d Reader %d start read rank%d last_blk_id=%d last_file_position=%d\n",
        //   gv->rank[0], lv->tid, last_gen_rank, block_id, last_file_position);
        // fflush(stdout);
        lv->read_time += t1 - t0;
        read_file_cnt++;


        t0 = get_cur_time();
        ring_buffer_put(gv,lv,new_buffer);
        t1 = get_cur_time();
        lv->ring_buffer_put_time += t1 - t0;

        flag = 0;

        pthread_mutex_lock(&gv->lock_recv);
        gv->ana_progress++;
        pthread_mutex_unlock(&gv->lock_recv);
      }

    }

    if(read_file_cnt>0){
      for(j=0;j<gv->computer_group_size;j++)
        fclose(fp[j]);
    }
  }

  free(fp);
  free(open);

  t3 = get_cur_time();
  printf("Analysis Process %d Reader %d total_time= %f, io_read_time = %f, only_fread_time = %f with %d blocks\n",
    gv->rank[0], lv->tid, t3-t2, lv->read_time, lv->only_fread_time, read_file_cnt);
  fflush(stdout);
}
