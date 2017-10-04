#################################################### 
# common commands for all experiments 

BUILD_DIR=${PBS_O_WORKDIR}/build

BIN_PRODUCER=${BUILD_DIR}/bin/run_lbm;
BIN_CONSUMER=${BUILD_DIR}/bin/adios_staging_read;

#This job runs with 3 nodes  
#ibrun in verbose mode will give binding detail  #BUILD=${PBS_O_WORKDIR}/build_dspaces/bin
DS_SERVER=${PBS_O_HOME}/envs/Dataspacesroot/bin/dataspaces_server
PBS_RESULTDIR=${SCRATCH_DIR}/results

DS_CLIENT_PROCS=$((${PROCS_PRODUCER} + ${PROCS_CONSUMER}))

echo "${DS_CLIENT_PROCS} clients, $PROCS_SERVER server"

mkdir -pv ${PBS_RESULTDIR}
mkdir -pv ${SCRATCH_DIR}
cd ${SCRATCH_DIR}
cp -R ${PBS_O_WORKDIR}/adios_xmls ${SCRATCH_DIR}

## this is a clean working dir
#rm -f conf *.log srv.lck
#rm -f dataspaces.conf

## Create dataspaces configuration file
# note that we now have 400 regions

#dims = 5, 300000, 10
#dims = 2, 1500000, 1
# 64*64*256 will generate 1048576 lines
echo "total number of lines is being calculated"
DS_LIMIT=$((${FILESIZE2PRODUCE}*${FILESIZE2PRODUCE}*${FILESIZE2PRODUCE}*${PROCS_PRODUCER}/16)) # make sure dspaces can hold all data

echo "total number of lines is $DS_LIMIT"

echo "## Config file for DataSpaces
ndim = 2
dims = 2, $((DS_LIMIT))
max_versions = 5
max_readers = 1
# lock_type = 2
" > dataspaces.conf
echo "DS_LIMIT= $DS_LIMIT"
  

#LAUNCHER="ibrun -v"
LAUNCHER="ibrun"
echo "use transport method $CMTransport with CMTransportVerbose=$CMTransportVerbose"

## Run DataSpaces servers
CMD_SERVER="$LAUNCHER -n $PROCS_SERVER -o 0 ${DS_SERVER} -s $PROCS_SERVER -c $DS_CLIENT_PROCS"
$CMD_SERVER  &> ${PBS_RESULTDIR}/server.log &
echo "server applciation lauched: $CMD_SERVER"
## Give some time for the servers to load and startup
while [ ! -f conf ]; do
    sleep 1s
done
sleep 5s  # wait server to fill up the conf file

CMD_PRODUCER="$LAUNCHER -n $PROCS_PRODUCER -o $((${SLURM_NTASKS_PER_NODE}*${num_nodes_server})) ${BIN_PRODUCER} ${NSTOP} ${FILESIZE2PRODUCE}"
$CMD_PRODUCER  &> ${PBS_RESULTDIR}/producer.log &
echo "producer applciation lauched: $CMD_PRODUCER"

CMD_CONSUMER="$LAUNCHER -n $PROCS_CONSUMER -o $((${SLURM_NTASKS_PER_NODE}*${num_nodes_server}+${PROCS_PRODUCER})) ${BIN_CONSUMER}"
$CMD_CONSUMER  &> ${PBS_RESULTDIR}/consumer.log &
echo " consumer applciation lauched $CMD_CONSUMER"

## Wait for the entire workflow to finish
wait



