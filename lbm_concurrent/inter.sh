# # mkdir /N/dc2/scratch/fuyuan/concurrent/syn/mbexp001vs001/cid000
# writer_num=1
# reader_num=1
# block_size=1
# cpt_total_blks=1000
# writer_thousandth=2
# computer_group_size=1
# num_analysis_nodes=1
# # 64KB
# # aprun -n 2 -N 2 -d 4 ./concurrent $writer_num $reader_num $block_size $cpt_total_blks $writer_thousandth $computer_group_size $num_analysis_nodes

# # 8MB
# block_size=128
# writer_thousandth=300
# aprun -n 2 -N 2 -d 4 ./concurrent $writer_num $reader_num $block_size $cpt_total_blks $writer_thousandth $computer_group_size $num_analysis_nodes


directory="LBMcon030vs002"
date

generator_num=1
writer_num=1
reader_num=1
writer_thousandth=300
computer_group_size=15
num_compute_nodes=30
num_analysis_nodes=2
total_nodes=32
maxp=1

# directory="LBMcon004vs004"
# date

# generator_num=1
# writer_num=1
# reader_num=1
# writer_thousandth=300
# computer_group_size=1
# num_compute_nodes=4
# num_analysis_nodes=4
# total_nodes=8
# maxp=1

# directory="LBMcon002vs002"
# date

# generator_num=1
# writer_num=1
# reader_num=1
# writer_thousandth=300
# computer_group_size=1
# num_compute_nodes=2
# num_analysis_nodes=2
# total_nodes=4
# maxp=1

echo "------LBM_concurrent_No-Keep---------------"
echo "L-S code"
echo "Usage: %s $writer_num $reader_num ${block_size[i]} $cpt_total_blks  ${writer_thousandth[k]} $computer_group_size $num_analysis_nodes"
echo "Block size starting from 64KB,128KB,256KB,512KB,1MB,2MB,4MB,8MB"
echo "Block size input =	   1   ,2    ,4    ,8    ,16 ,32 ,64 ,128"
my_run_exp2="aprun -n $total_nodes -N 16 -d 2 /N/u/fuyuan/BigRed2/Openfoam/20160302_test/lbm_concurrent/lbmconcurrentnotstore"
# my_run_exp2="aprun -n $total_nodes -N 4 -d 8 /N/u/fuyuan/BigRed2/Openfoam/20160302_test/lbm_concurrent/lbmconcurrentnotstore"
# my_run_exp2="aprun -n $total_nodes -N 2 -d 16 /N/u/fuyuan/BigRed2/Openfoam/20160302_test/lbm_concurrent/lbmconcurrentnotstore"
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
	# lfs setstripe --count 2 -o -1 $(printf "/N/dc2/scratch/fuyuan/LBMconcurrent/$directory/cid%03g " $m)
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

# cubex=16
# cubez=16
# step_stop=2
# writer_thousandth=50
# lp=1
# val=`expr $cubex \* $cubex \* $cubez \* 16 / 1024`
# echo "********************************************************************"
# echo "Exp2 $val KB, $writer_thousandth/10% PRB"
# echo "********************************************************************"
# $my_run_exp2 $generator_num $writer_num $reader_num $writer_thousandth $computer_group_size $num_analysis_nodes $cubex $cubez $step_stop $lp
# echo "-----------Start Deleting files-------------"
# for ((m=0;m<$num_compute_nodes;m++)); do
#     $my_del_exp2 $(printf "/N/dc2/scratch/fuyuan/LBMconcurrent/$directory/cid%03g" $m)
# done
# echo "-----------End Delete files-------------"

# cubex=32
# step_stop=80
# writer_thousandth=(300)
# lp=4
# for((;cubex<=64;cubex=cubex*2));do
# 	for ((t=1; t<=4; t=t*2)); do
# 		cubez=`expr  $cubex \* $t`
# 		val=`expr $cubex \* $cubex \* $cubez \* 16 / 1024`

# 		if [ $val -eq 512 ]
#    		then
#       		continue
#    		fi

# 		if [ $val -gt 1024 ]
#    		then
#       		break
#    		fi

# 		for ((k=0; k<${#writer_thousandth[@]}; k++)); do
# 			echo
# 			echo
# 			echo "*************************************************************************************"
# 			echo "---LBM_concurrent_No-Keep $val KB, ${writer_thousandth[k]}/10%------"
# 			echo "*************************************************************************************"
# 			# if [ $val -eq 64 ] && [ $val -eq 128 ] && [ $val -eq 16384 ]
# 			#  		then
# 			#     		break
# 			#fi
# 			for ((p=0; p<$maxp; p++)); do
# 				echo "=============Loop $p==============="
# 				$my_run_exp2 $generator_num $writer_num $reader_num ${writer_thousandth[k]} $computer_group_size $num_analysis_nodes $cubex $cubez $step_stop $lp
# 				# echo "-----------Start Deleting files-------------"
# 				# for ((m=0;m<$num_compute_nodes;m++)); do
# 				#     $my_del_exp2 $(printf "/N/dc2/scratch/fuyuan/LBMconcurrent/$directory/cid%03g" $m)
# 				# done
# 				# echo "-----------End Delete files-------------"
# 				echo
# 			done
# 		done
# 	done
# done


# echo "-----------Start Deleting files-------------"
# # for ((m=0;m<$num_compute_nodes;m++)); do
# #     $my_del_exp2 $(printf "/N/dc2/scratch/fuyuan/LBMconcurrent/$directory/cid%03g" $m)
# # done
# echo "-----------End Delete files-------------"

echo "-------------------------------------"
echo "--- 2MB---------"
echo "-------------------------------------"

cubex=32
cubez=128
step_stop=80
writer_thousandth=(600)
lp=4
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
			lfs setstripe --count 2 -o -1 $(printf "/N/dc2/scratch/fuyuan/LBMconcurrent/$directory/cid%03g " $m)
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
