#################################################### 

export OMP_NUM_THREADS=4

env|grep '^HAS' # trace enabled?
env|grep '^OMP' # trace enabled?

#module load remora

#module load libfabric
module load boost
module load phdf5
module list



echo "procs is \[ ${procs_this_app[*]}\], nodes is \[${nodes_this_app[*]}\]"

export BUILD_DIR=${PBS_O_WORKDIR}/build

#This job runs with 3 nodes  
#ibrun in verbose mode will give binding detail  #BUILD=${PBS_O_WORKDIR}/build_dspaces/bin
PBS_RESULTDIR=${SCRATCH_DIR}/results



mkdir -pv ${PBS_RESULTDIR}
tune_stripe_count=-1
lfs setstripe --stripe-size 1m --stripe-count ${tune_stripe_count} ${PBS_RESULTDIR}
mkdir -pv ${SCRATCH_DIR}
cd ${SCRATCH_DIR}
#cp -R ${PBS_O_WORKDIR}/global_range_select/arrays.xml ${SCRATCH_DIR}


# this scrWorkspaces/General_Data_Broker/lbm_adios/scripts
GENERATE_HOST_SCRIPT=${PBS_O_WORKDIR}/scripts/generate_hosts.sh
#GENERATE_HOST_SCRIPT=${HOME}/Downloads/LaucherTest/generate_hosts.sh
if [ -a $GENERATE_HOST_SCRIPT ]; then
    source $GENERATE_HOST_SCRIPT
else
    echo "generate_hosts.sh should downloaded from:"
    echo "https://github.iu.edu/lifen/LaucherTest/blob/master/generate_hosts.sh"
fi

export procs_prod=${procs_this_app[0]}
export procs_link=${procs_this_app[1]}
export procs_con=${procs_this_app[2]}

procs_all=$((procs_prod + procs_con + procs_link))

# generate graph
#PYTHON_RUN="python $PBS_O_WORKDIR/vector/vector_2nodes.py --np ${procs_all} --hostfile ${HOST_DIR}/machinefile-all"
PYTHON_RUN="python $PBS_O_WORKDIR/decaf/vector_2nodes_topo.py --np ${procs_all} --hostfile ${HOST_DIR}/machinefile-all"
$PYTHON_RUN &> python.log
echo "python run $PYTHON_RUN"

## order is prod/link/consumer
mpirun -l  --machinefile ${HOST_DIR}/machinefile-all -np ${procs_prod} $BUILD_DIR/bin/vector_2nodes : -np ${procs_link} $BUILD_DIR/bin/vector_2nodes : -np ${procs_con} $BUILD_DIR/bin/vector_2nodes


## Wait for the entire workflow to finish
wait
