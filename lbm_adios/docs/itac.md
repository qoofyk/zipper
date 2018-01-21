USE Intel tracing tool
#### status
1. automatically instrumentation:
    -tcollect -trace
    not successful with cmakea:
        VT_fileio.c:(.text+0x4d5): undefined reference to `PMPI_Comm_rank'
    tcollect should be link option?

2. manually instrumentation:
    see tests/itac_examples for more details

#### steps
   1.  compile code with -trace(use intel compiler)
   2. mpirun -trace the binary
   3. either:
    a. in output directory($LOGFILE_PREFIX), traceanalyzer xx.stf, view from remote directly
    b. merge the trace file:(its faster to generate multiple files and then merge them offline)
        stftool lbm.stf --convert lbm_single.stf --logfile-format SINGLESTF
   4. in a desktop
        traceanalyzer lbm_single.stf


#### software
    after installations, need to source install_dir>/parallel_studio_xe_<version>.x.xxx/bin/psxevars.sh


#### notes
1. cannot find VT.h?
    add definition
2. ld.so: object ''libVT.so' from LD_PRELOAD cannot be preloaded: ignored
    a. if compiled with -trace, don't run with -trace, this will link intel trace statically
    b. don't compile with -trace, instread use mpirun -trace, this will link trace lib dynamically
3. to use itac with cmake, only need to find VT.h and link the dynmaic libVT.so with applications
4. should use MPIC_CC_COMPILE_FLAGS instead of CMAKE_C_FLAGS, see [here](https://cmake.org/cmake/help/v3.7/module/FindMPI.html)
5. if LD_PRELOAD is not set as libVT.so, you need the 
6. cmakelist, library, vt should be ahead of mpi
7. for mpi code, VT_function should be placed after MPI_Init
    for non-mpi, there should be a VT_Init
8. decaf use mod-lib, which is loaded during runtime, use specify export LD_PRELAD=libVT.so, to generate the trace, but lbm_sim_only doesn't require.
9. decaf is different, it uses a module library which loaded in runtime, it will find VT entry then. but can it find static linked VT? must use dynamic linked VT and then export LD_PRELOAD then
10. if the application has very frequently function calls, edit the conf file to filter those functions


#### no-mpi build
    if no using mpicc:
        icc -L $VT_ROOT/slib -lVTcs -lpthread -I$VT_ROOT/include tmp.c
    
