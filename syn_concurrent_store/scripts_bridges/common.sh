####################################################
# common commands for all experiments

# proc_per_node=12

date
#MPI_Init_thread(Multiple level)
export MV2_ENABLE_AFFINITY=0
#Turn on Debug Info on Bridges
# export PGI_ACC_NOTIFY=3


# find number of threads for OpenMP
# find number of MPI tasks per node
echo "SLURM_TASKS_PER_NODE=$SLURM_TASKS_PER_NODE"
TPN=`echo $SLURM_TASKS_PER_NODE | cut -d '(' -f 1`
echo "TPN=$TPN"
# find number of CPU cores per node
echo "SLURM_JOB_CPUS_PER_NODE=$SLURM_JOB_CPUS_PER_NODE"
PPN=`echo $SLURM_JOB_CPUS_PER_NODE | cut -d '(' -f 1`
echo "PPN=$PPN"
THREADS=$(( PPN / TPN ))
export OMP_NUM_THREADS=$THREADS

echo "TPN=$TPN, PPN=$PPN, THREADS=$THREADS"

# export OMP_NUM_THREADS=28

echo "OMP_NUM_THREADS=$OMP_NUM_THREADS"


echo "Block size starting from 64KB,128KB,256KB,512KB,1MB,2MB,4MB,8MB"
echo "Block size input =	   1   ,2    ,4    ,8    ,16 ,32 ,64 ,128"

my_run_exp1="mpirun -genv OMP_NUM_THREADS $OMP_NUM_THREADS -genv MV2_ENABLE_AFFINITY 0 -genv KMP_AFFINITY verbose,granularity=core,compact,1,0 -np $total_proc $EXE1"
my_run_exp2="mpirun -genv OMP_NUM_THREADS $OMP_NUM_THREADS -genv MV2_ENABLE_AFFINITY 0 -genv KMP_AFFINITY verbose,granularity=core,compact,1,0 -np $total_proc $EXE2"
my_del_exp2="time rsync -a --delete-before ${EMPTY_DIR} "

# rm -rf ${SCRATCH_DIR}/${directory}/

echo "remove all subdirectories"
date
echo "-----------Delete files-----------------"
# for ((m=0;m<$num_comp_proc;m++)); do
#     $my_del_exp2 $(printf "${SCRATCH_DIR}/${directory}/cid%04g" $m)
# done
echo "-----------End Delete files-------------"
# $my_del_exp2  ${SCRATCH_DIR}/$directory/
date

mkdir -pv ${OUTPUT_DIR}
lfs setstripe --stripe-size 1m --stripe-count ${tune_stripe_count} ${OUTPUT_DIR}

echo "mkdir new"
for ((m=0;m<$num_comp_proc; m++)); do
	mkdir -pv $(printf "${OUTPUT_DIR}/cid%04g " $m)
	# lfs setstripe --stripe-size 1m --count 4 $(printf "${SCRATCH_DIR}/$directory/cid%04g" $m)
done


echo
echo
echo "####### Simulate $num_comp_proc Compute Parallel Write vs $num_ana_proc Analysis Parallel Read ########"

echo "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
echo "***********$num_comp_proc Compute Write*******************************"
echo "Block size starting from 64KB,128KB,256KB,512KB,1MB,2MB,4MB,8MB"
echo "cubex =	   			   16  ,16   ,16   ,32   ,32 ,32 ,64 ,64"
echo "cubez =	   			   16  ,32   ,64   ,32   ,64 ,128,64 ,128"
echo "*********************$num_ana_proc Analysis Read**********************"
echo "Block size starting from 64KB,128KB,256KB,512KB,1MB,2MB,4MB,8MB"
echo "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"

# for ((p=0; p<$maxp; p++)); do
# cubex=32
# cubez=128
# NSTOP=100
# writer_thousandth=0
# lp=4
# val=$((${cubex} * ${cubex} * ${cubez} * 16 / 1024))
# echo "********************************************************************"
# echo "LBMconcurrentstore $val KB, $((${writer_thousandth}/10))% PRB"
# echo "********************************************************************"
# $my_run_exp1 $compute_generator_num $compute_writer_num $analysis_reader_num $analysis_writer_num $writer_thousandth $compute_group_size $num_ana_proc $cubex $cubez $NSTOP $lp $FILESIZE2PRODUCE
# echo "-----------Start Deleting files-------------"
# # for ((m=0;m<$num_comp_proc;m++)); do
# #     $my_del_exp2 $(printf "/N/dc2/scratch/fuyuan/LBMconcurrentstore/$directory/cid%03g" $m)
# # done
# echo "-----------End Delete files-------------"


