#!/bin/bash
#SBATCH --job-name="wind-M"
#SBATCH --output="log.slurm.%j.out"
#SBATCH --partition=normal    
#SBATCH --nodes=64             
#SBATCH --ntasks-per-node=64   
#SBATCH -t 00:05:00             

export WORKDIR=$SLURM_SUBMIT_DIR

cd $WORKDIR

RESULTDIR=$WORKDIR/results
mkdir -pv ${RESULTDIR}

module list


source ./AllrunParallel


## Wait for the entire workflow to finish



