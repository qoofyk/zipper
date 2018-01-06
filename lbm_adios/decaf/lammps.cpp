//---------------------------------------------------------------------------
//
// lammps example
//
// 4-node workflow
//
//          print (1 proc)
//        /
//    lammps (4 procs)
//        \
//          print2 (1 proc) - print (1 proc)
//
//  entire workflow takes 10 procs (1 dataflow proc between each producer consumer pair)
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// tpeterka@mcs.anl.gov
//
//--------------------------------------------------------------------------
#include <decaf/decaf.hpp>
#include <bredala/data_model/pconstructtype.h>
#include <bredala/data_model/vectorfield.hpp>
#include <bredala/data_model/boost_macros.h>

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
#include "run_msd.h"
#define SIZE_ONE (5)
#define NSTEPS (100)

using namespace decaf;
using namespace LAMMPS_NS;
using namespace std;

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
void prod(Decaf* decaf, string infile)
{

    int nsteps = NSTEPS;
    int rank;
    int line;

	/*those are all the information io libaray need to know about*/
    MPI_Comm comm;
	int nlocal; //nlines processed by each process
	int size_one = SIZE_ONE; // each line stores 2 doubles
	double *buffer; // buffer address
    double **x;// all the atom values

    LAMMPS* lps = new LAMMPS(0, NULL, decaf->prod_comm_handle());
    lps->input->file(infile.c_str());
    printf("prod lammps_decaf started with input %s\n", infile.c_str() );

    rank = decaf->prod_comm()->rank();


    double t_start = MPI_Wtime();
    for (int timestep = 0; timestep < nsteps; timestep++)
    {

        lps->input->one("run 1");
        //int natoms = static_cast<int>(lps->atom->natoms);
        //lammps_gather_atoms(lps, (char*)"x", 1, 3, x);

        //extract "value"
        x = (double **)(lammps_extract_atom(lps,(char *)"x"));
        nlocal = static_cast<int>(lps->atom->nlocal); // get the num of lines this rank have
        if(x == NULL){
            fprintf(stderr, "extract failed\n");
            break;
        }

        buffer = new double[size_one * nlocal];

        printf("step %d i have %d lines\n",timestep, nlocal);
        for(line = 0; line < nlocal; line++){
            buffer[line*size_one] = line;
            buffer[line*size_one+1] = 1;
            buffer[line*size_one+2] = x[line][0];
            buffer[line*size_one+3] = x[line][1];
            buffer[line*size_one+4] = x[line][2];
        }

        /* decaf put */
        if (1)
        {
            pConstructData container;

            // lammps gathered all positions to rank 0
            //if (decaf->prod_comm()->rank() == 0)
            if (rank == 0)
            {
                fprintf(stderr, "lammps producing time step %d with %d lines\n",
                        timestep, nlocal);

            }
                // debug
                //         for (int i = 0; i < 10; i++)         // print first few atoms
                //           fprintf(stderr, "%.3lf %.3lf %.3lf\n",
                // x[3 * i], x[3 * i + 1], x[3 * i + 2]);

            VectorFliedd data(buffer, size_one * nlocal, size_one);

            container->appendData("pos", data,
                                      DECAF_NOFLAG, DECAF_PRIVATE,
                                      DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);
            /*else*/
            //{
                //vector<double> pos;
                //VectorFliedd data(pos, 3);
                //container->appendData("pos", data,
                                      //DECAF_NOFLAG, DECAF_PRIVATE,
                                      //DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);
            /*}*/

            decaf->put(container);
        }

       free(buffer);
    }

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    //
    double t_end = MPI_Wtime();
    printf("total-start-end %.3f %.3f %.3f\n", t_end- t_start, t_start, t_end);

    fprintf(stderr, "lammps terminating\n");
    decaf->terminate();

    delete lps;
}

