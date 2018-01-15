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
    
