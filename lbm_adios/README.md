# different transport support for parallel lbm code
# work submitted to IPDPS2018

see [this wiki](https://github.iu.edu/IUPUI-CS-HPC/data_broker_lammps/wiki) for more details

written by Feng Li at Fall 2017

## What is done
    performace analysis
        traces collection for all transports in 8vs4
        performace collection for all transport in 64v32
    scalability test
        scalibility test in both mpiio/flexpath
        

## What is missing
        traces:
            flexpath traces is not shown very clearly in performance analysis
            stampede traces and bridges flexpath traces show different behaviours
        mpiio
            didn't run the largest experiements, will take too long
        flexpath
            flexpath crashes in large experiments, need to fix
   

## Environment settings and Build
    se INSTALL for more details



## Run 
    * Performance test experiments
        * uninstructed build(change 8vs4 to 64v32 to run 64v32 experiemnts for the performance comparison)
            sbatch scripts/bridges/8vs4/native_dimes_nokeep.job
        * instructed build, this will generate all traces in $SCRATCH/$jobid/traces/app0.1.2, respectively
            HAS_TRACE=1 sbatch scripts/bridges/8vs4/native_dimes_nokeep.job
    * scalibility test
       *  note: scalability test experiments(flexpath and native dimes) are performed in both Stampede2/bridges KNL node.I use default compiler at that experiment(icc+impi)
            
        * HOWTO:
            ```shell
            sbatch scripts/stampede2/scaling_exp/flexpath/272vs136.job

            # run with
            HAS_KEEP=1 sbatch scripts/stampede2/scaling_exp/flexpath/68vs34.job
            ```
            * switch stampede2 to bridges to use another machine
            * switch flexpath to mpiio to use different transport
            * 272v136, change to other avaible scripts for different size of experiments
