directory="LBMcon512vs128"
date

generator_num=1
writer_num=1
reader_num=1
computer_group_size=4
num_compute_nodes=512
num_analysis_nodes=128
total_nodes=640
maxp=1
lp=4
step_stop=80

echo "------LBM-NoStore--LBMNoKeep512v128P10---------------"
echo "------LBM_concurrent_dont_store---------------"
echo "lp=$lp,step_stop=$step_stop"
echo "Usage: %s $writer_num $reader_num ${block_size[i]} $cpt_total_blks  ${writer_thousandth[k]} $computer_group_size $num_analysis_nodes"
echo "Block size starting from 64KB,128KB,256KB,512KB,1MB,2MB,4MB,8MB"
echo "Block size input =	   1   ,2    ,4    ,8    ,16 ,32 ,64 ,128"
my_run_exp2="aprun -n $total_nodes -N 8 -d 4 /N/u/fuyuan/BigRed2/Openfoam/20160302_test/lbm_concurrent/lbmconcurrentnotstore"
my_del_exp2='time rsync -a --delete-before  /N/dc2/scratch/fuyuan/empty/ '

# rm -rf /N/dc2/scratch/fuyuan/concurrent/syn/$directory/
mkdir /N/dc2/scratch/fuyuan/LBMconcurrent/$directory/

echo "remove all subdirectories"
echo "-----------Delete files-----------------"
# for ((m=0;m<$num_compute_nodes;m++)); do
#     $my_del_exp2 $(printf "/N/dc2/scratch/fuyuan/LBMconcurrent/$directory/cid%03g" $m)
# done
echo "-----------End Delete files-------------"
# $my_del_exp2  /N/dc2/scratch/fuyuan/LBMconcurrent/$directory/
echo "mkdir new"
for ((m=0;m<$num_compute_nodes; m++)); do
	mkdir $(printf "/N/dc2/scratch/fuyuan/LBMconcurrent/$directory/cid%03g " $m)
	# lfs setstripe --count 1 -o -1 $(printf "/N/dc2/scratch/fuyuan/LBMconcurrent/$directory/cid%03g " $m)
done
# "/N/dc2/scratch/fuyuan/mb/lbmmix/mbexp%03dvs%03d/cid%03d/cid%03dthrd%02dblk%d.d"


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



echo "-------------------------------------"
echo "--- 512KB 1MB---------"
echo "-------------------------------------"

