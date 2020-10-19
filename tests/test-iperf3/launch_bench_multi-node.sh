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
#SBATCH -N 8               # Total # of nodes 
#SBATCH --tasks-per-node=1
#SBATCH -t 00:05:00        # Run time (hh:mm:ss)

# Other commands must follow all #SBATCH directives...

module list
pwd
date

# Launch MPI code...
all_ips=(129.114.17.231 129.114.104.188 129.114.104.167 129.114.17.50 129.114.104.201 129.114.104.158 129.114.104.131 129.114.104.35)

echo "test cloud endpoints: ${all_ips[*]}"

export RESULT_FOLDER=results/$SLURM_JOB_ID
mkdir $RESULT_FOLDER
mkdir 

for nr_conn in 64
do
  echo "================================================="
  echo "==========Test $nr_conn connections: ============"
  echo "================================================="
  for nodeid in $(seq 0 $((SLURM_NNODES-1))); do
    # check process host binding
    # ibrun -n 1 -o ${nodeid} hostname &> $RESULT_FOLDER/node${nodeid} &
    outfile=$RESULT_FOLDER/log.node${nodeid}
    date> $outfile
    ibrun -n 1 -o ${nodeid} iperf3 -c ${all_ips[nodeid]} -t 30 -l 32K -p 1993 -P $nr_conn &>> $outfile &     # Use ibrun instead of mpirun or mpiexec
  done
  #mpirun -n 1 iperf3 -c ${all_ips[1]} -p 1993 -P $nr_conn &     # Use ibrun instead of mpirun or mpiexec
  #mpirun -n 1 iperf3 -c ${all_ips[2]} -p 1993 -P $nr_conn &     # Use ibrun instead of mpirun or mpiexec
  #mpirun -n 1 iperf3 -c ${all_ips[3]} -p 1993 -P $nr_conn &     # Use ibrun instead of mpirun or mpiexec
done

wait
echo "All complete"
#cp results/bench-4x1.o${PBS_JOB_ID} $RESULT_FOLDER
