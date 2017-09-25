BUILD_DIR=${PBS_O_WORKDIR}/build

BIN_PRODUCER=${BUILD_DIR}/bin/run_lbm;

#This job runs with 3 nodes  
#ibrun in verbose mode will give binding detail  
#BUILD=${PBS_O_WORKDIR}/build_dspaces/bin
#PBS_RESULTDIR=${PBS_O_WORKDIR}/results/${SLURM_JOBID}
PBS_RESULTDIR=${SCRATCH_DIR}/results


mkdir -pv ${PBS_RESULTDIR}
mkdir -pv ${SCRATCH_DIR}

cd ${SCRATCH_DIR}
cp -R ${PBS_O_WORKDIR}/adios_xmls ${SCRATCH_DIR}


#export SLURM_NODEFILE=`generate_pbs_nodefile`
#nodes=(`cat $SLURM_NODEFILE | uniq`)


HOST_DIR=$PBS_RESULTDIR/hosts
mkdir -pv $HOST_DIR
rm -f $HOST_DIR/hostfile*
srun -o $HOST_DIR/hostfile-dup hostname
nodes=(`cat $HOST_DIR/hostfile-dup | sort |uniq`)

echo "${nodes[*]}" > $HOST_DIR/hostfile-all                                                                                                                                        
idx=0

# Put the first $num_nodes_app1 nodes to hostfile-app1
#for i in {1..$num_nodes_app1}
for ((i=0;i<$num_nodes_app1;i++))
do
    echo "${nodes[$idx]}" >> $HOST_DIR/hostfile-app1
    echo "node in app1 +1"
    let "idx=idx+1"
done


#LAUNCHER="ibrun -v"
LAUNCHER="mpirun_rsh"

CMD_PRODUCER="$LAUNCHER -hostfile $HOST_DIR/hostfile-app1 -n $PROCS_PRODUCER ${BIN_PRODUCER} ${NSTOP} ${FILESIZE2PRODUCE} ${SCRATCH_DIR}"
$CMD_PRODUCER  &> ${PBS_RESULTDIR}/producer.log &
echo "producer applciation lauched: $CMD_PRODUCER"

## Wait for the entire workflow to finish
wait
