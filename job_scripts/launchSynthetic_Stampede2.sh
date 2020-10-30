#!/bin/bash
#SBATCH -J ElasticbrokerSynthetic 
#SBATCH -o results/log.slurm.%j.output      # Name of stdout output file
#SBATCH -p normal         # Queue (partition) name
#SBATCH -N 4     # Total # of nodes 
#SBATCH --ntasks-per-node=16
#SBATCH -t 00:15:00        # Run time (hh:mm:ss)
#SBATCH --mail-type=BEGIN

#submit from source dir
export ENABLE_TRACE=true

export NUM_PROCS=$SLURM_NTASKS
export SUBMITDIR=$SLURM_SUBMIT_DIR
export RUNDIR=$SUBMITDIR/results/${SLURM_JOBID}

# python3 ../python/mapper.py 512 ip1 ip2 ... > endpoints_512.ini
#export BROKER_ENDPOINT_FILE="$SUBMITDIR/cloud-components/endpoints_512.ini"
export BROKER_ENDPOINT_FILE="$SUBMITDIR/cloud-components/endpoints_64.ini"
export BROKER_QUEUE_LEN=8
# module load remora
module list

export REMORA_PERIOD=1
#export RUN="remora ibrun"
export RUN="ibrun"


if $ENABLE_TRACE; then
  export EXE_FILE=$SUBMITDIR/build/bin/test-synthetic-vt
else
  export EXE_FILE=$SUBMITDIR/build/bin/test-synthetic
fi
export CMD="$EXE_FILE -n 10000 -i 1000"

mkdir -pv $RUNDIR
export VT_LOGFILE_PREFIX=${RUNDIR}/trace
mkdir -p $VT_LOGFILE_PREFIX

echo "---------------"
echo "Rundir=$RUNDIR, bash_source = $BASH_SOURCE"
echo "---------------"

# backup for future reference
cp $BROKER_ENDPOINT_FILE $RUNDIR
cp $EXE_FILE $RUNDIR
cp $BASH_SOURCE $RUNDIR
cd $RUNDIR

echo "launching a mpirun ${CMD} with $NUM_PROCS procs, $SLURM_NTASKS_PER_NODE per node"

#echo "launching a mpirun ${CMD} with $NUM_PROCS procs, dryrun"
## dryrun
#$RUN -n  ${NUM_PROCS}  $CMD -d


# realrun
echo "realrun"
$RUN -n  ${NUM_PROCS}  $CMD

# generate trace in a sigle file
if $ENABLE_TRACE; then
  EXE_NAME=`basename ${EXE_FILE}`
  stftool trace/${EXE_NAME}.stf --convert ${EXE_NAME}_${SLURM_JOBID}.stf --logfile-format SINGLESTF
fi

echo "Now exiting..."
