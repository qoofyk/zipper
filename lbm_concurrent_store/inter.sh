directory="LBMcon001vs001"
date

compute_generator_num=1
compute_writer_num=1
analysis_reader_num=1
analysis_writer_num=1
writer_thousandth=300
computer_group_size=1
num_compute_nodes=1
num_analysis_nodes=1
total_nodes=2
maxp=1

echo "------LBM_concurrent_store---------------"
echo "L-S code"
echo "Usage: %s $compute_writer_num $analysis_reader_num $analysis_writer_num $analysis_reader_num $cpt_total_blks ${writer_thousandth[k]} $computer_group_size $num_analysis_nodes"
echo "Block size starting from 64KB,128KB,256KB,512KB,1MB,2MB,4MB,8MB"
echo "Block size input =	   1   ,2    ,4    ,8    ,16 ,32 ,64 ,128"
my_run_exp2="aprun -n $total_nodes -N 2 -d 16 /N/u/fuyuan/BigRed2/Openfoam/20160302_test/lbm_concurrent_store/lbm_concurrent_store"
my_del_exp2='time rsync -a --delete-before  /N/dc2/scratch/fuyuan/empty/ '

# rm -rf /N/dc2/scratch/fuyuan/concurrent/syn/$directory/
mkdir /N/dc2/scratch/fuyuan/LBMconcurrentstore/$directory/
echo "remove all subdirectories"
$my_del_exp2  /N/dc2/scratch/fuyuan/LBMconcurrentstore/$directory/
echo "mkdir new"
for ((m=0;m<$num_compute_nodes; m++)); do
	mkdir $(printf "/N/dc2/scratch/fuyuan/LBMconcurrentstore/$directory/cid%03g " $m)
	lfs setstripe --count 4 -o -1 $(printf "/N/dc2/scratch/fuyuan/LBMconcurrentstore/$directory/cid%03g " $m)
done
# "/N/dc2/scratch/fuyuan/mb/lbmmix/mbexp%03dvs%03d/cid%03d/cid%03dthrd%02dblk%d.d"
echo "-----------Delete files-----------------"
for ((m=0;m<$num_compute_nodes;m++)); do
    $my_del_exp2 $(printf "/N/dc2/scratch/fuyuan/LBMconcurrentstore/$directory/cid%03g" $m)
done
echo "-----------End Delete files-------------"

echo
echo
echo "####### Simulate $num_compute_nodes Compute Parallel Write vs $num_analysis_nodes Analysis Parallel Read ########"

echo "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
echo "***********$num_compute_nodes Compute Write*******************************"
echo "Each compute node has 1 thread, each thread will write according to NUM_ITER and num_blks"
echo "Block size starting from 64KB,128KB,256KB,512KB,1MB,2MB,4MB,8MB"
echo "cubex =	   			   16  ,16   ,16   ,32   ,32 ,32 ,64 ,64"
echo "cubez =	   			   16  ,32   ,64   ,32   ,64 ,128,64 ,128"
echo "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
echo
echo "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
echo "*********************$num_analysis_nodes Analysis Read**********************"
echo "Each Analysis node has 1 thread, each thread will read according to NUM_ITER and num_blks"
echo "Block size starting from 64KB,128KB,256KB,512KB,1MB,2MB,4MB,8MB"
echo "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"

cubex=32
cubez=32
step_stop=5
writer_thousandth=50
lp=1
val=`expr $cubex \* $cubex \* $cubez \* 16 / 1024`
echo "********************************************************************"
echo "Exp2 $val KB, $writer_thousandth/10% PRB"
echo "********************************************************************"
$my_run_exp2 $compute_generator_num $compute_writer_num $analysis_reader_num $analysis_writer_num $writer_thousandth $computer_group_size $num_analysis_nodes $cubex $cubez $step_stop $lp
echo "-----------Start Deleting files-------------"
for ((m=0;m<$num_compute_nodes;m++)); do
    $my_del_exp2 $(printf "/N/dc2/scratch/fuyuan/LBMconcurrentstore/$directory/cid%03g" $m)
done
echo "-----------End Delete files-------------"

# cubex=32
# step_stop=6
# writer_thousandth=(0 50 100 200 300)
# lp=1
# for((;cubex<=64;cubex=cubex*2));do
# 	for ((t=1; t<=4; t=t*2)); do
# 		cubez=`expr  $cubex \* $t`
# 		val=`expr $cubex \* $cubex \* $cubez \* 16 / 1024`

# 		if [ $val -eq 16384 ]
#    		then
#       		break
#    		fi

# 		for ((k=0; k<${#writer_thousandth[@]}; k++)); do
# 			echo
# 			echo
# 			echo "*************************************************************************************"
# 			echo "---LBM_concurrent_store $val KB, ${writer_thousandth[k]}/10%------"
# 			echo "*************************************************************************************"
# 			# if [ $val -eq 64 ] && [ $val -eq 128 ] && [ $val -eq 16384 ]
# 			#  		then
# 			#     		break
# 			#fi
# 			for ((p=0; p<$maxp; p++)); do
# 				echo "=============Loop $p==============="
# 				$my_run_exp2 $compute_generator_num $compute_writer_num $analysis_reader_num $analysis_writer_num ${writer_thousandth[k]} $computer_group_size $num_analysis_nodes $cubex $cubez $step_stop $lp
# 				echo "-----------Start Deleting files-------------"
# 				for ((m=0;m<$num_compute_nodes;m++)); do
# 				    $my_del_exp2 $(printf "/N/dc2/scratch/fuyuan/LBMconcurrentstore/$directory/cid%03g" $m)
# 				done
# 				echo "-----------End Delete files-------------"
# 				echo
# 			done
# 		done
# 	done
# done
