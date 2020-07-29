/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2011-2018 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Application
    simpleFoam

Description
    Steady-state solver for incompressible, turbulent flow, using the SIMPLE
    algorithm.

\*---------------------------------------------------------------------------*/

// /applications/solvers/incompressible/simpleFoam/

// #define DEBUG
#include "common/logging.h"
#include "common/utility.h"

#ifdef METHOD_CLOUD
#include "c-clients/elastic_broker.h"
#endif
#include <vector>
#include "mpi.h"


#include "fvCFD.H"
#include "singlePhaseTransportModel.H"
#include "turbulentTransportModel.H"
#include "simpleControl.H"
#include "fvOptions.H"

#define FOAM1906

//149.165.169.12 149.165.170.58 149.165.168.217 149.165.168.115 149.165.168.245

struct{
  const char * hostname = "149.165.170.58";
  int port = 6379;
  bool isunix = 0; // use unix socket or tcp
} cloud_config;

int main(int argc, char *argv[])
{
    argList::addNote
    (
        "Steady-state solver for incompressible, turbulent flows."
    );

    #include "postProcess.H"
#ifdef FOAM16
    #include "setRootCase.H"
#else
    #include "setRootCaseLists.H"
#endif
    #include "createTime.H"
    #include "createMesh.H"
    #include "createControl.H"
    #include "createFields.H"
#ifdef FOAM16
    //#include "createFvOptions.H"
#endif
    #include "initContinuityErrs.H"

    turbulence->validate();

    #include <iostream>
    #include <fstream>

    int taskid, numtasks;
    taskid = Pstream::myProcNo();

    double t1, t2;
    std::vector<double> time_stats;

		const char * field_name= "region";
#ifdef METHOD_CLOUD
    broker_ctx * context = broker_init(field_name, cloud_config.hostname, cloud_config.port, taskid, 0);
#endif

    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
		char utc_time[80];
		{ // a barrier
				label tmp = Pstream::myProcNo();
				reduce(tmp,sumOp<label>());
		}
		// MPI_Barrier((MPI_Comm)Pstream::worldComm)
		if (Pstream::master())
		{
			get_utc_time(utc_time);
			if(taskid == 0)
      PINF("-- Simulation started, %s", utc_time);
    	Info<< "\nStarting time loop\n" << endl;
		}


#ifdef FOAM1906
    while (simple.loop())
#else
    while (simple.loop(runTime))
#endif
    {
        string out_dir = std::getenv("PWD");
        string str_time = runTime.timeName();
        Info<< "Time = " << str_time << nl << endl;
        int step =  atoi(str_time.c_str());

        // --- Pessure-velocity SIMPLE corrector
        {
            #include "UEqn.H"
            #include "pEqn.H"
        }

        laminarTransport.correct();
        turbulence->correct();

        if(runTime.writeTime()){
#ifdef METHOD_FILE
              ofstream out_file(out_dir +"/snapshot_t" + str_time +".txt");
              forAll(p , i)
              {
                  out_file  << p[i] << std::endl;
              }

              /*
               forAll(U , i)
               {
                  out_file << U[i].x() << U[i].y() << U[i].z() << std::endl;
               }
             */

              out_file.close();
#elif defined(METHOD_CLOUD)

            std::string values;

            t1 = MPI_Wtime();
            forAll(U , i)
              {
                  values.append(std::to_string(U[i].x()));
                  values.append(",");
              }

						if(0 != broker_put(context, step, values)) break;

						t2 = MPI_Wtime();
						time_stats.push_back(t2-t1);

#elif defined(METHOD_NOWRITE)
          // do nothing
          //Info<< "Write disabled at step" << str_time << nl << endl;
#else
					// collated or unclloated write
          t1 = MPI_Wtime();
          runTime.write();

          t2 = MPI_Wtime();
          time_stats.push_back(t2-t1);
#endif

        // runTime.printExecutionTime(Info);
        //
        }
    }

		{ // a barrier
				label tmp = Pstream::myProcNo();
				reduce(tmp,sumOp<label>());
		}
		// MPI_Barrier((MPI_Comm)Pstream::worldComm)
		if (Pstream::master())
		{
			get_utc_time(utc_time);

      PINF("-- Simulation Ended:, %s", utc_time);
      double io_time_used = 0;
      for(auto iter = time_stats.begin(); iter != time_stats.end(); iter ++){
        PDBG("one end uses: %.3f", *iter);
        io_time_used += *iter;
      }

      PINF("total send time %.6f s", io_time_used);
      PINF("avg send time %.6f s, for %d iterations", io_time_used/time_stats.size(), time_stats.size() );
    }

#ifdef METHOD_CLOUD
      broker_finalize(context);
#endif

    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
