#!/bin/bash
#SBATCH -J ElasticbrokerSynthetic 
#SBATCH -o results/%j.output      # Name of stdout output file
#SBATCH -p skx-normal         # Queue (partition) name
#SBATCH -N 2              # Total # of nodes 
#SBATCH --ntasks-per-node=16
#SBATCH -t 00:01:00        # Run time (hh:mm:ss)
#SBATCH --mail-user=lifen@iupui.edu


##SBATCH --mail-type=all    # Send email at begin and end of job

# -q only if nodes < 4
# qsub -l nodes=4:ppn=16 -q debug job_scripts/launchSynthetic.sh
SIM_PROCS=16 # square of proc_side
allnodes=(`scontrol show hostname $SLURM_NODELIST`)

REDIS_LIST=(149.165.168.72 149.165.168.118)

RESULTDIR=$(pwd)/results
mkdir -pv $RESULTDIR

# 0-16, 17-32 willbe mapped to first redis, region_id=0~32
analysis_size=16 # how many processes will map to one redis
nr_nodes_per_analysis=$((analysis_size/SIM_PROCS))

node_idx=0
# for each 16 procs
for node in ${allnodes[*]}
do
  redis_idx=$((node_idx/nr_nodes_per_analysis))
  id_in_group=$((node_idx%nr_nodes_per_analysis))
  REDIS_IP=${REDIS_LIST[redis_idx]}
  CMD="build/bin/test-put-mpi-foam -n 10000 -i 2000 -p 6379 -h $REDIS_IP $id_in_group"

  echo "launching a mpirun ${CMD} to node ${node}, redisidx: $redis_idx, id_in_group: $id_in_group"
  #echo "launching a mpirun ${CMD} to node ${node}, redishost: $REDIS_IP}"
  ibrun -n  ${SIM_PROCS} -o $((node_idx*SIM_PROCS)) $CMD &>${RESULTDIR}/${node}.log &
  node_idx=$((node_idx+1))
done

wait


