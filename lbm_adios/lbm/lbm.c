#include "lbm.h"        

#ifndef STRING_LENGTH
#define STRING_LENGTH (160)
#endif


#define debug
//#define DEBUG_PRINT

#ifndef filesize2produce
#define filesize2produce (256)
#endif

// global variables
int gi, gj, gk, nx,ny,nz;
int n;
MPI_Status status;


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

/*
 * added by Feng 
 * 
*/
#define nx (filesize2produce/4)
#define ny (filesize2produce/4)
#define nz (filesize2produce)

double df1[nx][ny][nz][19],df2[nx][ny][nz][19],df_inout[2][ny][nz][19];
double rho[nx][ny][nz],u[nx][ny][nz],v[nx][ny][nz],w[nx][ny][nz];

double t7 = 0, t8 =0;

int step; // current step
int step_stop; //run how many steps
// timer
double t_buffer = 0;

int         rank, nprocs;
    

status_t lbm_init(MPI_Comm *pcomm, size_t size_one, double **pbuff){
        MPI_Comm comm = *pcomm;
        n = nx*ny*nz;

		/* alloc io buffer */
		*pbuff = (double *)malloc(n*sizeof(double)*size_one);
		if(NULL == *pbuff) return S_FAIL;
		double *buffer = *pbuff;

		if(rank == 0){
			printf("[LBM INFO]: io buffer allocated\n");
		}

		/* from this line it's just the origin code */
		n1=nx-1;/* n1,n2,n3 are the last indice in arrays*/
		n2=ny-1;
		n3=nz-1;

		num_data=ny*nz;/* number of data to be passed from process to process */
		//num_data1=num_data*19;
		
		np[0]=nprocs;period[0]=0;
		fp_np=np;
		fp_period=period;

		MPI_Type_vector(num_data,1,19,MPI_DOUBLE,&newtype);
		MPI_Type_commit(&newtype);

		MPI_Type_vector(ny,1,nz*19,MPI_DOUBLE,&newtype_bt);
		MPI_Type_commit(&newtype_bt);

		MPI_Type_vector(nz,1,19,MPI_DOUBLE,&newtype_fr);
		MPI_Type_commit(&newtype_fr);


		errorcode=MPI_Cart_create(comm,1,fp_np,fp_period,1,&comm1d);
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

		

		//---------------------------------------------BEGIN LBM -----------------------------------------------
		#ifdef DEBUG_PRINT
		printf("Compute Node %d Generator begin LBM!\n",rank);
		fflush(stdout);
		#endif //DEBUG_PRINT

		t2 = MPI_Wtime();

		// x_mid=1+(nx-4)/2;

		// y_mid=1+(ny-4)/2;

		// z_mid=1+(nz-4)/2;

		left_most=0;right_most=nprocs-1;middle=nprocs/2;

		// if( ((float)nprocs-(float)middle*2.0) < 1.0e-3) x_mid=2;

		s=2; /* the array for each process is df?[0:n1][][][], the actual part is [s:e][][][] */

		e=n1-2;



		p_bb=0.03;

        //Yuankun Modify, fluid grid x:y:z=1:1:4, 64*64*256
        depth=25.6e-5; /*256 microns */
        width=depth/4;  /* unit: cm i.e. 64 micron*/
        length=width*nprocs; //gv->size[1] = num_compute_process


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


		// time_restart=400000;

		time1=dt;

		time2=dt;

		time3=dt;

		step=0;

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

		fprintf(fp_out,"the total step run is %d steps\n", step_stop);

		fclose(fp_out);

		}


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


		t4=MPI_Wtime();
		init_lbm_time=t4-t2;
		only_lbm_time+=t4-t2;
		if(myid==0){
			printf("Init LBM Time=%f\n", init_lbm_time);
			fflush(stdout);
		}
        return S_OK;

}


