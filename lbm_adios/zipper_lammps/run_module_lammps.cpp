/*
 * lammps zipper driver
 * Yuankun Fu Jan 2018
 */

#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <string.h>
#include <utility>
#include <map>

// lammps includes
#include "lammps.h"
#include "input.h"
#include "atom.h"
#include "library.h"
#include "stdlib.h"
//#include "run_msd.h"


#include "run_module_lammps.h"


using namespace LAMMPS_NS;
using namespace std;

#ifdef V_T
#include <VT.h>
int class_id;
int advance_step_id;// get_buffer_id;
#endif

struct lammps_args_t                         // custom args for running lammps
{
    LAMMPS* lammps;
    string infile;
};

struct pos_args_t                            // custom args for atom positions
{
    int natoms;                              // number of atoms
    double* pos;                             // atom positions
};

// runs lammps and puts the atom positions to the dataflow at the consumer intervals
//void lammps(Decaf* decaf, int nsteps, int analysis_interval, string infile)
//void prod(Decaf* decaf, string infile)
status_t run_module_lammps (int argc, char *argv[], GV gv, MPI_Comm *pcomm)
{

    int nsteps;
    int rank;
    int line;

	/*those are all the information io libaray need to know about*/
  MPI_Comm comm = *pcomm;
	int nlocal; //nlines processed by each process
	int size_one = SIZE_ONE; // each line stores 2 doubles
  double **x;// all the atom values

    double t1, t2, t3;

    // MPI_Init(&argc, &argv);
    // comm = MPI_COMM_WORLD;

    string infile = argv[2];
    nsteps = atoi(argv[1]);

    double t_start = MPI_Wtime();

    LAMMPS* lps = new LAMMPS(0, NULL, comm);
    lps->input->file(infile.c_str());
    printf("prod lammps_sim_only started with input %s\n", infile.c_str() );

#ifdef V_T
      VT_classdef( "Computation", &class_id );
      VT_funcdef("PUT", class_id, &advance_step_id);
      //VT_funcdef("GETBUF", class_id, &get_buffer_id);
#endif

    MPI_Comm_rank (comm, &rank);
    //MPI_Comm_size (comm, &nprocs);


    for (int timestep = 0; timestep < nsteps; timestep++)
    {

        t1 = MPI_Wtime();
        lps->input->one("run 1 pre no post no"); // do not initialize each time, this is recommanded from lammps doc for coupled simulation using lammps library

        t2 = MPI_Wtime();
        int natoms = static_cast<int>(lps->atom->natoms);
        //lammps_gather_atoms(lps, (char*)"x", 1, 3, x);

        //extract "value"
        x = (double **)(lammps_extract_atom(lps,(char *)"x"));
        t3 = MPI_Wtime();
        nlocal = static_cast<int>(lps->atom->nlocal); // get the num of lines this rank have
        if(x == NULL){
            fprintf(stderr, "extract failed\n");
            break;
        }

        if(rank == 0){
        printf("step %d i have %d lines, sim time %.3f extract time %.3f\n",
                timestep,
                nlocal,
                t2-t1,
                t3-t2);
        }

        insert_zipper(gv, x, nlocal,timestep);


    }

    generate_exit_msg(gv);

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    //
    double t_end = MPI_Wtime();
    printf("total-start-end %.3f %.3f %.3f\n", t_end- t_start, t_start, t_end);


    delete lps;

    // MPI_Finalize();
    //   if(rank == 0)
    //     printf("[lammps]: terminating! \n");
      return 0;

}




