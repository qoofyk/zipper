#!/bin/bash
#----------------------------------------------------
# Sample Slurm job script
#   for TACC Stampede2 KNL nodes
#
#   *** MPI Job on Normal Queue ***
# 
# Last revised: 20 Oct 2017
#
# Notes:
#
#   -- Launch this script by executing
#      "sbatch knl.mpi.slurm" on Stampede2 login node.
#
#   -- Use ibrun to launch MPI codes on TACC systems.
#      Do not use mpirun or mpiexec.
#
#   -- Max recommended MPI tasks per KNL node: 64-68
#      (start small, increase gradually).
#
#   -- If you're running out of memory, try running
#      fewer tasks per node to give each task more memory.
#
#----------------------------------------------------

#SBATCH -J bench-4x1          # Job name
#SBATCH -o results/bench.o%j       # Name of stdout output file
#SBATCH -p normal          # Queue (partition) name
#SBATCH -N 8               # Total # of nodes 
#SBATCH --tasks-per-node=1
#SBATCH -t 00:10:00        # Run time (hh:mm:ss)

# Other commands must follow all #SBATCH directives...

module list
pwd
date

# Launch MPI code...
all_ips=(129.114.16.101 129.114.16.63 129.114.16.67 129.114.16.76 129.114.16.98 129.114.16.45 129.114.16.50 129.114.16.60)

echo "test cloud endpoints: ${all_ips[*]}"

export RESULT_FOLDER=results/$SLURM_JOB_ID
mkdir $RESULT_FOLDER

nodes=(`scontrol show hostname $SLURM_NODELIST`)
echo "all nodes: $nodes"

MY_PASS=5ba4239a1a2b7cd8131da1e557f4264df7ef2083f8895eab1d30384f870a9d87
REDIS_BENCH=$WORK/zipper-runtime/extern/redis/src/redis-benchmark
#NR_ELEMS_PER_CONN=1000
NR_ELEMS_PER_CONN=500 # shorttest
elem_size=90000  #10000 elems each have 9bytes

echo "Total of $elapsed seconds elapsed for process"


for nr_conn in 1 4 16 64
do
  echo "================================================="
  echo "==========Test $nr_conn connections: ============"
  echo "================================================="

  start_time=`date +%s%N`

  for nodeid in $(seq 0 $((SLURM_NNODES-1))); do
    nodename=${nodes[${nodeid}]}
    echo "placing on node ${nodename}"
    # check process host binding
    # ibrun -n 1 -o ${nodeid} hostname &> $RESULT_FOLDER/node${nodeid} &
    outfile=$RESULT_FOLDER/log.cpn${nr_conn}.node${nodeid}
    date> $outfile
    # mpirun -n 1 -hosts ${nodename} iperf3 -c ${all_ips[nodeid]} -t 30 -l 32K -p 31993 -P $nr_conn &>> $outfile &     # Use ibrun instead of mpirun or mpiexec

    nr_elems=$((NR_ELEMS_PER_CONN*nr_conn))
    mpirun -n 1 -hosts ${nodename} $REDIS_BENCH -h ${all_ips[nodeid]} -p 30379 -a $MY_PASS -r 10000 -n $nr_elems -t set -d ${elem_size} -c ${nr_conn} -P 8  &>> $outfile &
  done
  #mpirun -n 1 iperf3 -c ${all_ips[1]} -p 1993 -P $nr_conn &     # Use ibrun instead of mpirun or mpiexec
  #mpirun -n 1 iperf3 -c ${all_ips[2]} -p 1993 -P $nr_conn &     # Use ibrun instead of mpirun or mpiexec
  #mpirun -n 1 iperf3 -c ${all_ips[3]} -p 1993 -P $nr_conn &     # Use ibrun instead of mpirun or mpiexec
  wait
  end_time=`date +%s%N`
  elapsed=`python3 -c "print(($end_time-$start_time)/1000000000)"`

  data_total_size=`python3 -c "print($nr_elems*$elem_size*$SLURM_NNODES/1000000.0)"`
  throughput=`python3 -c "print($data_total_size/$elapsed)"`

  echo -e "STATS: nr_conn_per_node\t MB objects\t seconds\t MB/s"
  echo -e "STATS: $nr_conn\t $data_total_size\t $elapsed \t $throughput"
  sleep 10
done

#cp results/bench-4x1.o${PBS_JOB_ID} $RESULT_FOLDER
