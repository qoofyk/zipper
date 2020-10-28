#!/bin/bash
#SBATCH -J ElasticbrokerSynthetic 
#SBATCH -o results/log.slurm.%j.output      # Name of stdout output file
#SBATCH -p normal         # Queue (partition) name
#SBATCH -N 1     # Total # of nodes 
#SBATCH --ntasks-per-node=64
#SBATCH -t 00:15:00        # Run time (hh:mm:ss)
#SBATCH --mail-type=BEGIN

#submit from source dir
export NUM_PROCS=$SLURM_NTASKS
export SUBMITDIR=$SLURM_SUBMIT_DIR
export RUNDIR=$SUBMITDIR/results/${SLRUM_JOBID}

# python3 ../python/mapper.py 512 ip1 ip2 ... > endpoints_512.ini
#export BROKER_ENDPOINT_FILE="$SUBMITDIR/cloud-components/endpoints_512.ini"
export BROKER_ENDPOINT_FILE="$SUBMITDIR/cloud-components/endpoints_64.ini"
export BROKER_QUEUE_LEN=8
# module load remora
module list

export REMORA_PERIOD=1
#export RUN="remora ibrun"
export RUN="ibrun"


export EXE_FILE=$SUBMITDIR/build/bin/test-put-mpi-foam
export CMD="$EXE_FILE -n 10000 -i 1000"

echo "slurm jobid: $SLURM_JOBID, $SLURM_JOB_ID"
mkdir -pv $RUNDIR
export VT_LOGFILE_PREFIX=${RUNDIR}/trace
mkdir -p $VT_LOGFILE_PREFIX

# backup for future reference
cp $BROKER_ENDPOINT_FILE $RUNDIR
cp $EXE_FILE $RUNDIR
cd $RUNDIR

#echo "launching a mpirun ${CMD} with $NUM_PROCS procs, dryrun"
## dryrun
#$RUN -n  ${NUM_PROCS}  $CMD -d


# realrun
echo "launching a mpirun ${CMD} with $NUM_PROCS procs, realrun"
$RUN -n  ${NUM_PROCS}  $CMD

# generate trace in a sigle file
EXE_NAME=`basename ${EXE_FILE}`
stftool trace/${EXE_NAME}.stf --convert ${EXE_NAME}_${SLURM_JOBID}.stf --logfile-format SINGLESTF

echo "Now exiting..."
