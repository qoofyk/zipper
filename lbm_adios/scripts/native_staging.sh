####################################################
# common commands for all experiments
module list

#export  I_MPI_JOB_RESPECT_PROCESS_PLACEMENT=0

echo "case=$CASE_NAME datasize=$FILESIZE2PRODUCE nstops=$NSTOP, HASKEEP=${HAS_KEEP}"

echo "procs is \[ ${procs_this_app[*]}\], nodes is \[${nodes_this_app[*]}\]"

BUILD_DIR=${PBS_O_WORKDIR}/build

BIN_PRODUCER=${BUILD_DIR}/bin/run_lbm;
BIN_CONSUMER=${BUILD_DIR}/bin/native_staging_read;

#This job runs with 3 nodes  
#ibrun in verbose mode will give binding detail  #BUILD=${PBS_O_WORKDIR}/build_dspaces/bin
DS_SERVER=${WORK}/envs/Dataspacesroot/bin/dataspaces_server
PBS_RESULTDIR=${SCRATCH_DIR}/results

mkdir -pv ${PBS_RESULTDIR}
tune_stripe_count=-1
lfs setstripe --stripe-size 1m --stripe-count ${tune_stripe_count} ${PBS_RESULTDIR}
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
DS_LIMIT=$((${FILESIZE2PRODUCE}*${FILESIZE2PRODUCE}*${FILESIZE2PRODUCE}*${procs_this_app[1]}/16)) # make sure dspaces can hold all data


echo "total number of lines is $DS_LIMIT"

echo "## Config file for DataSpaces
ndim = 2
dims = 2, $((DS_LIMIT)) 
max_versions = 20
max_readers = 1
lock_type = 2
" > dataspaces.conf
echo "DS_LIMIT= $DS_LIMIT"

# this scripts is avaliable at
GENERATE_HOST_SCRIPT=${HOME}/Downloads/LaucherTest/generate_hosts.sh
if [ -a $GENERATE_HOST_SCRIPT ]; then
    source $GENERATE_HOST_SCRIPT
else
    echo "generate_hosts.sh should downloaded from:"
    echo "https://github.iu.edu/lifen/LaucherTest/blob/master/generate_hosts.sh"
fi

LAUNCHER="mpiexec.hydra"

## Run DataSpaces servers
CMD_SERVER="$LAUNCHER -np ${procs_this_app[0]} -machinefile $HOST_DIR/machinefile-app0 ${DS_SERVER} -s ${procs_this_app[0]} -c $((procs_this_app[1]+procs_this_app[2]))"
$CMD_SERVER  &> ${PBS_RESULTDIR}/server.log &
echo "server applciation lauched: $CMD_SERVER"
## Give some time for the servers to load and startup
while [ ! -f conf ]; do
    sleep 1s
done
sleep 5s  # wait server to fill up the conf file

CMD_PRODUCER="$LAUNCHER -np ${procs_this_app[1]} -machinefile $HOST_DIR/machinefile-app1  ${BIN_PRODUCER} ${NSTOP} ${FILESIZE2PRODUCE}"
$CMD_PRODUCER  &> ${PBS_RESULTDIR}/producer.log &
echo "producer applciation lauched: $CMD_PRODUCER"

CMD_CONSUMER="$LAUNCHER -np ${procs_this_app[2]} -machinefile $HOST_DIR/machinefile-app2 ${BIN_CONSUMER} ${NSTOP} ${FILESIZE2PRODUCE} ${procs_this_app[1]}"
$CMD_CONSUMER  &> ${PBS_RESULTDIR}/consumer.log &
echo " consumer applciation lauched $CMD_CONSUMER"

## Wait for the entire workflow to finish
wait

