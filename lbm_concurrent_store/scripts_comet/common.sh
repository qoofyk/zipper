####################################################
# common commands for all experiments

date
export MV2_ENABLE_AFFINITY=0

echo "Block size starting from 64KB,128KB,256KB,512KB,1MB,2MB,4MB,8MB"
echo "Block size input =	   1   ,2    ,4    ,8    ,16 ,32 ,64 ,128"

my_run_exp2="ibrun --verbose -np $total_proc $EXE"
my_del_exp2='time rsync -a --delete-before ${SCRATCH_DIR}/empty/ '

# rm -rf ${SCRATCH_DIR}/$directory/

echo "remove all subdirectories"
date
echo "-----------Delete files-----------------"
# for ((m=0;m<$num_comp_proc;m++)); do
#     $my_del_exp2 $(printf "${SCRATCH_DIR}/$directory/cid%03g" $m)
# done
echo "-----------End Delete files-------------"
# $my_del_exp2  ${SCRATCH_DIR}/$directory/
date

mkdir -pv $SCRATCH_DIR
lfs setstripe --stripe-size 1m --count ${tune_stripe_count} $SCRATCH_DIR

echo "mkdir new"
for ((m=0;m<$num_comp_proc; m++)); do
	mkdir -pv $(printf "${SCRATCH_DIR}/$directory/cid%03g " $m)
	# lfs setstripe --stripe-size 1m --count 4 $(printf "${SCRATCH_DIR}/$directory/cid%03g" $m)
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
# $my_run_exp2 $compute_generator_num $compute_writer_num $analysis_reader_num $analysis_writer_num $writer_thousandth $compute_group_size $num_ana_proc $cubex $cubez $NSTOP $lp $FILESIZE2PRODUCE
# echo "-----------Start Deleting files-------------"
# # for ((m=0;m<$num_comp_proc;m++)); do
# #     $my_del_exp2 $(printf "/N/dc2/scratch/fuyuan/LBMconcurrentstore/$directory/cid%03g" $m)
# # done
# echo "-----------End Delete files-------------"

# cubex=64
# cubez=64
# NSTOP=100
# writer_thousandth=0
# lp=4
# val=$((${cubex} * ${cubex} * ${cubez} * 16 / 1024))
# echo "********************************************************************"
# echo "LBMconcurrentstore $val KB, $((${writer_thousandth}/10))% PRB"
# echo "********************************************************************"
# $my_run_exp2 $compute_generator_num $compute_writer_num $analysis_reader_num $analysis_writer_num $writer_thousandth $compute_group_size $num_ana_proc $cubex $cubez $NSTOP $lp $FILESIZE2PRODUCE
# echo "-----------Start Deleting files-------------"
# # for ((m=0;m<$num_comp_proc;m++)); do
# #     $my_del_exp2 $(printf "/N/dc2/scratch/fuyuan/LBMconcurrentstore/$directory/cid%03g" $m)
# # done
# echo "-----------End Delete files-------------"

# cubex=64
# cubez=128
# NSTOP=100
# writer_thousandth=0
# lp=4
# val=$((${cubex} * ${cubex} * ${cubez} * 16 / 1024))
# echo "********************************************************************"
# echo "LBMconcurrentstore $val KB, $((${writer_thousandth}/10))% PRB"
# echo "********************************************************************"
# $my_run_exp2 $compute_generator_num $compute_writer_num $analysis_reader_num $analysis_writer_num $writer_thousandth $compute_group_size $num_ana_proc $cubex $cubez $NSTOP $lp $FILESIZE2PRODUCE
# echo "-----------Start Deleting files-------------"
# # for ((m=0;m<$num_comp_proc;m++)); do
# #     $my_del_exp2 $(printf "/N/dc2/scratch/fuyuan/LBMconcurrentstore/$directory/cid%03g" $m)
# # done
# echo "-----------End Delete files-------------"

# done


for((;cubex<=64;cubex=cubex*2));do
	for ((t=1; t<=4; t=t*2)); do
		cubez=$((${cubex} * ${t}))
		val=$((${cubex} * ${cubex} * ${cubez} * 16 / 1024))

		if [ $val -eq 64 ]
   		then
      		continue
   		fi

		if [ $val -eq 16384 ]
   		then
      		break
   		fi

		for ((k=0; k<${#writer_thousandth[@]}; k++)); do
			echo
			echo
			echo "*************************************************************************************"
			echo "---case=$CASE_NAME $val KB, $((${writer_thousandth[k]}/10))% PRB------"
			echo "*************************************************************************************"
			# if [ $val -eq 64 ] && [ $val -eq 128 ] && [ $val -eq 16384 ]
			#  		then
			#     		break
			#fi
			for ((p=0; p<$maxp; p++)); do
				echo "=============Loop $p==============="
				$my_run_exp2 $compute_generator_num $compute_writer_num $analysis_reader_num $analysis_writer_num ${writer_thousandth[k]} $compute_group_size $num_ana_proc $cubex $cubez $NSTOP $lp $FILESIZE2PRODUCE
				# echo "-----------Start Deleting files-------------"
				# for ((m=0;m<$num_comp_proc;m++)); do
				#     $my_del_exp2 $(printf "/N/dc2/scratch/fuyuan/LBMconcurrentstore/$directory/cid%03g" $m)
				# done
				# echo "-----------End Delete files-------------"
				echo
			done
		done
	done
done

date

echo "-----------Start Deleting files-------------"
# for ((m=0;m<$num_comp_proc;m++)); do
#     $my_del_exp2 $(printf "/N/dc2/scratch/fuyuan/LBMconcurrentstore/$directory/cid%03g" $m)
# done
echo "-----------End Delete files-------------"