// gets the atom positions and prints them
void con(Decaf* decaf)
{
    double global_t_analy = 0, t_analy = 0;
    double t1, t2;
    int slice_size = 0;
    double *buffer;
    MPI_Comm comm;
    int rank;

    comm = decaf->con_comm_handle();
    rank = decaf->con_comm()->rank();

    vector< pConstructData > in_data;

    int step = 0;


    if(rank == 0){
        printf("[msd]: application starts\n");
    }


    /* msd required*/
    double **msd;
    int nsteps = NSTEPS;
    int timestep = 0;
    int size_one = SIZE_ONE;
    msd =  init_msd(nsteps, size_one);

    //MPI_Barrier(comm);
    double t_start = MPI_Wtime();
    while (decaf->get(in_data))
    {
        // get the values
        for (size_t i = 0; i < in_data.size(); i++)
        {
            VectorFliedd pos = in_data[i]->getFieldData<VectorFliedd>("pos");
            if (pos)
            {
                // debug
                slice_size = pos.getNbItems();

                fprintf(stderr, "[msd]: consumer processing %d atoms at step %d\n",
                        slice_size,
                        step);


                buffer = &pos.getVector()[0];

                t1 =MPI_Wtime(); 
                calc_msd(msd, buffer, slice_size, size_one, timestep);

                //run_analysis(buffer, slice_size, lp, sum_vx,sum_vy);

                t2 =MPI_Wtime(); 
                t_analy += t2-t1;

            }
            else
                fprintf(stderr, "[msd]: Error: null pointer in node2\n");
        }
        timestep+=1;

        //printf("[msd]: Step %d,t_analy %lf\n", step, t2-t1);

        step +=1;
    }

    perform_msd_reduce(msd, nsteps, comm);
    free_msd(msd, size_one);

   printf("[msd]: t_analy %lf\n", t_analy);
    //MPI_Barrier(comm);
    double t_end = MPI_Wtime();

    MPI_Reduce(&t_analy, &global_t_analy, 1, MPI_DOUBLE, MPI_MAX, 0, comm);
    if(rank == 0){
		printf("[msd]: max t_analysis %.3f s\n", global_t_analy);
	}
    printf("[msd]: total-start-end %.3f %.3f %.3f\n", t_end- t_start, t_start, t_end);

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    fprintf(stderr, "[msd]: terminating\n");

    decaf->terminate();
}

// forwards the atom positions in this example
// in a more realistic example, could filter them and only forward some subset of them
void print2(Decaf* decaf)
{
    vector< pConstructData > in_data;

    while (decaf->get(in_data))
    {

        // get the values and add them
        for (size_t i = 0; i < in_data.size(); i++)
        {
            fprintf(stderr, "print2 forwarding positions\n");
            decaf->put(in_data[i]);
        }
    }

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    fprintf(stderr, "print2 terminating\n");
    decaf->terminate();
}

extern "C"
{
    // dataflow just forwards everything that comes its way in this example
    void dflow(void* args,                          // arguments to the callback
               Dataflow* dataflow,                  // dataflow
               pConstructData in_data)   // input data
    {
        dataflow->put(in_data, DECAF_LINK);
    }
} // extern "C"

void run(Workflow& workflow,              // workflow
         string infile)                      // lammps input config file*/
{
    MPI_Init(NULL, NULL);
    Decaf* decaf = new Decaf(MPI_COMM_WORLD, workflow);

    // run workflow node tasks
    // decaf simply tells the user whether this rank belongs to a workflow node
    // how the tasks are called is entirely up to the user
    // e.g., if they overlap in rank, it is up to the user to call them in an order that makes
    // sense (threaded, alternting, etc.)
    // also, the user can define any function signature she wants
    if (decaf->my_node("prod"))
        prod(decaf, infile);
    if (decaf->my_node("con"))
        con(decaf);
    if (decaf->my_node("print2"))
        print2(decaf);

    // MPI_Barrier(MPI_COMM_WORLD);

    // cleanup
    delete decaf;
    MPI_Finalize();
}

// test driver for debugging purposes
// normal entry point is run(), called by python
int main(int argc,
         char** argv)
{
    printf("main function launched\n");
    Workflow workflow;
    Workflow::make_wflow_from_json(workflow, "lammps.json");

    // run decaf
char * prefix         = getenv("DECAF_PREFIX");
    if (prefix == NULL)
    {
        fprintf(stderr, "ERROR: environment variable DECAF_PREFIX not defined. Please export "
                "DECAF_PREFIX to point to the root of your decaf install directory.\n");
        exit(1);
    }
    string infile = argv[1];

    run(workflow, infile);

           
    return 0;
}
