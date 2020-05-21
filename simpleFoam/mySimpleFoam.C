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

#define DEBUG
#include "common/logging.h"
#include "common/utility.h"


#include "fvCFD.H"
#include "singlePhaseTransportModel.H"
#include "turbulentTransportModel.H"
#include "simpleControl.H"
#include "fvOptions.H"

#define FOAM1906

extern "C"
{
  #include "hiredis.h"
}

struct{
  const char * hostname = "149.165.171.123";
  int port = 30379;
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

#ifdef METHOD_CLOUD
    redisContext *c;
    redisReply *reply;

    char stream_name[80];
    sprintf(stream_name, "region%d", taskid);

    unsigned int isunix = cloud_config.isunix;


    int queue_len = 128;
    int ii = 0;
      // taskid =1;
    const char * hostname = cloud_config.hostname;
    int port = cloud_config.port;

    PINF("running exp with server(%s:%d), taskid = %d",
       hostname, port, taskid);

    
    struct timeval timeout = {1, 500000}; // 1.5 seconds
    if (isunix) {
      c = redisConnectUnixWithTimeout(hostname, timeout);
    } else {
      c = redisConnectWithTimeout(hostname, port, timeout);
    }
    if (c == NULL || c->err) {
      if (c) {
        Info << "Connection error:" << c->errstr << endl;
        redisFree(c);
      } else {
        Info <<"Connection error: can't allocate redis context" << endl;
      }
      exit(1);
    }

    /* PING server */
    reply = (redisReply *)redisCommand(c, "PING");
    Info <<"PING: " <<reply->str << endl;
    freeReplyObject(reply);

      /* writing some floatting number with binary-safe string */
#endif

    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
    Info<< "\nStarting time loop\n" << endl;

    char str_time[80];
    get_utc_time(str_time);
    if(taskid == 0)
      PINF("-- Simulation started, %s", str_time);


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

#ifdef METHOD_FILE
          if(runTime.writeTime()){
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
          }
#elif defined(METHOD_CLOUD)

          if(runTime.writeTime()){

              t1 = MPI_Wtime();
              std::string commandString = "XADD ";
              commandString.append(stream_name);
              commandString.append(" MAXLEN ~ 1000 * ");
              commandString.append(" step ");
              commandString.append(str_time);

              // commandString.append(" field ")
              // commandString.append(fieldname)
              commandString.append(" valuelist ");

              forAll(U , i)
              {
                  commandString.append(std::to_string(U[i].x()));
                  commandString.append(",");
              }
              // Info<< "----command is" << commandString << endl;

              reply = (redisReply *)redisCommand(c, commandString.c_str());
              if(reply == NULL || reply->type == REDIS_REPLY_ERROR){
                PERR("Error on hiredis command ");
                if(reply)
                  PERR("error info: %s", reply->str);
                break;
              }
              freeReplyObject(reply);

              t2 = MPI_Wtime();
              time_stats.push_back(t2-t1);
           }

#elif defined(METHOD_NOWRITE)
          // do nothing
#else
          t1 = MPI_Wtime();
          runTime.write();

          t2 = MPI_Wtime();
          time_stats.push_back(t2-t1);
#endif

        runTime.printExecutionTime(Info);
    }

    get_utc_time(str_time);
    if(taskid == 0){
      PINF("-- Simulation Ended:, %s", str_time);
      double io_time_used = 0;
      for(auto iter = time_stats.begin(); iter != time_stats.end(); iter ++){
        PDBG("one end uses: %.3f", *iter);
        io_time_used += *iter;
      }

      PINF("total send time %.6f s", io_time_used);
      PINF("avg send time %.6f s, for %d iterations", io_time_used/time_stats.size(), time_stats.size() );
    }

#ifdef METHOD_CLOUD
          redisFree(c);
#endif

    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
