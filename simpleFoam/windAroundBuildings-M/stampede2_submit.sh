#!/bin/bash
#SBATCH --job-name="wind-M"
#SBATCH --output="log.slurm.%j.out"
#SBATCH --partition=normal    
#SBATCH --nodes=8             
#SBATCH --ntasks-per-node=64   
#SBATCH -t 00:15:00             

export SUBMITDIR=$SLURM_SUBMIT_DIR
export BROKER_ENDPOINT_FILE="$SUBMITDIR/../../cloud-components/endpoints_512.ini"
cd $SUBMITDIR

export data_str=$(date +"%Y%m%d-%H%M")
export RUNDIR=$SCRATCH/wind/$data_str

export RESULTDIR=$SUBMITDIR/saved_logs/$data_str
mkdir -pv ${RESULTDIR}
mkdir -pv $RUNDIR

module list

source ./AllrunParallel
## Wait for the entire workflow to finish



