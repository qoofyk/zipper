####################################################
# common commands for all experiments
BUILD_DIR=${PBS_O_WORKDIR}/build

BIN_PRODUCER=${BUILD_DIR}/bin/run_lbm;
BIN_CONSUMER=${BUILD_DIR}/bin/native_staging_read;

#This job runs with 3 nodes  
#ibrun in verbose mode will give binding detail  #BUILD=${PBS_O_WORKDIR}/build_dspaces/bin
DS_SERVER=${PBS_O_HOME}/envs/Dataspacesroot/bin/dataspaces_server
PBS_RESULTDIR=${SCRATCH_DIR}/results


DS_CLIENT_PROCS=$((${PROCS_PRODUCER} + ${PROCS_CONSUMER}))

echo "${DS_CLIENT_PROCS} clients, $PROCS_SERVER server"

mkdir -pv ${PBS_RESULTDIR}
mkdir -pv ${SCRATCH_DIR}

## traces
SERVER_TRACE_DIR=${PBS_RESULTDIR}/server_trace
PRODUCER_TRACE_DIR=${PBS_RESULTDIR}/producer_trace
CONSUMER_TRACE_DIR=${PBS_RESULTDIR}/consumer_trace
mkdir -pv ${SERVER_TRACE_DIR}
mkdir -pv ${PRODUCER_TRACE_DIR}
mkdir -pv ${CONSUMER_TRACE_DIR}


cd ${SCRATCH_DIR}
cp -R ${PBS_O_WORKDIR}/adios_xmls ${SCRATCH_DIR}



## Clean up
rm -f conf *.log srv.lck
rm -f dataspaces.conf

## Create dataspaces configuration file
# note that we now have 400 regions

#dims = 5, 300000, 10
#dims = 2, 1500000, 1
# 64*64*256 will generate 1048576 lines
DS_LIMIT=$((${FILESIZE2PRODUCE}*${FILESIZE2PRODUCE}*${FILESIZE2PRODUCE}*${PROCS_PRODUCER}/16)) # make sure dspaces can hold all data

echo "total number of lines is $DS_LIMIT"

echo "## Config file for DataSpaces
ndim = 2
dims = 2, $((DS_LIMIT)) 
max_versions = 20
max_readers = 1
lock_type = 2
" > dataspaces.conf
echo "DS_LIMIT= $DS_LIMIT"
  
#export SLURM_NODEFILE=`generate_pbs_nodefile`
#nodes=(`cat $SLURM_NODEFILE | uniq`)
HOST_DIR=$PBS_RESULTDIR/hosts
mkdir -pv $HOST_DIR
rm -f $HOST_DIR/hostfile*
srun -o $HOST_DIR/hostfile-dup hostname
nodes=(`cat $HOST_DIR/hostfile-dup | sort |uniq`)

echo "${nodes[*]}" > $HOST_DIR/hostfile-all                                                                                                                                        
idx=0
# Put first $num_nodes_server to hostfile-server
#for i in {1..$num_nodes_server}
for ((i=0;i<$num_nodes_server;i++)) 
do
    echo "${nodes[$idx]}" >> $HOST_DIR/hostfile-server
    echo "node in server +1"
    let "idx=idx+1"
done

# Put the first $num_nodes_app1 nodes to hostfile-app1
#for i in {1..$num_nodes_app1}
for ((i=0;i<$num_nodes_app1;i++))
do
    echo "${nodes[$idx]}" >> $HOST_DIR/hostfile-app1
    echo "node in app1 +1"
    let "idx=idx+1"
done

# Put the next $num_nodes_app2 nodes to hostfile-app2
#for i in {1..$num_nodes_app2}
for ((i=0;i<$num_nodes_app2;i++))
do
    echo "node in app2 +1"
    echo "${nodes[$idx]}" >> $HOST_DIR/hostfile-app2
    let "idx=idx+1"
done


#LAUNCHER="ibrun -v"
LAUNCHER="mpirun_rsh"

## Run DataSpaces servers
CMD_SERVER="$LAUNCHER -hostfile $HOST_DIR/hostfile-server -n $PROCS_SERVER TAU_TRACE=1 TRACEDIR=${SERVER_TRACE_DIR} ${DS_SERVER} -s $PROCS_SERVER -c $DS_CLIENT_PROCS"
$CMD_SERVER  &> ${PBS_RESULTDIR}/server.log &
echo "server applciation lauched: $CMD_SERVER"
## Give some time for the servers to load and startup
while [ ! -f conf ]; do
    sleep 1s
done
sleep 5s  # wait server to fill up the conf file

#aprun -n 4 ${BUILD}/adios_write_global  &> ${PBS_RESULTDIR}/adios_write.log &

#Use ibrun to run the MPI job. It will detect the MPI, generate the hostfile
# and doing the right binding. With no options ibrun will use all cores.
#export OMP_NUM_THREADS=1
CMD_PRODUCER="$LAUNCHER -export -hostfile $HOST_DIR/hostfile-app1 -n $PROCS_PRODUCER TAU_TRACE=1 TRACEDIR=${PRODUCER_TRACE_DIR}  ${BIN_PRODUCER} ${NSTOP} ${FILESIZE2PRODUCE}"
$CMD_PRODUCER  &> ${PBS_RESULTDIR}/producer.log &
echo "producer applciation lauched: $CMD_PRODUCER"

CMD_CONSUMER="$LAUNCHER -export -hostfile $HOST_DIR/hostfile-app2 -n $PROCS_CONSUMER TAU_TRACE=1 TRACEDIR=${CONSUMER_TRACE_DIR} ${BIN_CONSUMER} ${NSTOP} ${FILESIZE2PRODUCE} ${PROCS_PRODUCER}"
$CMD_CONSUMER  &> ${PBS_RESULTDIR}/consumer.log &
echo " consumer applciation lauched $CMD_CONSUMER"

## Wait for the entire workflow to finish
wait
