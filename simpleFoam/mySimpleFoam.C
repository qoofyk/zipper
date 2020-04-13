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

#include "Hash.H"
#include "Field.H"
#include "FieldField.H"
#include <getopt.h>

#include "fvCFD.H"
#include "singlePhaseTransportModel.H"
#include "turbulentTransportModel.H"
#include "simpleControl.H"
#include "fvOptions.H"
extern "C"
{
  #include "hiredis.h"
}

#define USE_MYDUMP

enum METHOD{
  METHOD_LAMMPS = 0,
  METHOD_FILE = 1,
  METHOD_REDIS = 2
};


int main(int argc, char *argv[])
{
    #include "postProcess.H"

    #include "setRootCaseLists.H"
    #include "createTime.H"
    #include "createMesh.H"
    #include "createControl.H"
    #include "createFields.H"
    #include "initContinuityErrs.H"

    turbulence->validate();

    // #include "MyDump.H"
    // basic file operations
    #include <iostream>
    #include <fstream>
    #include <getopt.h>
    /*
    #include <unistd.h>
    char buff[FILENAME_MAX]; //create string buffer to hold path
    getcwd( buff, FILENAME_MAX );
    cout << "will dump to" << buff << endl;
    */
    redisContext *c;
    redisReply *reply;
    const char *stream_name = "fluids";

    const char * hostname = "127.0.0.1";
    int port = 6379;;
    int nr_steps = 30;
    int taskid, numtasks;
    unsigned int isunix = 0;

    int queue_len = 128;
    int ii = 0;

    int method = METHOD_REDIS;
    string out_dir = std::getenv("PWD");

    taskid =1;

    if(method == METHOD_REDIS){
      
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
    }



    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

    Info << "Running my Simple Foam, results saved to " << out_dir << endl;

    Info<< "\nStarting time loop\n" << endl;

    while (simple.loop(runTime))
    {
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


        if(method == METHOD_FILE){

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
        }
        else if(method ==METHOD_REDIS){

          if(runTime.writeTime()){
              std::string commandString = "XADD ";
              commandString.append(stream_name);
              commandString.append(" MAXLEN ~ 1000000 * ");
              commandString.append(" step ");
              commandString.append(str_time);
              commandString.append(" region_id ");
              commandString.append(std::to_string(taskid));
              // commandString.append(" field ")
              // commandString.append(fieldname)
              commandString.append(" valuelist ");

              forAll(U , i)
              {
                  commandString.append(std::to_string(U[i].x()));
                  commandString.append(",");
              }
              std::cout << "----Writing!" << std::endl;
              // Info<< "----command is" << commandString << endl;
              redisAppendCommand(
                  c, commandString.c_str());
              if(step > 0 && step % queue_len == 0){
                for(ii = 0; ii < queue_len; ii ++){
                  redisGetReply(c, (void **)(&reply));
                  freeReplyObject(reply);
                }
              }
              redisGetReply(c, (void **)(&reply));
              freeReplyObject(reply);
              }
        }
        else{
          runTime.write();
        }

        Info<< "ExecutionTime = " << runTime.elapsedCpuTime() << " s"
            << "  ClockTime = " << runTime.elapsedClockTime() << " s"
            << nl << endl;
    }
    if(method ==METHOD_REDIS){
          redisFree(c);
        }

    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
