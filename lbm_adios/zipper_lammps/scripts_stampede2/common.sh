#########################################################################################
# common commands for all experiments

date

export SCRATCH_DIR=$SCRATCH/lammps_benchtest/${SLURM_JOBID}
EMPTY_DIR=$SCRATCH/empty/
mkdir -pv ${EMPTY_DIR}

if [ x"$HAS_TRACE" = "x" ];then
    BUILD_DIR=${PBS_O_WORKDIR}/build

elif [ x"$HAS_TRACE" = "xitac" ]; then
    NSTOP=10
    export LD_PRELOAD=libVT.so
    export VT_CONFIG=${PBS_O_HOME}/itac_filter.conf
    echo "itac ENABLED, use 10 steps"
    export BUILD_DIR=${PBS_O_WORKDIR}/build_itac
    echo "use itac"
    export VT_LOGFILE_PREFIX=${SCRATCH_DIR}/trace
    mkdir -pv $VT_LOGFILE_PREFIX
else
    echo "TRACE ENABLED, use 10 steps"
    NSTOP=10
    export BUILD_DIR=${PBS_O_WORKDIR}/build_tau
    #enable trace
    export TAU_TRACE=1
    # set trace dir
    export ALL_TRACES=${SCRATCH_DIR}/trace
    mkdir -pv $ALL_TRACES/app0
    mkdir -pv $ALL_TRACES/app1
    mkdir -pv $ALL_TRACES/app2

    if [ -z $TAU_MAKEFILE ]; then
        module load tau
        echo "LOAD TAU!"
    fi

fi

BIN1=${BUILD_DIR}/bin/lmp_mpi_nokeep
BIN2=${BUILD_DIR}/bin/lmp_mpi_nokeep_sep_file

#prepare output result directory
PBS_RESULTDIR=${SCRATCH_DIR}/results
mkdir -pv ${PBS_RESULTDIR}
lfs setstripe --stripe-size 1m --stripe-count ${tune_stripe_count} ${PBS_RESULTDIR}

# echo "if use sepfile, mkdir new"
# for ((m=0;m<$num_comp_proc; m++)); do
# 	mkdir -pv $(printf "${PBS_RESULTDIR}/cid%04g " $m)
# 	# lfs setstripe --stripe-size 1m --count 4 $(printf "${SCRATCH_DIR}/cid%04g" $m)
# done

#generate hostfile
# HOST_DIR=$SCRATCH_DIR/hosts
# mkdir -pv $HOST_DIR
# rm -f $HOST_DIR/hostfile*
# #all tasks run the following command
# srun -o $HOST_DIR/hostfile-dup hostname
# cat $HOST_DIR/hostfile-dup | sort | uniq | sed "s/$/:${nproc_per_mac}/" >$HOST_DIR/hostfile-all
# cd ${PBS_RESULTDIR}

#SET TOTAL MPI PROC
export SLURM_NTASKS=$total_proc

#MPI_Init_thread(Multiple level)
export MV2_ENABLE_AFFINITY=0

#Turn on Debug Info on Bridges
# export PGI_ACC_NOTIFY=3

# print SLURM Environment variables
echo "SLURM_NNODES=$SLURM_NNODES" #Number of nodes allocated to job
# echo "SLURM_SUBMIT_HOST=$SLURM_SUBMIT_HOST" #Names of nodes allocated to job

# find number of threads for OpenMP
# find number of MPI tasks per node
echo "SLURM_TASKS_PER_NODE=$SLURM_TASKS_PER_NODE"
# TPN=`echo $SLURM_TASKS_PER_NODE | cut -d '(' -f 1`
# echo "TPN=$TPN"
# # find number of CPU cores per node
echo "SLURM_JOB_CPUS_PER_NODE=$SLURM_JOB_CPUS_PER_NODE"
PPN=`echo $SLURM_JOB_CPUS_PER_NODE | cut -d '(' -f 1`
echo "PPN=$PPN"
# THREADS=$(( PPN / TPN ))
# export OMP_NUM_THREADS=$THREADS

# echo "TPN=$TPN, PPN=$PPN, THREADS=$THREADS, lp=$lp, SLURM_NTASKS=$SLURM_NTASKS"

#
export OMP_NUM_THREADS=$(( PPN / nproc_per_mac ))
# export OMP_NUM_THREADS=2
echo "OMP_NUM_THREADS=$OMP_NUM_THREADS, SLURM_NTASKS=$SLURM_NTASKS"

# LAUNCHER="mpirun_rsh"
# my_run_exp1="$LAUNCHER -export -hostfile $HOST_DIR/hostfile-all -np $total_proc $BIN -in $inputfile"
# LAUNCHER="mpirun"
# my_run_exp1="$LAUNCHER -np $total_proc $BIN -in $inputfile"
LAUNCHER="ibrun"
my_run_exp1="$LAUNCHER -n $total_proc $BIN1 $NSTOP $inputfile"


echo "------lammps_data_broker_benchtest---------------"
echo "NSTOP=$NSTOP"
echo "Usage: %s $comp_writer_num $ana_reader_num $ana_writer_num ${block_size[i]} $cpt_total_blks ${writer_thousandth[k]} $compute_group_size $num_ana_proc"

