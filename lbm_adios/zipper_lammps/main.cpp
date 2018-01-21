/* ----------------------------------------------------------------------
   LAMMPS - Large-scale Atomic/Molecular Massively Parallel Simulator
   http://lammps.sandia.gov, Sandia National Laboratories
   Steve Plimpton, sjplimp@sandia.gov

   Copyright (2003) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under
   the GNU General Public License.

   See the README file in the top-level LAMMPS directory.
------------------------------------------------------------------------- */

 #include <mpi.h>
// #include "lammps.h"
// #include "input.h"
// #include <stdio.h>
// #include "concurrent.h"
// #include <iostream>
// #include <fstream>


#include "run_module_lammps.h"
#include "concurrent.h"

// using namespace LAMMPS_NS;

/* ----------------------------------------------------------------------
   main program to drive LAMMPS
------------------------------------------------------------------------- */


// #define STEP0FILE "/home/qoofyk/data_broker_lammps/job/step0.data"

// void init_step0data(GV gv, char* step0file){
//   double atom_id, type;

//   gv->xoriginal = (double *)malloc(sizeof(double)*gv->num_atom);
//   gv->yoriginal = (double *)malloc(sizeof(double)*gv->num_atom);
//   gv->zoriginal = (double *)malloc(sizeof(double)*gv->num_atom);

//   // open file
//   std::ifstream in_stream;
//   in_stream.open(step0file);
//   if (in_stream.fail()) {
//     printf("Fail to open file step0\n");
//     exit(1);  //exits the program
//   }

//   //read file
//   int i = 0;

//   while (in_stream >> atom_id >> type >> gv->xoriginal[i] >> gv->yoriginal[i] >> gv->zoriginal[i]) {

// // #ifdef DEBUG_PRINT
// //     // only the first ana_proc printf the debug_info
// //     if(gv->rank[0]==gv->compute_process_num){
// //       printf("atom_id=%.f, type=%.f, x0=%f, y0=%f, z0=%f\n",
// //         atom_id, type, gv->xoriginal[i], gv->yoriginal[i], gv->zoriginal[i]);
// //     }
// // #endif //DEBUG_PRINT
//     i++;
//     if(i>=gv->num_atom)
//       break;
//   }
//   in_stream.close();

// }

void init_msd(GV gv){

  int i, j;
  gv->msd = (double**)malloc(sizeof(double*)*5);

  for(i=0; i<5; i++){
    gv->msd[i] = (double*)malloc(sizeof(double)*(gv->total_num_dump_steps));
    for(j=0; j<(gv->total_num_dump_steps); j++)
      gv->msd[i][j]=0.0;
  }
}


#ifdef WRITE_ONE_FILE
void comp_open_one_big_file(GV gv){
  char file_name[256];
  int i=0;

  sprintf(file_name, "%s/results/cid%d", gv->filepath, gv->rank[0]);
  // sprintf(file_name,"/var/tmp/exp2_file_blk%d.data",blk_id);
  while(gv->fp==NULL){

    gv->fp=fopen(file_name,"wb");

    i++;

    if(i>1)
    usleep(OPEN_USLEEP);

    if(i>TRYNUM){
        printf("Fatal Error: Comp_Proc%d open an empty big file\n", gv->rank[0]);
        fflush(stdout);
        break;
    }
  }

}
#endif //WRITE_ONE_FILE

#ifdef WRITE_ONE_FILE
void ana_open_one_big_file(GV gv){
  int i, j, src;
  char file_name[256];

  for(j=0;j<gv->computer_group_size;j++){

    src=(gv->rank[0]-gv->compute_process_num)*gv->computer_group_size+j;

    sprintf(file_name, "%s/results/cid%d", gv->filepath, src);

    i=0;

    while((gv->ana_read_fp[j]==NULL) || (gv->ana_write_fp[j]==NULL)){

    gv->ana_read_fp[j]=fopen(file_name,"rb");
    gv->ana_write_fp[j]=fopen(file_name,"rb+");

    i++;
    if(i>1)
      usleep(OPEN_USLEEP);

    if(i>TRYNUM){
        printf("Fatal Error: Ana_Proc%d read an empty big file from%d\n", gv->rank[0], src);
        fflush(stdout);
        break;
    }

    }

  }
}
#endif //WRITE_ONE_FILE