echo "~~~~~~~~~~~~~~~~~~Write_one_file~~~~~~~~~~~~~~~~~~~~~~~"
for((i=0;i<${#block_size1[@]};i++));do
	for ((k=0; k<${#writer_thousandth[@]}; k++)); do
		echo
		echo
		echo
		echo
		echo
		echo
		val=$(( ${block_size1[i]} * 64 ))
		echo "*************************************************************************************"
		echo "---case=$CASE_NAME $val KB, cpt_total_blks=${cpt_total_blks1[i]}, Hint=$((${writer_thousandth[k]}/10))% Use Disk, Writer_PRB=${writer_prb_thousandth[k]} /1000------"
		echo "*************************************************************************************"
		# if [ $val -eq 64 ] && [ $val -eq 128 ] && [ $val -eq 16384 ]
		#  		then
		#     		break
		#fi
		for ((p=0; p<$maxp; p++)); do
			echo "=============Loop $p==============="
			# echo "utime=${utime1[i]}"
			echo "setstripe $tune_stripe_count"
			echo "computation_lp=${computation_lp[i]}"
			echo "-----------------------------------"
			$my_run_exp1 $compute_generator_num $compute_writer_num $analysis_reader_num $analysis_writer_num ${block_size1[i]} ${cpt_total_blks1[i]} ${writer_thousandth[k]} $compute_group_size $num_ana_proc $lp ${computation_lp[i]} ${writer_prb_thousandth[k]}
			sleep 5
			echo "-----------Start Deleting files-------------"
			for ((m=0;m<$num_comp_proc;m++)); do
			    $my_del_exp2 $(printf "${OUTPUT_DIR}/cid%04g" $m)
			done
			echo "-----------End Delete files-------------"
			echo

		done
	done
done


echo
echo
echo
echo
echo
echo
echo
echo
echo
echo
echo
echo
echo
echo
echo
echo
echo
echo
echo
echo
echo
echo
echo
echo
echo "~~~~~~~~~~~~~~~~~~Write_sep_file~~~~~~~~~~~~~~~~~~~~~~~"
for((i=0;i<${#block_size2[@]};i++));do
	for ((k=0; k<${#writer_thousandth[@]}; k++)); do
		echo
		echo
		echo
		echo
		val=$(( ${block_size2[i]} * 64 ))
		echo "*************************************************************************************"
		echo "---case=$CASE_NAME $val KB, cpt_total_blks=${cpt_total_blks2[i]}, Hint=$((${writer_thousandth[k]}/10))% Use Disk, Writer_PRB=${writer_prb_thousandth[k]} /1000------"
		echo "*************************************************************************************"
		# if [ $val -eq 64 ] && [ $val -eq 128 ] && [ $val -eq 16384 ]
		#  		then
		#     		break
		#fi
		for ((p=0; p<$maxp; p++)); do
			echo "=============Loop $p==============="
			echo "setstripe $tune_stripe_count"
			echo "computation_lp=${computation_lp[i]}"
			echo "-----------------------------------"
			$my_run_exp2 $compute_generator_num $compute_writer_num $analysis_reader_num $analysis_writer_num ${block_size2[i]} ${cpt_total_blks2[i]} ${writer_thousandth[k]} $compute_group_size $num_ana_proc $lp ${computation_lp[i]} ${writer_prb_thousandth[k]}
			sleep 5
			echo "-----------Start Deleting files-------------"
			for ((m=0;m<$num_comp_proc;m++)); do
				ls -1 $(printf "${OUTPUT_DIR}/cid%04g" $m) | wc -l
			    $my_del_exp2 $(printf "${OUTPUT_DIR}/cid%04g" $m)
			done
			echo "-----------End Delete files-------------"
			echo
		done
	done
done

date

echo "-----------Start Deleting files-------------"
# for ((m=0;m<$num_comp_proc;m++)); do
#     $my_del_exp2 $(printf "/N/dc2/scratch/fuyuan/LBMconcurrentstore/$directory/cid%04g" $m)
# done
echo "-----------End Delete files-------------"


