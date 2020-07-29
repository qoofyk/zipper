#!/bin/bash  
#PBS -N LaunchTest 
#PBS -j oe
#PBS -q debug 
#PBS -l walltime=00:01:00
#PBS -l nodes=4:ppn=16
#PBS -o results/$PBS_JOBID.output


source ${PBS_O_WORKDIR}/job_scripts/common.sh

node_idx=0
for node in $allnodes
do
  CMD=hostname
  REDIS_IP=${REDIS_LIST[node_idx]}
  echo "launching a mpirun ${CMD} to node ${node}, redishost: $REDIS_IP}"

  mpirun -n  ${SIM_PROCS} -host ${node} $CMD &>${PBS_RESULTDIR}/${node}.log &
  node_idx=$((node_idx+1))
done

wait
