#!/bin/bash  
#PBS -N LaunchTest 
#PBS -j oe
#PBS -l walltime=00:01:00
#PBS -l nodes=4:ppn=16
#PBS -o results/$PBS_JOBID.output
#PBS -m b

# -q only if nodes < 4
# qsub -l nodes=4:ppn=16 -q debug job_scripts/launchSynthetic.sh
source ${PBS_O_WORKDIR}/job_scripts/common.sh

# 0-16, 17-32 willbe mapped to first redis, region_id=0~32
analysis_size=16 # how many processes will map to one redis
nr_nodes_per_analysis=$((analysis_size/SIM_PROCS))

node_idx=0
# for each 16 procs
for node in $allnodes
do
  redis_idx=$((node_idx/nr_nodes_per_analysis))
  id_in_group=$((node_idx%nr_nodes_per_analysis))
  REDIS_IP=${REDIS_LIST[redis_idx]}
  CMD="build/bin/test-put-mpi-foam -n 10000 -i 2000 -p 6379 -h $REDIS_IP $id_in_group"

  echo "launching a mpirun ${CMD} to node ${node}, redisidx: $redis_idx, id_in_group: $id_in_group"
  #echo "launching a mpirun ${CMD} to node ${node}, redishost: $REDIS_IP}"
  mpirun -n  ${SIM_PROCS} -host ${node} $CMD &>${PBS_RESULTDIR}/${node}.log &
  node_idx=$((node_idx+1))
done

wait