int main(int argc, char **argv)
{
  GV              gv;
  LV              lvs;

  int provided;

  // char *cname[] = { "Compute_Group", "Analysis_Group"};

  int i;

  pthread_t       *thrds;
  pthread_attr_t  *attrs;
  void            *retval;

  double t0=0, t1=0, t2=0, t3=0;

  gv    = (GV) malloc(sizeof(*gv));

  //init IObox
  gv->compute_generator_num = atoi(argv[3]);
  gv->compute_writer_num = atoi(argv[4]);
  gv->analysis_reader_num = atoi(argv[5]);
  gv->analysis_writer_num = atoi(argv[6]);

  gv->computer_group_size=atoi(argv[10]);
  gv->analysis_process_num=atoi(argv[11]);
  gv->compute_process_num = gv->computer_group_size * gv->analysis_process_num;

  gv->data_id = 0;
  gv->dump_step_cnt = 0;

  gv->dump_lines_per_blk = atoi(argv[7]);
  gv->block_size = sizeof(int)*2 + sizeof(double)*5*gv->dump_lines_per_blk; //time_step, dump_lines_in_this_blk + data (atom_id,xs,ys,zs)

  gv->dump_step_internal = atoi(argv[8]);
  gv->num_atom = atof(argv[12]);
  gv->total_run_steps = atoi(argv[13]);
  gv->writer_prb_thousandth = atoi(argv[14]); //writer start to get element


  gv->total_num_dump_steps = gv->total_run_steps/gv->dump_step_internal + 1;

  // cpt_total_blks need to write into adaptive way in future
  // cpt_total_blks now is only a guess number
  gv->cpt_total_blks = (gv->num_atom/gv->compute_process_num/gv->dump_lines_per_blk)*gv->total_num_dump_steps;

  gv->writer_thousandth  = atoi(argv[9]);
  gv->writer_blk_num = gv->cpt_total_blks*gv->writer_thousandth/1000;
  gv->sender_blk_num = gv->cpt_total_blks - gv->writer_blk_num;

  gv->total_file = gv->cpt_total_blks*gv->block_size/(1024.0*1024.0); //MB

  gv->compute_data_len = sizeof(int) + gv->block_size + sizeof(int)*(gv->writer_blk_num*2+1); //blk_id + ||time_step,dump_lines_in_this_blk,data|| + written_id_array + mpi_send_count
  gv->analysis_data_len = sizeof(int)*4 + gv->block_size; // src,blkid,writer_state,consumer_state|time_step+dump_lines_in_this_blk+data
  // printf("writer_blk_num=%d, sender_blk_num=%d\n", gv->writer_blk_num, gv->sender_blk_num);
  // fflush(stdout);

  gv->calc_counter = 0;

  gv->full = 0;
  gv->filepath = getenv("SCRATCH_DIR");
  if(gv->filepath == NULL){
      fprintf(stderr, "scratch dir is not set!\n");
  }

  // MPI_Init(&argc,&argv);
  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
  MPI_Comm_rank(MPI_COMM_WORLD, &gv->rank[0]);
  MPI_Comm_size(MPI_COMM_WORLD, &gv->size[0]);
  MPI_Get_processor_name(gv->processor_name, &gv->namelen);

  // printf("Hello world! I'm rank %d of %d on %s with provided=%d, block_size=%d, dump_lines_per_blk=%d, #_double=%d\n",
  //   gv->rank[0], gv->size[0], gv->processor_name, provided, gv->block_size, gv->dump_lines_per_blk, 5*gv->dump_lines_per_blk);
  // fflush(stdout);


  if(gv->rank[0]<gv->compute_process_num)
    //Compute Group
    gv->color = 0;
  else
    //Analysis Group
    gv->color = 1;

  MPI_Comm_split(MPI_COMM_WORLD, gv->color, gv->rank[0], &gv->mycomm);

  MPI_Comm_rank(gv->mycomm, &gv->rank[1]);

  MPI_Comm_size(gv->mycomm, &gv->size[1]);

  // printf("%d: I’m  rank %d of %d in the %s context\n", gv->rank[0], gv->rank[1], gv->size[1], cname[gv->color]);
  // fflush(stdout);

  MPI_Barrier(MPI_COMM_WORLD);

  if (gv->color == 0){

//---------------------------- Init DataBroker ---------------------------------------------------//
    // lammps_thread <--> compute_writer, compute_sender
    int num_total_thrds = gv->compute_writer_num+1;
    lvs   = (LV) malloc(sizeof(*lvs)*num_total_thrds);
    thrds = (pthread_t*) malloc(sizeof(pthread_t)*num_total_thrds);
    attrs = (pthread_attr_t*) malloc(sizeof(pthread_attr_t)*num_total_thrds);

    gv->all_lvs = lvs;

    //producer_ring_buffer initialize
    ring_buffer producer_rb;
    producer_rb.bufsize = PRODUCER_RINGBUFFER_TOTAL_MEMORY/(gv->block_size-2*sizeof(int));
    producer_rb.head = 0;
    producer_rb.tail = 0;
    producer_rb.num_avail_elements = 0;
    producer_rb.buffer = (char**)malloc(sizeof(char*)*producer_rb.bufsize);
    producer_rb.lock_ringbuffer = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
    producer_rb.full = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
    producer_rb.empty = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
    pthread_mutex_init(producer_rb.lock_ringbuffer, NULL);
    pthread_cond_init(producer_rb.full, NULL);
    pthread_cond_init(producer_rb.empty, NULL);
    gv->producer_rb_p = &producer_rb;

    //gv->producer_rb_p = rb_init(gv,PRODUCER_RINGBUFFER_TOTAL_MEMORY,&producer_rb);
    //compute node
    if(gv->rank[0]==0 || gv->rank[0]==(gv->compute_process_num-1)){
      printf("Comp_Proc%04d of %d on %s: atom#=%.1f, dump_lines/blk=%d, double#/blk=%d, total_dump_steps#=%d, \
H_cpt_total_blk#=%d, H_writer_blk#=%d, sender_blk#=%d, blk_size=%dKB, \
PRB %.3fGB, size=%d\n",
    gv->rank[0], gv->size[1], gv->processor_name, gv->num_atom,
    gv->dump_lines_per_blk, 5*gv->dump_lines_per_blk, gv->total_num_dump_steps,
    gv->cpt_total_blks, gv->writer_blk_num, gv->sender_blk_num, gv->block_size/1024,
    PRODUCER_RINGBUFFER_TOTAL_MEMORY/(1024.0*1024.0*1024.0), gv->producer_rb_p->bufsize);
      fflush(stdout);
    }

    gv->written_id_array = (int *) malloc(sizeof(int)*gv->writer_blk_num*2); //blkid + #lines_in_this_block
    gv->send_tail = 0;

    gv->flag_sender_get_finalblk = 0;
    gv->flag_writer_get_finalblk = 0;
    gv->writer_exit = 0;

#ifndef MPI_NOKEEP
#ifdef WRITE_ONE_FILE
    //init gv->fp
    gv->fp = NULL;
    comp_open_one_big_file(gv);
    MPI_Barrier(MPI_COMM_WORLD);
#endif //WRITE_ONE_FILE
#endif //MPI_NOKEEP

    //init lock
    pthread_mutex_init(&gv->lock_block_id, NULL);
    pthread_mutex_init(&gv->lock_disk_id_arr, NULL);
    pthread_mutex_init(&gv->lock_writer_done, NULL);
//--------------------------------------------------------------------------------------------//

    t0=MPI_Wtime();

    /* Create threads */
    for(i=0; i<num_total_thrds; i++) {
      init_lv(lvs+i, i, gv);
      if(pthread_attr_init(attrs+i)) perror("attr_init()");
      if(pthread_attr_setscope(attrs+i, PTHREAD_SCOPE_SYSTEM)) perror("attr_setscope()");
      if(pthread_create(thrds+i, attrs+i, compute_node_do_thread, lvs+i)) {
        perror("pthread_create()");
        exit(1);
      }
    }

    //-------------------------- LAMMPS start --------------------------//
    t2 = MPI_Wtime();

    // LAMMPS *lammps = new LAMMPS(3, argv, gv->mycomm);
    // lammps->input->file();
    // // delete lammps;

    run_module_lammps(argc, argv, gv, &gv->mycomm);

    t3 =  MPI_Wtime();
    //-------------------------- LAMMPS ends --------------------------//

    /* Join threads */
    for(i = 0; i < num_total_thrds; i++) {
      pthread_join(thrds[i], &retval);
      // printf("Compute Node %d Thread %d is finished\n", gv->rank[0] ,i);
    }

#ifndef MPI_NOKEEP
#ifdef WRITE_ONE_FILE
    fclose(gv->fp);
#endif //WRITE_ONE_FILE
#endif //MPI_NOKEEP

    t1=MPI_Wtime();

    free(lvs);
    free(attrs);
    free(thrds);

    printf("Comp_Proc%04d: Job finish on %s, T_total=%.3f, T_lammps=%.3f, cnt=%d, full=%d\n",
      gv->rank[0], gv->processor_name, t1-t0, t3-t2, gv->data_id, gv->full);
    fflush(stdout);

  }
  else{
    //Ana_Proc
    gv->ana_total_blks = gv->computer_group_size * gv->cpt_total_blks;

    gv->reader_blk_num = gv->computer_group_size * gv->writer_blk_num;
    gv->analysis_writer_blk_num = gv->ana_total_blks - gv->reader_blk_num;

    //ana_receiver, ana_reader <--> ana_writer, ana_consumer
    int num_total_thrds= gv->analysis_reader_num+gv->analysis_writer_num+2;
    lvs   = (LV) malloc(sizeof(*lvs)*num_total_thrds);
    thrds = (pthread_t*) malloc(sizeof(pthread_t)*num_total_thrds);
    attrs = (pthread_attr_t*) malloc(sizeof(pthread_attr_t)*num_total_thrds);

    gv->all_lvs = lvs;

    //Consumer_ring_buffer initialize
    ring_buffer consumer_rb;
    consumer_rb.bufsize = CONSUMER_RINGBUFFER_TOTAL_MEMORY/(gv->block_size-2*sizeof(int));
    consumer_rb.head = 0;
    consumer_rb.tail = 0;
    consumer_rb.num_avail_elements = 0;
    consumer_rb.buffer = (char**)malloc(sizeof(char*)*consumer_rb.bufsize);
    consumer_rb.lock_ringbuffer = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
    consumer_rb.full = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
    consumer_rb.empty = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
    consumer_rb.new_tail = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
    pthread_mutex_init(consumer_rb.lock_ringbuffer, NULL);
    pthread_cond_init(consumer_rb.full, NULL);
    pthread_cond_init(consumer_rb.empty, NULL);
    pthread_cond_init(consumer_rb.new_tail, NULL);
    gv->consumer_rb_p = &consumer_rb;

    //gv->consumer_rb_p = rb_init(gv,CONSUMER_RINGBUFFER_TOTAL_MEMORY,&consumer_rb);

    if(gv->rank[0]==gv->compute_process_num || gv->rank[0]==(gv->compute_process_num+gv->analysis_process_num-1)){
      printf("Ana_Proc%d of %d on %s: ana_total_blks=%d, reader_blk#=%d, analysis_writer_blk#=%d, \
CRB %.3fGB, size=%d\n",
        gv->rank[0], gv->size[1], gv->processor_name, gv->ana_total_blks, gv->reader_blk_num, gv->analysis_writer_blk_num,
        CONSUMER_RINGBUFFER_TOTAL_MEMORY/(1024.0*1024.0*1024.0), gv->consumer_rb_p->bufsize);
      fflush(stdout);
    }

    gv->org_recv_buffer = (char *) malloc(gv->compute_data_len); //long message+
    check_malloc(gv->org_recv_buffer);

    // prfetch threads 1+1+1: cid+blkid+dump_lines_in_this_blk
    gv->recv_exit     = 0;
    gv->reader_exit   = 0;
    gv->ana_writer_exit = 0;

    gv->prefetch_id_array = (int *) malloc(sizeof(int)*3*gv->reader_blk_num); //3: src(compute_proc_rank), blkid, dump_lines_in_this_blk
    gv->recv_head     = 0;
    gv->recv_tail     = 0;
    gv->recv_avail    = 0;
    check_malloc(gv->prefetch_id_array);

    // Initialize of MSD
    init_msd(gv);

//     // Init Step0file
// #ifdef READ_STEP0
//     char step0file[128];
//     strcpy (step0file, argv[14]);
//     init_step0data(gv, step0file);
// #endif //READ_STEP0

#ifndef MPI_NOKEEP
#ifdef WRITE_ONE_FILE
    //wait for compute_proc create big files
    MPI_Barrier(MPI_COMM_WORLD);
      //init every ana_fp[i]
    gv->ana_read_fp = (FILE **)malloc(sizeof(FILE *) * gv->computer_group_size);
    gv->ana_write_fp = (FILE **)malloc(sizeof(FILE *) * gv->computer_group_size);
    for(i=0; i<gv->computer_group_size; i++){
      gv->ana_read_fp[i]=NULL;
      gv->ana_write_fp[i]=NULL;
    }
    ana_open_one_big_file(gv);
#endif //WRITE_ONE_FILE
#endif //MPI_NOKEEP

    //initialize lock
    pthread_mutex_init(&gv->lock_recv_disk_id_arr, NULL);

    t0=MPI_Wtime();
    /* Create threads */
    for(i=0; i<num_total_thrds; i++) {
      init_lv(lvs+i, i, gv);
      if(pthread_attr_init(attrs+i)) perror("attr_init()");
      if(pthread_attr_setscope(attrs+i, PTHREAD_SCOPE_SYSTEM)) perror("attr_setscope()");
      if(pthread_create(thrds+i, attrs+i, analysis_node_do_thread, lvs+i)) {
        perror("pthread_create()");
        exit(1);
      }
    }

    /* Join threads */
    for(i=0; i<num_total_thrds; i++) {
      pthread_join(thrds[i], &retval);
      // printf("Ana_Proc%04d: Thread %d is finished\n", gv->rank[0], i);
      // fflush(stdout);
    }

#ifndef MPI_NOKEEP
#ifdef WRITE_ONE_FILE
    free(gv->ana_read_fp);
    free(gv->ana_write_fp);
#endif //WRITE_ONE_FILE
#endif //MPI_NOKEEP

    free(lvs);
    free(attrs);
    free(thrds);

    // printf("Ana_Proc%04d: pass free variables\n", gv->rank[0]);
    // fflush(stdout);

    t1=MPI_Wtime();
    printf("Ana_Proc%04d: Job finished on %s, T_ana_total=%.3f\n",
      gv->rank[0], gv->processor_name, t1-t0);
    fflush(stdout);
  }

  // MPI_Barrier(MPI_COMM_WORLD);

  // printf("Proc%04d: Pass MPI_Barrier\n", gv->rank[0]);
  // fflush(stdout);
  MPI_Comm_free(&gv->mycomm);
  MPI_Finalize();

  // printf("Proc%04d: Pass MPI_Finalize\n", gv->rank[0]);
  // fflush(stdout);

  free(gv);

  return 0;
}
