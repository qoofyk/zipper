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
#SBATCH -o results/bench-4x1.o%j       # Name of stdout output file
#SBATCH -p normal          # Queue (partition) name
#SBATCH -N 2               # Total # of nodes 
#SBATCH --tasks-per-node=1
#SBATCH -t 00:05:00        # Run time (hh:mm:ss)

# Other commands must follow all #SBATCH directives...

module list
pwd
date

# Launch MPI code...
all_ips=(129.114.104.67 129.114.16.85 129.114.16.50 129.114.16.79 129.114.16.97 129.114.16.77 129.114.16.60 129.114.16.45)

echo "test cloud endpoints: ${all_ips[*]}"

export RESULT_FOLDER=results/$SLURM_JOB_ID
mkdir $RESULT_FOLDER

nodes=(`scontrol show hostname $SLURM_NODELIST`)
echo "all nodes: $nodes"

for nr_conn in 1
do
  echo "================================================="
  echo "==========Test $nr_conn connections: ============"
  echo "================================================="
  for nodeid in $(seq 0 $((SLURM_NNODES-1))); do
    nodename=${nodes[${nodeid}]}
    echo "placing on node ${nodename}"
    # check process host binding
    # ibrun -n 1 -o ${nodeid} hostname &> $RESULT_FOLDER/node${nodeid} &
    outfile=$RESULT_FOLDER/log.node${nodeid}
    date> $outfile
    mpirun -n 1 -hosts ${nodename} iperf3 -c ${all_ips[nodeid]} -t 30 -l 32K -p 31993 -P $nr_conn &>> $outfile &     # Use ibrun instead of mpirun or mpiexec
  done
  #mpirun -n 1 iperf3 -c ${all_ips[1]} -p 1993 -P $nr_conn &     # Use ibrun instead of mpirun or mpiexec
  #mpirun -n 1 iperf3 -c ${all_ips[2]} -p 1993 -P $nr_conn &     # Use ibrun instead of mpirun or mpiexec
  #mpirun -n 1 iperf3 -c ${all_ips[3]} -p 1993 -P $nr_conn &     # Use ibrun instead of mpirun or mpiexec
done

wait
echo "All complete"
#cp results/bench-4x1.o${PBS_JOB_ID} $RESULT_FOLDER