cubex=32
cubez=32
writer_thousandth=(0 100 200 300)
# writer_thousandth=(500)
val=`expr $cubex \* $cubex \* $cubez \* 16 / 1024`
for ((k=0; k<${#writer_thousandth[@]}; k++)); do
	echo
	echo
	echo "*************************************************************************************"
	echo "---LBM_concurrent_No-Keep $val KB, ${writer_thousandth[k]}/10%------"
	echo "*************************************************************************************"
	# if [ $val -eq 64 ] && [ $val -eq 128 ] && [ $val -eq 16384 ]
	#  		then
	#     		break
	#fi
	for ((p=0; p<$maxp; p++)); do
		echo "=============Loop $p==============="
		for ((m=0;m<$num_compute_nodes; m++)); do
			lfs setstripe --count 1 -o -1 $(printf "/N/dc2/scratch/fuyuan/LBMconcurrent/$directory/cid%03g " $m)
		done
		$my_run_exp2 $generator_num $writer_num $reader_num ${writer_thousandth[k]} $computer_group_size $num_analysis_nodes $cubex $cubez $step_stop $lp
		# echo "-----------Start Deleting files-------------"
		# for ((m=0;m<$num_compute_nodes;m++)); do
		#     $my_del_exp2 $(printf "/N/dc2/scratch/fuyuan/LBMconcurrent/$directory/cid%03g" $m)
		# done
		# echo "-----------End Delete files-------------"
		# echo
	done
done

# cubex=32
# cubez=64
# writer_thousandth=(200 300)
# val=`expr $cubex \* $cubex \* $cubez \* 16 / 1024`
# for ((k=0; k<${#writer_thousandth[@]}; k++)); do
# 	echo
# 	echo
# 	echo "*************************************************************************************"
# 	echo "---LBM_concurrent_No-Keep $val KB, ${writer_thousandth[k]}/10%------"
# 	echo "*************************************************************************************"
# 	# if [ $val -eq 64 ] && [ $val -eq 128 ] && [ $val -eq 16384 ]
# 	#  		then
# 	#     		break
# 	#fi
# 	for ((p=0; p<$maxp; p++)); do
# 		echo "=============Loop $p==============="
# 		for ((m=0;m<$num_compute_nodes; m++)); do
# 			lfs setstripe --count 1 -o -1 $(printf "/N/dc2/scratch/fuyuan/LBMconcurrent/$directory/cid%03g " $m)
# 		done
# 		$my_run_exp2 $generator_num $writer_num $reader_num ${writer_thousandth[k]} $computer_group_size $num_analysis_nodes $cubex $cubez $step_stop $lp
# 		# echo "-----------Start Deleting files-------------"
# 		# for ((m=0;m<$num_compute_nodes;m++)); do
# 		#     $my_del_exp2 $(printf "/N/dc2/scratch/fuyuan/LBMconcurrent/$directory/cid%03g" $m)
# 		# done
# 		# echo "-----------End Delete files-------------"
# 		# echo
# 	done
# done


# echo "-------------------------------------"
# echo "--- 2MB---------"
# echo "-------------------------------------"

# cubex=32
# cubez=128
# writer_thousandth=(300)
# val=`expr $cubex \* $cubex \* $cubez \* 16 / 1024`
# for ((k=0; k<${#writer_thousandth[@]}; k++)); do
# 	echo
# 	echo
# 	echo "*************************************************************************************"
# 	echo "---LBM_concurrent_No-Keep $val KB, ${writer_thousandth[k]}/10%------"
# 	echo "*************************************************************************************"
# 	# if [ $val -eq 64 ] && [ $val -eq 128 ] && [ $val -eq 16384 ]
# 	#  		then
# 	#     		break
# 	#fi
# 	for ((p=0; p<$maxp; p++)); do
# 		echo "=============Loop $p==============="
# 		for ((m=0;m<$num_compute_nodes; m++)); do
# 			lfs setstripe --count 2 -o -1 $(printf "/N/dc2/scratch/fuyuan/LBMconcurrent/$directory/cid%03g " $m)
# 		done
# 		$my_run_exp2 $generator_num $writer_num $reader_num ${writer_thousandth[k]} $computer_group_size $num_analysis_nodes $cubex $cubez $step_stop $lp
# 		# echo "-----------Start Deleting files-------------"
# 		# for ((m=0;m<$num_compute_nodes;m++)); do
# 		#     $my_del_exp2 $(printf "/N/dc2/scratch/fuyuan/LBMconcurrent/$directory/cid%03g" $m)
# 		# done
# 		# echo "-----------End Delete files-------------"
# 		# echo
# 	done
# done

# echo "-------------------------------------"
# echo "--- 4MB 8MB---------"
# echo "-------------------------------------"

# cubex=64
# cubez=64
# writer_thousandth=(0 50 100 200 300)
# val=`expr $cubex \* $cubex \* $cubez \* 16 / 1024`
# for ((k=0; k<${#writer_thousandth[@]}; k++)); do
# 	echo
# 	echo
# 	echo "*************************************************************************************"
# 	echo "---LBM_concurrent_No-Keep $val KB, ${writer_thousandth[k]}/10%------"
# 	echo "*************************************************************************************"
# 	# if [ $val -eq 64 ] && [ $val -eq 128 ] && [ $val -eq 16384 ]
# 	#  		then
# 	#     		break
# 	#fi
# 	for ((p=0; p<$maxp; p++)); do
# 		echo "=============Loop $p==============="
# 		for ((m=0;m<$num_compute_nodes; m++)); do
# 			lfs setstripe --count 4 -o -1 $(printf "/N/dc2/scratch/fuyuan/LBMconcurrent/$directory/cid%03g " $m)
# 		done
# 		$my_run_exp2 $generator_num $writer_num $reader_num ${writer_thousandth[k]} $computer_group_size $num_analysis_nodes $cubex $cubez $step_stop $lp
# 		# echo "-----------Start Deleting files-------------"
# 		# for ((m=0;m<$num_compute_nodes;m++)); do
# 		#     $my_del_exp2 $(printf "/N/dc2/scratch/fuyuan/LBMconcurrent/$directory/cid%03g" $m)
# 		# done
# 		# echo "-----------End Delete files-------------"
# 		# echo
# 	done
# done

# cubex=64
# cubez=128
# writer_thousandth=(0 50 100 200 300)
# val=`expr $cubex \* $cubex \* $cubez \* 16 / 1024`
# for ((k=0; k<${#writer_thousandth[@]}; k++)); do
# 	echo
# 	echo
# 	echo "*************************************************************************************"
# 	echo "---LBM_concurrent_No-Keep $val KB, ${writer_thousandth[k]}/10%------"
# 	echo "*************************************************************************************"
# 	# if [ $val -eq 64 ] && [ $val -eq 128 ] && [ $val -eq 16384 ]
# 	#  		then
# 	#     		break
# 	#fi
# 	for ((p=0; p<$maxp; p++)); do
# 		echo "=============Loop $p==============="
# 		for ((m=0;m<$num_compute_nodes; m++)); do
# 			lfs setstripe --count 4 -o -1 $(printf "/N/dc2/scratch/fuyuan/LBMconcurrent/$directory/cid%03g " $m)
# 		done
# 		$my_run_exp2 $generator_num $writer_num $reader_num ${writer_thousandth[k]} $computer_group_size $num_analysis_nodes $cubex $cubez $step_stop $lp
# 		# echo "-----------Start Deleting files-------------"
# 		# for ((m=0;m<$num_compute_nodes;m++)); do
# 		#     $my_del_exp2 $(printf "/N/dc2/scratch/fuyuan/LBMconcurrent/$directory/cid%03g" $m)
# 		# done
# 		# echo "-----------End Delete files-------------"
# 		# echo
# 	done
# done
