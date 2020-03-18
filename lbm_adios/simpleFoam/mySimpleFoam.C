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

#include "fvCFD.H"
#include "singlePhaseTransportModel.H"
#include "turbulentTransportModel.H"
#include "simpleControl.H"
#include "fvOptions.H"

#define USE_MYDUMP

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

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

    #ifdef USE_MYDUMP
    // #include "MyDump.H"
    // basic file operations
    #include <iostream>
    #include <fstream>
    /*
    #include <unistd.h>
    char buff[FILENAME_MAX]; //create string buffer to hold path
    getcwd( buff, FILENAME_MAX );
    cout << "will dump to" << buff << endl;
    */
    string out_dir = std::getenv("PWD");
    #endif


    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

    Info << "Running my Simple Foam, results saved to " << out_dir << endl;

    Info<< "\nStarting time loop\n" << endl;

    while (simple.loop(runTime))
    {
        string str_time = runTime.timeName();
        Info<< "Time = " << str_time << nl << endl;

        // --- Pessure-velocity SIMPLE corrector
        {
            #include "UEqn.H"
            #include "pEqn.H"
        }

        laminarTransport.correct();
        turbulence->correct();

        runTime.write();

    #ifdef USE_MYDUMP

        if(runTime.writeTime()){
            ofstream out_file(out_dir +"/snapshot_t" + str_time +".txt");
            forAll(p , i)
            {
                out_file  << p[i] << " " << U[i].x() << " " << U[i].y() << " "<< U[i].z() << std::endl;
            }
            out_file.close();
        }
    #endif

        Info<< "ExecutionTime = " << runTime.elapsedCpuTime() << " s"
            << "  ClockTime = " << runTime.elapsedClockTime() << " s"
            << nl << endl;
    }

    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //