## DMD analysis

#### Background
1. See the [mrDMD paper](https://epubs.siam.org/doi/pdf/10.1137/15M1023543)
2. openfoam can be downloaded from ``http://dl.openfoam.org/ubuntu/dists/bionic/main/binary-amd64/openfoam6_20190620_amd64.deb``

    - example used is in https://github.com/OpenFOAM/OpenFOAM-6/tree/master/tutorials/incompressible/simpleFoam/windAroundBuildings
    - tutorial in https://cfd.direct/openfoam/user-guide/v7-tutorials/
    - ``source /opt/openfoam6/etc/bashrc`` before use it
    - visualization guide: https://www.openfoam.com/documentation/user-guide/paraview.php
    - simplefoam (https://www.openfoam.com/documentation/guides/latest/doc/guide-applications-solvers-incompressible-simpleFoam.html)

#### Steps
1. Generate data from openfoam (try to use the same code from https://github.com/OpenFOAM/OpenFOAM-5.x/tree/master/tutorials/incompressible/simpleFoam/windAroundBuildings)
2. Data will be inserted to Redis.
2. redis-spark will pip time-series data to pydmd using spark pipe.