status_t lbm_advance_step(MPI_Comm * pcomm, double *buffer){

//void run_lbm(char * filepath, int step_stop, int dims_cube[3], MPI_Comm *pcomm)


   
        // original code
		//double df1[nx][ny][nz][19],df2[nx][ny][nz][19],df_inout[2][ny][nz][19];
		//double rho[nx][ny][nz],u[nx][ny][nz],v[nx][ny][nz],w[nx][ny][nz];



		//int blk_id=0;

		// mark advance_step

		//while (step < step_stop)

		//{

		t5=MPI_Wtime();

		if (myid==0){
			printf("step = %d   of   %d,rank %d nprocs %d   \n", step, step_stop, rank, nprocs);
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
		printf("Compute Node %d Generator start LBM data sending and receiving !\n", rank);
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
		printf("Compute Node %d Generator finish LBM data sending and receiving !\n",rank);
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
#if 0

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
#endif





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


		t6=MPI_Wtime();
		only_lbm_time+=t6-t5;



		#ifdef DEBUG_PRINT
        printf("Compute Node %d Generator start create_prb_element!\n",rank);
        fflush(stdout);
		#endif //DEBUG_PRINT

		/*create_prb_element*/
		//double * buffer;
		// FILE *fp;
		// char file_name[64];
		int count=0;

        // if use adios, not partioning is required
        //buffer = (double *) malloc(n*sizeof(double)*2);
        //check_malloc(buffer);

        // fill the buffer, each line has two double data
        for(gi = 0; gi < nx; gi++){
            for(gj = 0; gj < ny; gj++){
                for(gk = 0; gk < nz; gk++){
		            *((double *)(buffer+count))=u_r*u[gi][gj][gk];
		            *((double *)(buffer+count+1))=u_r*v[gi][gj][gk];
#ifdef debug_1
                    printf("(%d %d %d), u_r=_%lf u=%lf v= %lf\n", gi, gj, gk, u_r, u[gi][gj][gk],v[gi][gj][gk]);
#endif
		            count+=2;
                }
            }
        }

        t7 = MPI_Wtime();
		t_buffer+=t7-t6;


		
		#ifdef DEBUG_PRINT
		if(step%10==0)
		  printf("Node %d step = %d\n", rank, step);
		#endif //DEBUG_PRINT
		time1=0;

		step+=dt;

		time1+=dt;

		time2+=dt;

		time3+=dt;


		// MPI_Barrier(comm1d);



		//}  /* end of while loop */

        return S_OK;
}


status_t lbm_finalize(MPI_Comm *pcomm, double *buffer){
    MPI_Comm comm = *pcomm;

	// from "end of while loop in original code"
	printf("[rank %d]:sim_time %.3lf \n", rank, only_lbm_time);

	double global_t_cal=0;
	double global_t_write=0;
	MPI_Reduce(&only_lbm_time, &global_t_cal, 1, MPI_DOUBLE, MPI_MAX, 0, comm);

	if(rank == 0){
		//printf("t_prepare:%f s, t_cal %f s,t_buffer = %f, t_write %f s, t_put %f s\n", rank,init_lbm_time, only_lbm_time,t_buffer, t_write, t_write_2);
		printf("t_prepare:%f s, max t_cal %f s\n", init_lbm_time, global_t_cal);
	}

	// MPI_Barrier(comm1d);
	t3= MPI_Wtime();

	MPI_Comm_free(&comm1d);
	MPI_Type_free(&newtype);
	MPI_Type_free(&newtype_bt);
	MPI_Type_free(&newtype_fr);

	if(buffer){
		free(buffer);
		//printf("io buffer freed\n");
	}

    return S_OK;
}

/*
 * test driver for lbm code
 */
int main(int argc, char * argv[]){

    /*
     * @input
     * @param NSTOP
     * @param FILESIZE2PRODUCE
     */
    if(argc !=2){
        printf("run_lbm step_stop\n");
        exit(-1);
    }
	int i;

	/*those are all the information io libaray need to know about*/
	int nlocal; //nlines processed by each process
	int size_one = SIZE_ONE; // each line stores 2 doubles
	double *buffer; // buffer address

    step_stop = atoi(argv[1]);
    
    int dims_cube[3] = {filesize2produce/4,filesize2produce/4,filesize2produce};
    //strcpy(filepath, argv[3]);

	MPI_Init(&argc, &argv);

    MPI_Comm comm = MPI_COMM_WORLD;
    //int         rank, nprocs; // now is global
    MPI_Comm_rank (comm, &rank);
    MPI_Comm_size (comm, &nprocs);
char nodename[256];
    int nodename_length;
	MPI_Get_processor_name(nodename, &nodename_length );

	nlocal = dims_cube[0]*dims_cube[1]*dims_cube[2];
	
	/* init lbm with dimension info*/
	if( S_FAIL == lbm_init(&comm, size_one, &buffer)){
		printf("[lbm]: init not success, now exit\n");
		goto cleanup;
	}


	if(rank == 0){
		printf("[lbm]: init with nlocal = %d size_one = %d\n", nlocal, size_one);
	}
	for(i = 0; i< step_stop; i++){
		if(S_OK != lbm_advance_step(&comm, buffer)){
			fprintf(stderr, "[lbm]: err when process step %d\n", i);
		}

		// replace this line with different i/o libary
		if(S_OK != lbm_io_template(&comm, buffer, nlocal, size_one)){
			fprintf(stderr,"[lbm]: error when writing step %d \n", i);
		}
	}

	if(S_OK != lbm_finalize(&comm, buffer)){
		fprintf(stderr, "[lbm]: err when finalized\n");
	}

	  //run_lbm(filepath, step_stop, dims_cube, &comm);
    MPI_Barrier(comm);
    //printf("[lbm]: reached the barrier\n");

cleanup:
  MPI_Finalize();
  if(rank == 0)
    printf("exit\n", rank);
  return 0;
}

