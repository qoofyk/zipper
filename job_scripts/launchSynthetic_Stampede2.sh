#!/bin/bash
#SBATCH -J ElasticbrokerSynthetic 
#SBATCH -o results/log.slurm.%j.output      # Name of stdout output file
#SBATCH -p normal         # Queue (partition) name
#SBATCH -N 8        # Total # of nodes 
#SBATCH --ntasks-per-node=16
#SBATCH -t 00:15:00        # Run time (hh:mm:ss)
#SBATCH --mail-type=BEGIN

#submit from source dir
export SUBMITDIR=$SLURM_SUBMIT_DIR
export NUM_PROCS=$SLURM_NTASKS
export SUBMITDIR=$SLURM_SUBMIT_DIR
export BROKER_ENDPOINT_FILE="$SUBMITDIR/cloud-components/endpoints_128.ini"

# module load remora
module list

export REMORA_PERIOD=1
#export RUN="remora ibrun"
export RUN="ibrun"

export EXE_DIR=$SUBMITDIR/build/bin
export CMD="$EXE_DIR/test-put-mpi-foam -n 10000 -i 1000"


#echo "launching a mpirun ${CMD} with $NUM_PROCS procs, dryrun"
## dryrun
#$RUN -n  ${NUM_PROCS}  $CMD -d


# realrun
echo "launching a mpirun ${CMD} with $NUM_PROCS procs, realrun"
$RUN -n  ${NUM_PROCS}  $CMD

echo "Now exiting..."
