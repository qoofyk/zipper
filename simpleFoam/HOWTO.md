## SimpleFoam with windAroundBuild case

#### References
openfoam can be downloaded from ``http://dl.openfoam.org/ubuntu/dists/bionic/main/binary-amd64/openfoam6_20190620_amd64.deb``

    - example used is in https://github.com/OpenFOAM/OpenFOAM-6/tree/master/tutorials/incompressible/simpleFoam/windAroundBuildings
    - tutorial in https://cfd.direct/openfoam/user-guide/v7-tutorials/
    - ``source /opt/openfoam6/etc/bashrc`` before use it
    - visualization guide: https://www.openfoam.com/documentation/user-guide/paraview.php
    - simplefoam (https://www.openfoam.com/documentation/guides/latest/doc/guide-applications-solvers-incompressible-simpleFoam.html)

#### Steps
1. Generate data from openfoam (try to use the same code from https://github.com/OpenFOAM/OpenFOAM-5.x/tree/master/tutorials/incompressible/simpleFoam/windAroundBuildings)
2. Data will be inserted to Redis.
3. redis-spark will pip time-series data to pydmd using spark pipe.
4. Baseline: data will be write into files(copied via network if necessary)

#### Data format

DMD works on windows of snapshots. Each stream insert can contain:
1. stepid + partid(geometric-based or just linear-based) + array of velocities
2. Batch processing will get a window for each 20 steps

#### Openfoam
1. build  (will generated binaries in $FOAM_USER_APPBIN, as specified in Make/files, and referred in the $FOAM_RUN/windAroundBuildings_zipper/Allrun script)

See CMakefiles.txt, which are modified from the Make folder from simpleFoam

2. run
```
cd windAroundBuilding_zipper
./Allrun
```

#### With redis
1. link to hiredis (linkage with cmake is totoally fine.)
  - edit the Make/options
  - I don't know why hiredis cannot be linked to the original one:
      ```
      lifen@in-csci-20wk300(:):~/Workspace/zipper-runtime/simpleFoam$ldd /home/lifen/OpenFOAM/lifen-6/platforms/linux64GccDPInt32Opt/bin/mySimpleFoam |grep redis
        libhiredis.so.0.14 => not found
      ```
  - A dirty fix:
      ```
      ln -s /home/lifen/Workspace/zipper-runtime/build/extern/hiredis/libhiredis.so.0.14 $FOAM_USER_LIBBIN/ (FOAM_USER_LIBBIN) is checked in in LD_LIBRARY_PATH
      ```

1. it will call redis by:
```
XADD fluids MAXLEN ~ 1000000 *  step 99 region_id 0 valuelist 0.772943,-0.879689,0.954192,-0.993723,
```
