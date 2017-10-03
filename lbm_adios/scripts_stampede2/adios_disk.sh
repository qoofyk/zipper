
BUILD_DIR=${PBS_O_WORKDIR}/build

BIN_PRODUCER=${BUILD_DIR}/bin/run_lbm;
BIN_CONSUMER=${BUILD_DIR}/bin/adios_disk_read;

#This job runs with 3 nodes  
#ibrun in verbose mode will give binding detail  
#BUILD=${PBS_O_WORKDIR}/build_dspaces/bin
PBS_RESULTDIR=${SCRATCH_DIR}/results

DS_CLIENT_PROCS=$((${PROCS_PRODUCER} + ${PROCS_CONSUMER}))


mkdir -pv ${PBS_RESULTDIR}
mkdir -pv ${SCRATCH_DIR}

cd ${SCRATCH_DIR}
cp -R ${PBS_O_WORKDIR}/adios_xmls ${SCRATCH_DIR}


## Create dataspaces configuration file
# note that we now have 400 regions

#dims = 5, 300000, 10
#dims = 2, 1500000, 1
# 64*64*256 will generate 1048576 lines
DS_LIMIT=$((${FILESIZE2PRODUCE}*${FILESIZE2PRODUCE}*${FILESIZE2PRODUCE}/16)) # make sure dspaces can hold all data

echo "total number of lines is $DS_LIMIT"

 

#LAUNCHER="ibrun -v"
LAUNCHER="ibrun"

#Use ibrun to run the MPI job. It will detect the MPI, generate the hostfile
# and doing the right binding. With no options ibrun will use all cores.
#export OMP_NUM_THREADS=1
CMD_PRODUCER="$LAUNCHER -n $PROCS_PRODUCER -o 0 ${BIN_PRODUCER} ${NSTOP} ${FILESIZE2PRODUCE}"
$CMD_PRODUCER  &> ${PBS_RESULTDIR}/producer.log &
echo "producer applciation lauched: $CMD_PRODUCER"

CMD_CONSUMER="$LAUNCHER -n $PROCS_CONSUMER -o $PROCS_PRODUCER ${BIN_CONSUMER} ${NSTOP}"
$CMD_CONSUMER  &> ${PBS_RESULTDIR}/consumer.log &
echo " consumer applciation lauched $CMD_CONSUMER"

## Wait for the entire workflow to finish
wait