echo "remove all subdirectories"
date
echo "-----------Start Deleting files-------------"
# for ((m=0;m<$num_comp_proc;m++)); do
# 	ls -1 $(printf "${PBS_RESULTDIR}/cid%04g" $m) | wc -l
#     $my_del_exp2 $(printf "${PBS_RESULTDIR}/cid%04g" $m)
# done
echo "-----------End Delete files-------------"
date


echo
echo
echo "------------ $num_comp_proc Compute vs $num_ana_proc Analysis Proc -----------"

# writer_thousandth=0
# val=$((${dump_lines_per_blk} * ${one_dump_blk_size} / 1024))
# echo "********************************************************************"
# echo "LAMMPS_Databroker $val KB, $((${writer_thousandth}/10))% PRB"
# echo "********************************************************************"
# $run $comp_gen_num $comp_writer_num $ana_reader_num $ana_writer_num $dump_lines_per_blk $dump_step_internal $writer_thousandth $compute_group_size $num_ana_proc $num_atom $NSTOP
# echo "-----------Start Deleting files-------------"
# # for ((m=0; m<$num_comp_proc;m++)); do
#     # $del $(printf "${SCRATCH_DIR}/lammps_benchtest/${directory}/cid%04g" $m)
# # done
# echo "-----------End Delete files-------------"
# echo
# echo
# echo

# writer_thousandth=100
# val=$((${dump_lines_per_blk} * ${one_dump_blk_size} / 1024))
# echo "********************************************************************"
# echo "LAMMPS_Databroker $val KB, $((${writer_thousandth}/10))% PRB"
# echo "********************************************************************"
# $run $comp_gen_num $comp_writer_num $ana_reader_num $ana_writer_num $dump_lines_per_blk $dump_step_internal $writer_thousandth $compute_group_size $num_ana_proc $num_atom $NSTOP
# echo "-----------Start Deleting files-------------"
# # for ((m=0; m<$num_comp_proc;m++)); do
#     # $del $(printf "${SCRATCH_DIR}/lammps_benchtest/${directory}/cid%04g" $m)
# # done
# echo "-----------End Delete files-------------"
# echo
# echo
# echo

# writer_thousandth=150
# val=$((${dump_lines_per_blk} * ${one_dump_blk_size} / 1024))
# echo "********************************************************************"
# echo "LAMMPS_Databroker $val KB, $((${writer_thousandth}/10))% PRB"
# echo "********************************************************************"
# $run $comp_gen_num $comp_writer_num $ana_reader_num $ana_writer_num $dump_lines_per_blk $dump_step_internal $writer_thousandth $compute_group_size $num_ana_proc $num_atom $NSTOP
# echo "-----------Start Deleting files-------------"
# # for ((m=0; m<$num_comp_proc;m++)); do
#     # $del $(printf "${SCRATCH_DIR}/lammps_benchtest/${directory}/cid%04g" $m)
# # done
# echo "-----------End Delete files-------------"
# echo
# echo
# echo

# writer_thousandth=200
# val=$((${dump_lines_per_blk} * ${one_dump_blk_size} / 1024))
# echo "********************************************************************"
# echo "LAMMPS_Databroker $val KB, $((${writer_thousandth}/10))% PRB"
# echo "********************************************************************"
# $run $comp_gen_num $comp_writer_num $ana_reader_num $ana_writer_num $dump_lines_per_blk $dump_step_internal $writer_thousandth $compute_group_size $num_ana_proc $num_atom $NSTOP
# echo "-----------Start Deleting files-------------"
# # for ((m=0; m<$num_comp_proc;m++)); do
#     # $del $(printf "${SCRATCH_DIR}/lammps_benchtest/${directory}/cid%04g" $m)
# # done
# echo "-----------End Delete files-------------"
# echo
# echo
# echo

for ((i=0; i<${#dump_lines_per_blk[@]}; i++)); do
	for ((k=0; k<${#writer_thousandth[@]}; k++)); do
		echo
		echo
		val=$((${dump_lines_per_blk[i]} * ${one_dump_blk_size} / 1024))
		echo "********************************************************************"
		echo "LAMMPS_Databroker $val KB, $((${writer_thousandth[k]}/10))% PRB"
		echo "********************************************************************"

		for ((p=0; p<$maxp; p++)); do
			date
			echo "=============Loop $p==============="
			echo "setstripe $tune_stripe_count"
			echo "-----------------------------------"
			export TRACEDIR=$(printf "${MYTRACE}/EXP%04g" $k)
			real_run="$my_run_exp1 $comp_gen_num $comp_writer_num $ana_reader_num $ana_writer_num ${dump_lines_per_blk[i]} $dump_step_internal ${writer_thousandth[k]} $compute_group_size $num_ana_proc $num_atom $NSTOP ${writer_prb_thousandth[k]}"
			# $real_run &>> ${PBS_RESULTDIR}/log
			$real_run
			echo "real_run=$real_run"
			date
			# sleep 8
			echo "-----------Start Deleting files-------------"
			# for ((m=0;m<$num_comp_proc;m++)); do
			#     $my_del_exp2 $(printf "${PBS_RESULTDIR}/cid%04g" $m)
			# done
			echo "-----------End Delete files-------------"
			echo
		done
	done
done

date
