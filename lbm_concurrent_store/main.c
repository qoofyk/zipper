#include "concurrent.h"

int main(int argc, char **argv){

	MPI_Comm mycomm;
	LV              lvs;
	GV              gv;
	int provided;

	// char *cname[] = { "Compute_Group", "Analysis_Group", "BLUE" };

	int i;

	pthread_t       *thrds;
	pthread_attr_t  *attrs;
	void            *retval;

	ring_buffer producer_rb;
	ring_buffer consumer_rb;

	double t0=0,t1=0,t2=0,t3=0;

	if(argc != 12) {
		fprintf(stderr, "Usage: %s generator_num, writer_num, reader#, writer_thousandth, \
computer_group_size, num_analysis_nodes, cubex, cubez, step_stop, lp\n", argv[0]);
		exit(1);
	}

	gv    = (GV) malloc(sizeof(*gv));

	//init lbm
	gv->cubex	=	atoi(argv[8]);
	gv->cubey	=	atoi(argv[8]);
	gv->cubez	=	atoi(argv[9]);

	gv->X 	=	nx/gv->cubex;
	gv->Y 	=	ny/gv->cubey;
	gv->Z 	= 	nz/gv->cubez;

	gv->step_stop = atoi(argv[10]);

	gv->lp = atoi(argv[11]); //Loop times in Analysis calc_n_moments

	//init IObox
	gv->compute_generator_num = atoi(argv[1]);
	gv->compute_writer_num = atoi(argv[2]);
	gv->analysis_reader_num = atoi(argv[3]);
	gv->analysis_writer_num = atoi(argv[4]);

	gv->data_id = 0;
	gv->mpi_send_progress_counter = 0;

	gv->block_size = sizeof(int)*4+sizeof(double)*(gv->cubex*gv->cubey*gv->cubez*2); //64K B+4B,step,ci,cj,ck,data

  	gv->cpt_total_blks = gv->X*gv->Y*gv->Z*gv->step_stop;
  	gv->writer_thousandth  = atoi(argv[5]);
  	gv->writer_blk_num = gv->cpt_total_blks*gv->writer_thousandth/1000;
  	gv->sender_blk_num = gv->cpt_total_blks - gv->writer_blk_num;
  	gv->total_file = (long)gv->cpt_total_blks*(long)gv->block_size/1024; //KB
  	gv->compute_data_len = sizeof(int)+sizeof(char)*gv->block_size+sizeof(int)*(gv->writer_blk_num+1);//blk_id,step,ci,cj,ck,data,written_id
  	gv->analysis_data_len = sizeof(int)*3+sizeof(char)*gv->block_size; // src,blkid,writer_state/consumer_state | step,ci,cj,ck,data
  	// printf("writer_blk_num=%d, sender_blk_num=%d\n", gv->writer_blk_num, gv->sender_blk_num);
  	// fflush(stdout);

  	gv->computer_group_size=atoi(argv[6]);
  	gv->analysis_process_num=atoi(argv[7]);
  	//gv->msleep = atof(argv[8]);
  	gv->compute_process_num = gv->computer_group_size * gv->analysis_process_num;
  	gv->calc_counter = 0;
  	// gv->prefetch_counter = 0;

	// MPI_Init(&argc, &argv);
	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
	MPI_Comm_rank(MPI_COMM_WORLD, &gv->rank[0]);
	MPI_Comm_size(MPI_COMM_WORLD, &gv->size[0]);
	MPI_Get_processor_name(gv->processor_name, &gv->namelen);
	// printf("Hello world! I’m rank %d of %d on %s\n", gv->rank[0], gv->size[0], gv->processor_name);
	// fflush(stdout);

	if(gv->rank[0]<gv->compute_process_num)
	  //Compute Group
	  gv->color = 0;
	else
	  //Analysis Group
	  gv->color = 1;

	MPI_Comm_split(MPI_COMM_WORLD, gv->color, gv->rank[0], &mycomm);

	MPI_Comm_rank(mycomm, &gv->rank[1]);

	MPI_Comm_size(mycomm, &gv->size[1]);

	// printf("%d: I’m  rank %d of %d in the %s context\n", gv->rank[0], gv->rank[1], gv->size[1], cname[gv->color]);
	// fflush(stdout);

	MPI_Barrier(MPI_COMM_WORLD);

	//debug_print(gv->rank[0]);

	if (gv->color == 0){
		// int last_gen=0; //last generate blk_id
		MPI_Status status;
		double df1[nx][ny][nz][19],df2[nx][ny][nz][19],df_inout[2][ny][nz][19];
		double rho[nx][ny][nz],u[nx][ny][nz],v[nx][ny][nz],w[nx][ny][nz];
		double c[19][3],dfeq,pow_2;
		int i,j,k,m,n1,n2,n3,ii,dt,time1,time2,time3;
		//int x_mid,y_mid,z_mid,step_wr;
		double width,length,nu_e,tau,rho_e,u_e, Re,time_stop;
		//double cs;
		double length_r,time_r,s1, s2, s3,s4, nu_lb, u_lb, rho_lb, rho_r, u_r;
		//double time_restart;
		double p_bb,depth;

		MPI_Comm comm1d;
		MPI_Datatype newtype,newtype_bt,newtype_fr;
		int nbleft,nbright,left_most,right_most,middle;
		int num_data,s,e,myid;
		//int num_data1,nj;
		int np[1],period[1];
		int *fp_np,*fp_period;
		// FILE *fp_u, *fp_v, *fp_out;
		FILE *fp_out;
		double t2=0, t3=0,t4=0,t5=0,t6=0,only_lbm_time=0,init_lbm_time=0;
		int errorcode;

		char * buffer; //to insert into databroker

		#ifdef DEBUG_PRINT
		printf("Comp_Proc%d: Setup LBM parameter!\n",gv->rank[0]);
		fflush(stdout);
		#endif //DEBUG_PRINT

		n1=nx-1;/* n1,n2,n3 are the last indice in arrays*/
		n2=ny-1;
		n3=nz-1;

		num_data=ny*nz;/* number of data to be passed from process to process */
		//num_data1=num_data*19;
		int nprocs=gv->size[1];
		np[0]=nprocs;period[0]=0;
		fp_np=np;
		fp_period=period;

		MPI_Type_vector(num_data,1,19,MPI_DOUBLE,&newtype);
		MPI_Type_commit(&newtype);

		MPI_Type_vector(ny,1,nz*19,MPI_DOUBLE,&newtype_bt);
		MPI_Type_commit(&newtype_bt);

		MPI_Type_vector(nz,1,19,MPI_DOUBLE,&newtype_fr);
		MPI_Type_commit(&newtype_fr);


		errorcode=MPI_Cart_create(mycomm,1,fp_np,fp_period,1,&comm1d);
		if(errorcode!= MPI_SUCCESS){
		  printf("Error cart create!\n");
		  exit(1);
		}

		MPI_Comm_rank(comm1d,&myid);
		errorcode=MPI_Cart_shift(comm1d,0,1,&nbleft,&nbright);
		if(errorcode!= MPI_SUCCESS){
		  printf("Error shift create!\n");
		  exit(1);
		}

		lvs   = (LV) malloc(sizeof(*lvs)*(gv->compute_writer_num+1+1));
		thrds = (pthread_t*) malloc(sizeof(pthread_t)*(gv->compute_writer_num+1+1));
		attrs = (pthread_attr_t*) malloc(sizeof(pthread_attr_t)*(gv->compute_writer_num+1+1));

		gv->all_lvs = lvs;

		//producer_ring_buffer initialize
	    producer_rb.bufsize = PRODUCER_RINGBUFFER_TOTAL_MEMORY/(gv->block_size-4*sizeof(int));
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
	    	printf("Comp_Proc%d of %d cpt_total_blks=%d, writer_blk_num=%d, sender_blk_num=%d, \
PRODUCER_Ringbuffer %.3fGB, size=%d member\n",
			gv->rank[0], gv->size[1], gv->cpt_total_blks, gv->writer_blk_num, gv->sender_blk_num,
			PRODUCER_RINGBUFFER_TOTAL_MEMORY/(1024.0*1024.0*1024.0),gv->producer_rb_p->bufsize);
		    fflush(stdout);
	    }

		gv->written_id_array = (int *) malloc(sizeof(int)*gv->writer_blk_num);
		gv->send_tail = 0;

		gv->flag_sender_get_finalblk = 0;
    	gv->flag_writer_get_finalblk = 0;

		//init lock
		pthread_mutex_init(&gv->lock_block_id,NULL);
	    pthread_mutex_init(&gv->lock_writer_progress, NULL);
	    pthread_mutex_init(&gv->lock_writer_done, NULL);

		t0=get_cur_time();

		/* Create threads */
		for(i = 0; i < (gv->compute_writer_num+1); i++) {
		  init_lv(lvs+i, i, gv);
		  if(pthread_attr_init(attrs+i)) perror("attr_init()");
		  if(pthread_attr_setscope(attrs+i, PTHREAD_SCOPE_SYSTEM)) perror("attr_setscope()");
		  if(pthread_create(thrds+i, attrs+i, compute_node_do_thread, lvs+i)) {
		    perror("pthread_create()");
		    exit(1);
		  }
		}

		//---------------------------------------------BEGIN LBM -----------------------------------------------
		#ifdef DEBUG_PRINT
		printf("Comp_Proc%d: Begin LBM!\n",gv->rank[0]);
		fflush(stdout);
		#endif //DEBUG_PRINT

		t2 = get_cur_time();

		// x_mid=1+(nx-4)/2;

		// y_mid=1+(ny-4)/2;

		// z_mid=1+(nz-4)/2;

		left_most=0;right_most=nprocs-1;middle=nprocs/2;

		// if( ((float)nprocs-(float)middle*2.0) < 1.0e-3) x_mid=2;

		s=2; /* the array for each process is df?[0:n1][][][], the actual part is [s:e][][][] */

		e=n1-2;



		p_bb=0.03;

		depth=3.0e-5; /*30 microns */

		width=3.0e-2;  /* unit: cm i.e. 300 micron*/

		length=width*2.0;

		u_e=0.616191358; /* cm/sec */

		rho_e=0.99823; /* g/cm^3 */

		nu_e=1.007e-2; /* cm^2/sec */

		Re=u_e*depth/nu_e;



		tau=0.52;

		nu_lb=(2.0*tau-1.0)/6.0;

		u_lb=Re*nu_lb/(nz-4);

		rho_lb=0.3;



		/* real quantities in units cm*g*sec = reference quantities x quantities in simulation */

		/*depth-dx/(nz-4-1)=dx,solve it get the following length_r*/

		length_r=depth/(nz-4);  /* cm */

		time_r=depth*u_lb/((nz-4)*u_e);  /* sec, note that (nz-4), the # of nodes used to represent the depth of wall

		                  is the correct length scale L_lb in LBM */

		rho_r=rho_e/rho_lb;    /* g/cm^3 */

		u_r=u_e/u_lb;  /* cm/sec */



		/*nu_lb=u_lb*n2*nu_e/(u_e*width);*/

		/*nu_lb=u_lb*n3/Re;

		tau=(6.0*nu_lb+1.0)*0.5;*/



		dt=1;



		// cs=1.0/(sqrt(3));  /* non-dimensional */

		time_stop=depth/u_e*1.0; /* 10 times of the characteric time, which can be defined to be depth/u_e */

		//step_stop=(int)(time_stop/time_r) + 1;

		// gv->step_stop = atoi(argv[4]);
		// step_wr=gv->step_stop/2-1;

		// time_restart=400000;

		time1=dt;

		time2=dt;

		time3=dt;

		gv->step=0;

		if (myid==middle) {

		fp_out=fopen("output_p.d","w");

		fprintf (fp_out,"the # of CPUs used in x-direction is %d   \n",nprocs);

		fprintf(fp_out,"the kinematic viscosity of the fluid is %e cm^2/sec\n",nu_e);

		fprintf(fp_out,"the mass density of fluid in channel is %e g/cm^3\n",rho_e);

		fprintf(fp_out,"the inlet speed is %e cm/sec\n",u_e);

		fprintf(fp_out,"the channel length is %e cm\n",length);

		fprintf(fp_out,"the channel width is %e cm\n", width);

		fprintf(fp_out,"the channel depth is %e cm\n", depth);

		fprintf(fp_out,"the Re number of the flow is %e \n",Re);

		fprintf(fp_out,"the number of lattice nodes in x-direction is %d \n",nx-4);

		fprintf(fp_out,"the number of lattice nodes in y-direction is %d \n",ny-4);

		fprintf(fp_out,"the number of lattice nodes in z-direction is %d \n",nz-4);

		fprintf(fp_out,"the inlet speed in simulation is %e \n",u_lb);

		fprintf(fp_out,"the mass density in simulation is %e \n",rho_lb);

		fprintf(fp_out,"the reference length is %e cm\n",length_r);

		fprintf(fp_out,"the reference time is %e sec\n",time_r);

		fprintf(fp_out,"the reference mass density is %e g/cm^3\n",rho_r);

		fprintf(fp_out,"the reference speed is %e cm/sec\n",u_r);

		fprintf(fp_out,"the kinematic viscosity in simulation is %e cm^2/sec\n",nu_lb);

		fprintf(fp_out,"the relaxation time tau in simulation is %e\n",tau);

		fprintf(fp_out,"the total time run is %e sec\n", time_stop);

		fprintf(fp_out,"the total step run is %d steps\n", gv->step_stop);

		fclose(fp_out);

		}


		// #ifdef DEBUG_PRINT
		// printf("Compute Node %d Generator LBM Here 1 !\n",gv->rank[0]);
		// fflush(stdout);
		// #endif //DEBUG_PRINT

		/* discrete particle velocity */

		c[0][0]=0.0;c[0][1]=0.0;c[0][2]=0;



		c[1][0]=1.0;c[1][1]=0.0;c[1][2]=0;

		c[2][0]=-1.0;c[2][1]=0.0;c[2][2]=0.0;

		c[3][0]=0.0;c[3][1]=0.0;c[3][2]=1;

		c[4][0]=0.0;c[4][1]=0.0;c[4][2]=-1;

		c[5][0]=0.0;c[5][1]=-1.0;c[5][2]=0;

		c[6][0]=0.0;c[6][1]=1.0;c[6][2]=0;



		c[7][0]=1.0;c[7][1]=0.0;c[7][2]=1;

		c[8][0]=-1.0;c[8][1]=0.0;c[8][2]=-1;

		c[9][0]=1.0;c[9][1]=0.0;c[9][2]=-1.0;

		c[10][0]=-1.0;c[10][1]=0.0;c[10][2]=1.0;

		c[11][0]=0.0;c[11][1]=-1.0;c[11][2]=1.0;

		c[12][0]=0.0;c[12][1]=1.0;c[12][2]=-1.0;

		c[13][0]=0.0;c[13][1]=1.0;c[13][2]=1.0;

		c[14][0]=0.0;c[14][1]=-1.0;c[14][2]=-1.0;

		c[15][0]=1.0;c[15][1]=-1.0;c[15][2]=0.0;

		c[16][0]=-1.0;c[16][1]=1.0;c[16][2]=0.0;

		c[17][0]=1.0;c[17][1]=1.0;c[17][2]=0.0;

		c[18][0]=-1.0;c[18][1]=-1.0;c[18][2]=0.0;



		if(myid==left_most) s=1;

		if(myid==right_most) e=n1-1;

		// #ifdef DEBUG_PRINT
		// printf("Compute Node %d Generator LBM Here 1.1 !\n",gv->rank[0]);
		// fflush(stdout);
		// #endif //DEBUG_PRINT

		/* initialization of rho and (u,v), non-dimensional */

		/*for (i=1;i<=n1-1;i++)*/

		for (i=s;i<=e;i++)

		 for (j=1;j<=n2-1;j++)

		for (k=1;k<=n3-1;k++)

		    {

		    rho[i][j][k]=rho_lb;

		    u[i][j][k]=0.0;

		    v[i][j][k]=0.0;

		        w[i][j][k]=0.0;

		    }

		// #ifdef DEBUG_PRINT
		// printf("Compute Node %d Generator LBM Here 1.2 !\n",gv->rank[0]);
		// fflush(stdout);
		// #endif //DEBUG_PRINT

		if (myid==left_most) {

		/* inlet speed */

		i=2;

		for (j=1;j<=n2-1;j++)

		 for (k=1;k<=n3-1;k++)

		{u[i][j][k]=u_lb;}

		}



		if (myid==right_most) {

		/* outlet speed */

		i=n1-2;

		for (j=1;j<=n2-1;j++)

		 for(k=1;k<=n3-1;k++)

		{u[i][j][k]=u_lb;}

		}


		// #ifdef DEBUG_PRINT
		// printf("Compute Node %d Generator LBM Here 1.3 !\n",gv->rank[0]);
		// fflush(stdout);
		// #endif //DEBUG_PRINT

		// df1[2][1][1][0]=0;
		// #ifdef DEBUG_PRINT
		// printf("Compute Node %d Generator LBM Here 1.31 !\n",gv->rank[0]);
		// fflush(stdout);
		// #endif //DEBUG_PRINT

		/* equilibrium distribution function df0 */

		/*for (i=1;i<=n1-1;i++)*/

		for (i=s;i<=e;i++){
		// #ifdef DEBUG_PRINT
		// printf("Compute Node %d Generator LBM Here 1.32 i=%d!\n",gv->rank[0],i);
		// fflush(stdout);
		// #endif //DEBUG_PRINT

		for(j=1;j<=n2-1;j++)

		    for(k=1;k<=n3-1;k++){

		  df1[i][j][k][0]=1.0/3.0*rho[i][j][k]*(1.0-1.5*(u[i][j][k]*u[i][j][k]

		    +v[i][j][k]*v[i][j][k]+w[i][j][k]*w[i][j][k]));

		    for (m=1;m<=18;m++)  {
		      pow_2=c[m][0]*u[i][j][k]+c[m][1]*v[i][j][k]+c[m][2]*w[i][j][k];

		        df1[i][j][k][m]=1.0/18.0*rho[i][j][k]*(1.0+3.0*(c[m][0]*u[i][j][k]+c[m][1]*v[i][j][k]

		          +c[m][2]*w[i][j][k])

		                /*+4.5*pow(c[m][0]*u[i][j][k]+c[m][1]*v[i][j][k]+c[m][2]*w[i][j][k],2)*/

		                +4.5*pow_2*pow_2

		            -1.5*(u[i][j][k]*u[i][j][k]+v[i][j][k]*v[i][j][k]+w[i][j][k]*w[i][j][k]));

		      }



		    for (m=7;m<=18;m++)
		      {df1[i][j][k][m]=0.5*df1[i][j][k][m];}

		 }
		}



		// #ifdef DEBUG_PRINT
		// printf("Compute Node %d Generator LBM Here 1.4 !\n",gv->rank[0]);
		// fflush(stdout);
		// #endif //DEBUG_PRINT


		ii=0;

		for (i=2;i<=n1-2;i+=n1-4)

		 {

		 if (i>3) ii=1;

		 for(j=1;j<=n2-1;j++)

		    for(k=1;k<=n3-1;k++)

		       for (m=0;m<=18;m++)

		    {

		    df_inout[ii][j][k][m]=df1[i][j][k][m];

		    }



		}/*end of computing the inlet and outlet d.f. */


		t4=get_cur_time();
		init_lbm_time=t4-t2;
		only_lbm_time+=t4-t2;
		if(myid==0){
			printf("Init LBM Time=%f\n", init_lbm_time);
			fflush(stdout);
		}

		//int blk_id=0;

		while (gv->step < gv->step_stop)

		{

		t5=get_cur_time();

		if (myid==0){
			printf("step=%d of %d\n", gv->step, gv->step_stop);

			fflush(stdout);
		}





		s=2;e=n1-2;



		/* collision */

		/*for (i=2;i<=n1-2;i++)*/

		for (i=s;i<=e;i++)

		   for (j=2;j<=n2-2;j++) /* changing from n2-1 to n2-2 after using two walls */

		      for (k=2;k<=n3-2;k++)

		         for (m=0;m<=18;m++)

		  {



		    if (m==0)

		   {

		dfeq=1.0/3.0*rho[i][j][k]*(1.0-1.5*(u[i][j][k]*u[i][j][k]

		    +v[i][j][k]*v[i][j][k]+w[i][j][k]*w[i][j][k]));}



		else if (m>=1 && m<=6)

		      {

		               pow_2=c[m][0]*u[i][j][k]+c[m][1]*v[i][j][k]+c[m][2]*w[i][j][k];

		      dfeq=1.0/18.0*rho[i][j][k]*(1.0+3.0*(c[m][0]*u[i][j][k]+c[m][1]*v[i][j][k]

		      +c[m][2]*w[i][j][k])

		               /*+4.5*pow(c[m][0]*u[i][j][k]+c[m][1]*v[i][j][k]+c[m][2]*w[i][j][k],2)*/

		               +4.5*pow_2*pow_2

		         -1.5*(u[i][j][k]*u[i][j][k]+v[i][j][k]*v[i][j][k]+w[i][j][k]*w[i][j][k]));

		      }

		else

		      {

		               pow_2=c[m][0]*u[i][j][k]+c[m][1]*v[i][j][k]+c[m][2]*w[i][j][k];

		      dfeq=1.0/36.0*rho[i][j][k]*(1.0+3.0*(c[m][0]*u[i][j][k]+c[m][1]*v[i][j][k]

		      +c[m][2]*w[i][j][k])

		               /*+4.5*pow(c[m][0]*u[i][j][k]+c[m][1]*v[i][j][k]+c[m][2]*w[i][j][k],2)*/

		               +4.5*pow_2*pow_2

		         -1.5*(u[i][j][k]*u[i][j][k]+v[i][j][k]*v[i][j][k]+w[i][j][k]*w[i][j][k]));

		      }



		      df1[i][j][k][m]=df1[i][j][k][m]*(1-1/tau)+1/tau*dfeq;

		  }



		/* streaming */

		/*for (i=2;i<=n1-2;i++) */

		for (i=s;i<=e;i++)

		   for (j=2;j<=n2-2;j++)

		      for (k=2;k<=n3-2;k++)

		      {

		      df2[i][j][k][0]    =df1[i][j][k][0];



		      df2[i+1][j][k][1]=df1[i][j][k][1];

		      df2[i-1][j][k][2]=df1[i][j][k][2];

		      df2[i][j][k+1][3]=df1[i][j][k][3];

		      df2[i][j][k-1][4]=df1[i][j][k][4];

		      df2[i][j-1][k][5]=df1[i][j][k][5];

		      df2[i][j+1][k][6]=df1[i][j][k][6];



		      df2[i+1][j][k+1][7]=df1[i][j][k][7];

		      df2[i-1][j][k-1][8]=df1[i][j][k][8];

		      df2[i+1][j][k-1][9]=df1[i][j][k][9];

		      df2[i-1][j][k+1][10]=df1[i][j][k][10];

		      df2[i][j-1][k+1][11]=df1[i][j][k][11];

		      df2[i][j+1][k-1][12]=df1[i][j][k][12];

		      df2[i][j+1][k+1][13]=df1[i][j][k][13];

		      df2[i][j-1][k-1][14]=df1[i][j][k][14];

		      df2[i+1][j-1][k][15]=df1[i][j][k][15];

		      df2[i-1][j+1][k][16]=df1[i][j][k][16];

		      df2[i+1][j+1][k][17]=df1[i][j][k][17];

		      df2[i-1][j-1][k][18]=df1[i][j][k][18];

		      }


		#ifdef DEBUG_PRINT
		printf("Comp_Proc%d: Start LBM data MPI_Sendrecv!\n",gv->rank[0]);
		fflush(stdout);
		#endif //DEBUG_PRINT

		/* data sending and receiving*/

		MPI_Sendrecv(&df2[e+1][0][0][1],1,newtype,nbright,1,&df2[s][0][0][1],1,newtype,nbleft,1,comm1d,&status);
		// MPI_Barrier(comm1d);
		MPI_Sendrecv(&df2[e+1][0][0][7],1,newtype,nbright,7,&df2[s][0][0][7],1,newtype,nbleft,7,comm1d,&status);
		// MPI_Barrier(comm1d);
		MPI_Sendrecv(&df2[e+1][0][0][9],1,newtype,nbright,9,&df2[s][0][0][9],1,newtype,nbleft,9,comm1d,&status);
		// MPI_Barrier(comm1d);
		MPI_Sendrecv(&df2[e+1][0][0][15],1,newtype,nbright,15,&df2[s][0][0][15],1,newtype,nbleft,15,comm1d,&status);
		// MPI_Barrier(comm1d);
		MPI_Sendrecv(&df2[e+1][0][0][17],1,newtype,nbright,17,&df2[s][0][0][17],1,newtype,nbleft,17,comm1d,&status);
		// MPI_Barrier(comm1d);


		MPI_Sendrecv(&df2[s-1][0][0][2],1,newtype,nbleft,2,&df2[e][0][0][2],1,newtype,nbright,2,comm1d,&status);
		// MPI_Barrier(comm1d);
		MPI_Sendrecv(&df2[s-1][0][0][8],1,newtype,nbleft,8,&df2[e][0][0][8],1,newtype,nbright,8,comm1d,&status);
		// MPI_Barrier(comm1d);
		MPI_Sendrecv(&df2[s-1][0][0][10],1,newtype,nbleft,10,&df2[e][0][0][10],1,newtype,nbright,10,comm1d,&status);
		// MPI_Barrier(comm1d);
		MPI_Sendrecv(&df2[s-1][0][0][16],1,newtype,nbleft,16,&df2[e][0][0][16],1,newtype,nbright,16,comm1d,&status);
		// MPI_Barrier(comm1d);
		MPI_Sendrecv(&df2[s-1][0][0][18],1,newtype,nbleft,18,&df2[e][0][0][18],1,newtype,nbright,18,comm1d,&status);
		// MPI_Barrier(comm1d);


		/*sending  and receiving data for boundary conditions for df1[][][][]u*/

		MPI_Sendrecv(&df1[e][0][2][9],1,newtype_bt,nbright,99,

		           &df1[s-1][0][2][9],1,newtype_bt,nbleft,99,comm1d,&status);
		// MPI_Barrier(comm1d);
		MPI_Sendrecv(&df1[e][0][n3-2][7],1,newtype_bt,nbright,77,

		           &df1[s-1][0][n3-2][7],1,newtype_bt,nbleft,77,comm1d,&status);
		// MPI_Barrier(comm1d);


		MPI_Sendrecv(&df1[e][2][0][15],1,newtype_fr,nbright,1515,

		           &df1[s-1][2][0][15],1,newtype_fr,nbleft,1515,comm1d,&status);
		// MPI_Barrier(comm1d);
		MPI_Sendrecv(&df1[e][n2-2][0][17],1,newtype_fr,nbright,1717,

		           &df1[s-1][n2-2][0][17],1,newtype_fr,nbleft,1717,comm1d,&status);
		// MPI_Barrier(comm1d);






		MPI_Sendrecv(&df1[s][0][2][8],1,newtype_bt,nbleft,88,

		           &df1[e+1][0][2][8],1,newtype_bt,nbright,88,comm1d,&status);
		// MPI_Barrier(comm1d);
		MPI_Sendrecv(&df1[s][0][n3-2][10],1,newtype_bt,nbleft,1010,

		           &df1[e+1][0][n3-2][10],1,newtype_bt,nbright,1010,comm1d,&status);
		// MPI_Barrier(comm1d);


		MPI_Sendrecv(&df1[s][2][0][18],1,newtype_fr,nbleft,1818,

		           &df1[e+1][2][0][18],1,newtype_fr,nbright,1818,comm1d,&status);
		// MPI_Barrier(comm1d);
		MPI_Sendrecv(&df1[s][n2-2][0][16],1,newtype_fr,nbleft,1616,

		           &df1[e+1][n2-2][0][16],1,newtype_fr,nbright,1616,comm1d,&status);
		// MPI_Barrier(comm1d);

		#ifdef DEBUG_PRINT
		printf("Comp_Proc%d: LBM finish MPI_Sendrecv!\n",gv->rank[0]);
		fflush(stdout);
		#endif //DEBUG_PRINT

		if (myid==left_most) s=3;

		if (myid==right_most) e=n1-3;

		// MPI_Barrier(comm1d);

		/* boundary conditions */



		/* 1.1 bounce back and reflection condition on the bottom */

		k=2;

		/*for (i=3;i<=n1-3;i++) */

		for (i=s;i<=e;i++)

		   for (j=2;j<=n2-2;j++)

		{

		df2[i][j][k][3]=df1[i][j][k][4];

		df2[i][j][k][7]=p_bb*df1[i][j][k][8]+(1.0-p_bb)*df1[i-1][j][k][9];

		df2[i][j][k][10]=p_bb*df1[i][j][k][9]+(1.0-p_bb)*df1[i+1][j][k][8];

		df2[i][j][k][11]=p_bb*df1[i][j][k][12]+(1.0-p_bb)*df1[i][j+1][k][14];

		df2[i][j][k][13]=p_bb*df1[i][j][k][14]+(1.0-p_bb)*df1[i][j-1][k][12];

		}



		/* 1.2 bounce back and reflection condition on the top*/

		k=n3-2;

		/* for (i=3;i<=n1-3;i++)*/

		for (i=s;i<=e;i++)

		   for (j=2;j<=n2-2;j++)

		{

		df2[i][j][k][4]=df1[i][j][k][3];

		df2[i][j][k][8]=p_bb*df1[i][j][k][7]+(1.0-p_bb)*df1[i+1][j][k][10];

		df2[i][j][k][9]=p_bb*df1[i][j][k][10]+(1.0-p_bb)*df1[i-1][j][k][7];

		df2[i][j][k][12]=p_bb*df1[i][j][k][11]+(1.0-p_bb)*df1[i][j-1][k][13];

		df2[i][j][k][14]=p_bb*df1[i][j][k][13]+(1.0-p_bb)*df1[i][j+1][k][11];

		}



		/* 1.3 bounce back and reflection condition on the front*/

		j=2;

		/*for (i=3;i<=n1-3;i++) */

		for (i=s;i<=e;i++)

		   for (k=2;k<=n3-2;k++)

		  {

		  df2[i][j][k][6]=df1[i][j][k][5];

		        df2[i][j][k][12]=p_bb*df1[i][j][k][11]+(1.0-p_bb)*df1[i][j][k+1][14];

		  df2[i][j][k][13]=p_bb*df1[i][j][k][14]+(1.0-p_bb)*df1[i][j][k-1][11];

		  df2[i][j][k][16]=p_bb*df1[i][j][k][15]+(1.0-p_bb)*df1[i+1][j][k][18];

		  df2[i][j][k][17]=p_bb*df1[i][j][k][18]+(1.0-p_bb)*df1[i-1][j][k][15];

		  }





		/* 1.4 bounce back and reflection condition on the rear*/

		j=n2-2;

		/*for (i=3;i<=n1-3;i++)*/

		for (i=s;i<=e;i++)

		   for (k=2;k<=n3-2;k++)

		  {

		  df2[i][j][k][5]=df1[i][j][k][6];

		        df2[i][j][k][11]=p_bb*df1[i][j][k][12]+(1.0-p_bb)*df1[i][j][k-1][13];

		  df2[i][j][k][14]=p_bb*df1[i][j][k][13]+(1.0-p_bb)*df1[i][j][k+1][12];

		  df2[i][j][k][15]=p_bb*df1[i][j][k][16]+(1.0-p_bb)*df1[i-1][j][k][17];

		  df2[i][j][k][18]=p_bb*df1[i][j][k][17]+(1.0-p_bb)*df1[i+1][j][k][16];

		  }



		s=2;e=n1-2;

		// MPI_Barrier(comm1d);




		if(myid==left_most) s=3;

		if(myid==right_most) e=n1-3;

		// MPI_Barrier(comm1d);




		/* compute rho and (u,v) from distribution function */

		/* for (i=3;i<=n1-3;i++) */

		for (i=s;i<=e;i++)

		   for (j=2;j<=n2-2;j++)

		      for (k=2;k<=n3-2;k++)

		      {

		      s1=df2[i][j][k][0];

		      s2=c[0][0]*df2[i][j][k][0];

		      s3=c[0][1]*df2[i][j][k][0];

		      s4=c[0][2]*df2[i][j][k][0];



		      for (m=1;m<=18;m++)

		      {

		      s1+=df2[i][j][k][m];

		      s2+=c[m][0]*df2[i][j][k][m];

		      s3+=c[m][1]*df2[i][j][k][m];

		      s4+=c[m][2]*df2[i][j][k][m];

		      }



		          rho[i][j][k]=s1;

		          u[i][j][k]=s2/s1;

		          v[i][j][k]=s3/s1;

		      w[i][j][k]=s4/s1;

		      }





		if (myid==left_most) {

		/* 2. inlet and outlet conditions */

		/* inlet */

		i=2;

		for (j=1;j<=n2-1;j++)

		   for (k=1;k<=n3-1;k++)

		      for (m=0;m<=18;m++)

		  {

		   df2[i][j][k][m]=df_inout[0][j][k][m];

		      }

		}



		if (myid==right_most) {

		/* outlet */

		i=n1-2;

		for (j=1;j<=n2-1;j++)

		   for (k=1;k<=n3-1;k++)

		      for (m=0;m<=18;m++)

		{

		df2[i][j][k][m]=df_inout[1][j][k][m];

		}

		}



		for (i=s;i<=e;i++)

		   for (k=2;k<=n3-2;k++)

		      for (m=0;m<=18;m++)

		{df2[i][1][k][m]=df2[i][n2-3][k][m];

		 df2[i][n2-1][k][m]=df2[i][3][k][m];}





		/* along z-direction, z=1 & n3-1 */

		/*for (i=3;i<=n1-3;i++)*/

		for (i=s;i<=e;i++)

		   for (j=2;j<=n2-2;j++)

		      for (m=0;m<=18;m++)

		  {df2[i][j][1][m]=df2[i][j][n3-3][m];

		         df2[i][j][n3-1][m]=df2[i][j][3][m];}



		/* replacing the old d.f. values by the newly compuited ones */

		if (myid==left_most) s=1;

		if(myid==right_most) e=n1-1;

		/* for (i=1;i<=n1-1;i++) */

		for (i=s;i<=e;i++)

		   for (j=1;j<=n2-1;j++)

		      for (k=1;k<=n3-1;k++)

		         for (m=0;m<=18;m++)

		          {df1[i][j][k][m]=df2[i][j][k][m];}

		t6=get_cur_time();
		only_lbm_time+=t6-t5;

		#ifdef DEBUG_PRINT
		printf("Comp_Proc%d: Start create_prb_element!\n",gv->rank[0]);
		fflush(stdout);
		#endif //DEBUG_PRINT

		/*create_prb_element*/
		// char * buffer;
		// FILE *fp;
		// char file_name[64];
		int count=0;


		// cubex=4;
		// cubey=4;
		// cubez=4;

		// X=nx/cubex;
		// Y=ny/cubey;
		// Z=nz/cubez;

		// #ifdef DEBUG_PRINT
		// printf("Compute Node %d Generator start create_prb_element!\n",gv->rank[0]);
		// fflush(stdout);
		// #endif //DEBUG_PRINT

		for(gv->CI=0;gv->CI<gv->X;gv->CI++)
		  for(gv->CJ=0;gv->CJ<gv->Y;gv->CJ++)
		    for(gv->CK=0;gv->CK<gv->Z;gv->CK++){
		      gv->originx=gv->CI*gv->cubex;
		      gv->originy=gv->CJ*gv->cubey;
		      gv->originz=gv->CK*gv->cubez;
		      //
		      //pthread_mutex_lock(&gv->lock_generator);
		      //last_gen = gv->data_id++;
		      //pthread_mutex_unlock(&gv->lock_generator);

		      // if(last_gen>=gv->cpt_total_blks) {
		      //   gv->compute_all_done = 1;
		      //   break;
		      // }

		      buffer = (char*) malloc(sizeof(char)*gv->compute_data_len);
		      check_malloc(buffer);

		      ((int *)buffer)[0] = gv->data_id++;
		      ((int *)buffer)[1]=gv->step;
		      ((int *)buffer)[2]=gv->CI;
		      ((int *)buffer)[3]=gv->CJ;
		      ((int *)buffer)[4]=gv->CK;
		      //printf("Node %d put to pr_rb step%d i%d j%d k%d\n", gv->rank[0], *(int *)(buffer),*(int *)(buffer+4),*(int *)(buffer+8),*(int *)(buffer+12));
		      count=0;
		      for(i=0;i<gv->cubex;i++)
		        for(j=0;j<gv->cubey;j++)
		          for(k=0;k<gv->cubez;k++){
		            gv->gi=gv->originx+i;
		            gv->gj=gv->originy+j;
		            gv->gk=gv->originz+k;

		            ((double *)(buffer+sizeof(int)*5))[count]	=u_r*u[gv->gi][gv->gj][gv->gk];
		            ((double *)(buffer+sizeof(int)*5))[count+1]	=u_r*v[gv->gi][gv->gj][gv->gk];
		            // *((double *)(buffer+count+2))=gi;
		            // *((double *)(buffer+count+3))=gj;
		            // *((double *)(buffer+count+4))=gk;
		            count+=2;
		          }
		      //total produce X*Y*Z blocks each step

		      producer_ring_buffer_put(gv,buffer);
		      // sprintf(file_name,"/N/dc2/scratch/fuyuan/inter/id%d_v&u_step%03d_blk_k%04d_j%04d_i%04d.data",myid,step,CI,CJ,CK);
		      // fp=fopen(file_name,"wb");
		      // fwrite(buffer, count, 1, fp);
		      // fclose(fp);
		    }
		//free(buffer);
		#ifdef DEBUG_PRINT
		if(gv->step%10==0)
		  printf("Comp_Proc%d: LBM gv->step = %d\n", gv->rank[0], gv->step);
		#endif //DEBUG_PRINT
		time1=0;

		gv->step+=dt;

		time1+=dt;

		time2+=dt;

		time3+=dt;


		// MPI_Barrier(comm1d);



		}  /* end of while loop */

		// generate exit message
		buffer = (char*) malloc(sizeof(char)*gv->compute_data_len);
    	check_malloc(buffer);

    	((int *)buffer)[0]= EXIT_BLK_ID;
	    // ((int *)buffer)[1]= -1;
	    // ((int *)buffer)[2]= -1;

	    printf("Comp_Proc%d: LBM generate the EXIT block_id=%d in timestep=%d with total_blks %d\n",
	      gv->rank[0], ((int *)buffer)[0], gv->step, gv->data_id);
	    fflush(stdout);

	    producer_ring_buffer_put(gv,buffer);


		// MPI_Barrier(comm1d);
		t3= get_cur_time();

		MPI_Comm_free(&comm1d);
		MPI_Type_free(&newtype);
		MPI_Type_free(&newtype_bt);
		MPI_Type_free(&newtype_fr);

		// gv->compute_all_done = 1;
		// printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
		printf("Comp_Proc%d on %s: LBM computation Done! Only_LBM_Time=%.3f, T_total_compute_Time/Block=%.3fus, T_total_compute=%.3f\n",
		gv->rank[0], gv->processor_name, only_lbm_time, (t3-t2)*1000000/gv->cpt_total_blks, t3-t2);
		fflush(stdout);
		//------------------------------------------------END OF LBM--------------------------------------------------------

		/* Join threads */
		for(i = 0; i < (gv->compute_writer_num+1); i++) {
		  pthread_join(thrds[i], &retval);
		  // printf("Compute Node %d Thread %d is finished\n", gv->rank[0] ,i);
		}

		t1=get_cur_time();
		free(lvs);
		free(attrs);
		free(thrds);

		printf("Comp_Proc%d: Job finish on %s, T_total=%.3f\n", gv->rank[0], gv->processor_name, t1-t0);
		fflush(stdout);
	}
	else if (gv->color == 1){
		//ana node
		gv->ana_total_blks = gv->computer_group_size * gv->cpt_total_blks;

		gv->reader_blk_num = gv->computer_group_size * gv->writer_blk_num;
		gv->analysis_writer_blk_num = gv->ana_total_blks - gv->reader_blk_num;

		lvs   = (LV) malloc(sizeof(*lvs)*(gv->analysis_reader_num+gv->analysis_writer_num+1+1));
		thrds = (pthread_t*) malloc(sizeof(pthread_t)*(gv->analysis_reader_num+gv->analysis_writer_num+1+1));
		attrs = (pthread_attr_t*) malloc(sizeof(pthread_attr_t)*(gv->analysis_reader_num+gv->analysis_writer_num+1+1));

		gv->all_lvs = lvs;

		//Consumer_ring_buffer initialize
	    consumer_rb.bufsize = CONSUMER_RINGBUFFER_TOTAL_MEMORY/(gv->block_size-4*sizeof(int));
	    consumer_rb.head = 0;
	    consumer_rb.tail = 0;
	    consumer_rb.num_avail_elements = 0;
	    consumer_rb.buffer = (char**)malloc(sizeof(char*)*consumer_rb.bufsize);
	    consumer_rb.lock_ringbuffer = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
	    consumer_rb.full = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
	    consumer_rb.empty = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
	    pthread_mutex_init(consumer_rb.lock_ringbuffer, NULL);
	    pthread_cond_init(consumer_rb.full, NULL);
	    pthread_cond_init(consumer_rb.empty, NULL);
	    gv->consumer_rb_p = &consumer_rb;
	    //gv->consumer_rb_p = rb_init(gv,CONSUMER_RINGBUFFER_TOTAL_MEMORY,&consumer_rb);
	    if(gv->rank[0]==gv->compute_process_num || gv->rank[0]==(gv->compute_process_num+gv->analysis_process_num-1)){
	    	printf("Ana_Proc%d of %d ana_total_blks = %d, reader_blk_num=%d, analysis_writer_blk_num=%d, \
CONSUMER_Ringbuffer %.3fGB, size=%d member\n",
	    		gv->rank[0], gv->size[1], gv->ana_total_blks, gv->reader_blk_num, gv->analysis_writer_blk_num,
	    		CONSUMER_RINGBUFFER_TOTAL_MEMORY/(1024.0*1024.0*1024.0),gv->consumer_rb_p->bufsize);
		    fflush(stdout);
	    }


		gv->mpi_recv_progress_counter = 0;
	    gv->org_recv_buffer = (char *) malloc(gv->compute_data_len); //long message+
	    check_malloc(gv->org_recv_buffer);

	    // prfetch threads 1+1:cid+blkid
	    gv->ana_reader_done=0;
	    gv->prefetch_id_array = (int *) malloc(sizeof(int)*(1+1)*gv->reader_blk_num);
	    gv->recv_tail = 0;
	    check_malloc(gv->prefetch_id_array);

	    //initialize lock
	    pthread_mutex_init(&gv->lock_recv,NULL);
	    // pthread_mutex_init(&gv->lock_prefetcher_progress, NULL);

	    t2=get_cur_time();
		/* Create threads */
		for(i = 0; i < (gv->analysis_reader_num+gv->analysis_writer_num+1+1); i++) {
		  init_lv(lvs+i, i, gv);
		  if(pthread_attr_init(attrs+i)) perror("attr_init()");
		  if(pthread_attr_setscope(attrs+i, PTHREAD_SCOPE_SYSTEM)) perror("attr_setscope()");
		  if(pthread_create(thrds+i, attrs+i, analysis_node_do_thread, lvs+i)) {
		    perror("pthread_create()");
		    exit(1);
		  }
		}

		/* Join threads */
		for(i = 0; i < (gv->analysis_reader_num+gv->analysis_writer_num+1+1); i++) {
		  pthread_join(thrds[i], &retval);
		  // printf("Analysis Node %d Thread %d is finished\n", gv->rank[0], i);
		}

		free(lvs);
		free(attrs);
		free(thrds);

		t3=get_cur_time();
		printf("Ana_Proc%d on %s: Analysis Job Done! T_total=%.3f\n",gv->rank[0], gv->processor_name,t3-t2);
	}
	else{
		printf("Error!\n");
	}

	MPI_Comm_free(&mycomm);
	MPI_Finalize();

	free(gv);

	return 0;
}

