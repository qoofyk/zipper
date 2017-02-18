directory="LBM120vs040"
date

cubex=16
producer=1
prefetcher=1
each_computer_group_size=3
num_compute_nodes=120
num_analysis_nodes=40
# cid_dir_num=1
total_nodes=160
lp=4
step_stop=80
maxj=1
echo "cubex, cubez, producer#, prefetcher#, step_stop, computer_group_size, num_analysis_nodes, lp=$lp,step_stop=$step_stop"
echo "Total File to produce ?GB, each compute node produce ?GB, Testing parallel write and read, each compute node generate $total_blks blocks"

my_run_exp2="aprun -n $total_nodes -N 4 -d 8 /N/home/f/u/fuyuan/BigRed2/Openfoam/20160302_test/lbm_disk/lbm_IObox"

# rm -rf /N/dc2/scratch/fuyuan/LBM/$directory/
mkdir /N/dc2/scratch/fuyuan/LBM
mkdir /N/dc2/scratch/fuyuan/LBM/$directory/

my_del_exp2='time rsync -a --delete-before  /N/dc2/scratch/fuyuan/empty/ '
# MODIFY {0..1}
echo "remove all subdirectories"
# MODIFY k<2
echo "-----------Delete files-----------------"
# for ((k=0;k<$total_nodes;k++)); do
#     $my_del_exp2 $(printf "/N/dc2/scratch/fuyuan/LBM/$directory/cid%03g" $k)
# done
echo "-----------End Delete files-------------"
# $my_del_exp2 /N/dc2/scratch/fuyuan/LBM/$directory/


val=0

echo
echo "####### Synthetic Application $num_compute_nodes Compute Parallel Write vs $num_analysis_nodes Analysis Parallel Read ########"

echo "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
echo "***********$num_compute_nodes Compute Write*******************************"
echo "Each compute node has 1 thread, each thread will write according to num_blks"
echo "Block size starting from 64KB,128KB,256KB,512KB,1MB,2MB,4MB,8MB"
echo "cubex =	   			   16  ,16   ,16   ,32   ,32 ,32 ,64 ,64"
echo "cubez =	   			   16  ,32   ,64   ,32   ,64 ,128,64 ,128"
echo "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
echo
echo "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
echo "*********************$num_analysis_nodes Analysis Read**********************"
echo "Each Analysis node has 1 thread, each thread will read according to num_blks"
echo "Block size starting from 64KB,128KB,256KB,512KB,1MB,2MB,4MB,8MB"
echo "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"

echo "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
echo "****************$num_compute_nodes Compute vs $num_analysis_nodes Ana**********************"
echo "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"

# cubez=16
# $my_run_exp2  $cubex $cubez $producer $prefetcher $step_stop $each_computer_group_size $num_analysis_nodes $lp

echo "mkdir new"
#mkdir $(printf "/N/dc2/scratch/fuyuan/LBM/$directory/cid%03g " {0..1})
for ((k=0;k<$num_compute_nodes; k++)); do
	mkdir $(printf "/N/dc2/scratch/fuyuan/LBM/$directory/cid%03g " $k)
	# lfs setstripe --count 1 -o -1 $(printf "/N/dc2/scratch/fuyuan/LBM/$directory/cid%03g " $k)
done
cubex=32
for((;cubex<=64;cubex=cubex*2));do
	for ((m=1; m<=4; m=m*2)); do
		cubez=`expr  $cubex \* $m`
		val=`expr $cubex \* $cubex \* $cubez \* 16 / 1024`

		if [ $val -eq 64 ]
   		then
      		continue
   		fi

		if [ $val -gt 512 ]
   		then
      		break
   		fi

		for ((j=0; j<$maxj; j++)); do
			echo
			echo "----------------------------------"
			echo "**********LBM Disk Keep $val KB************"
			echo "----------Loop $j-----------------"
			echo "----------------------------------"
			for ((k=0;k<$num_compute_nodes; k++)); do
				# mkdir $(printf "/N/dc2/scratch/fuyuan/LBM/$directory/cid%03g " $k)
				lfs setstripe --count 1 -o -1 $(printf "/N/dc2/scratch/fuyuan/LBM/$directory/cid%03g " $k)
			done
			$my_run_exp2  $cubex $cubez $producer $prefetcher $step_stop $each_computer_group_size $num_analysis_nodes $lp
			# MODIFY k<2
			# echo "-----------Delete files-----------------"
			# for ((k=0;k<$num_compute_nodes; k++)); do
			#     $my_del_exp2 $(printf "/N/dc2/scratch/fuyuan/LBM/$directory/cid%03g" $k)
			# done
			# echo "-----------End Delete files-------------"
			echo
		done
	done
done
echo
echo

# echo "-------------------------------------------"
# echo "2MB"
# echo "-------------------------------------------"
# # for ((k=0;k<$total_nodes; k++)); do
# # 	mkdir $(printf "/N/dc2/scratch/fuyuan/LBM/$directory/exp2/cid%03g " $k)
# # 	lfs setstripe --count 2 -o -1 $(printf "/N/dc2/scratch/fuyuan/LBM/$directory/exp2/cid%03g " $k)
# # done

# cubex=32
# cubez=128
# val=`expr $cubex \* $cubex \* $cubez \* 16 / 1024`
# for ((j=0; j<$maxj; j++)); do
# 	echo
# 	echo "----------------------------------"
# 	echo "**********LBM Disk Keep $val KB************"
# 	echo "----------Loop $j-----------------"
# 	echo "----------------------------------"
# 	for ((k=0;k<$num_compute_nodes; k++)); do
# 		# mkdir $(printf "/N/dc2/scratch/fuyuan/LBM/$directory/exp2/cid%03g " $k)
# 		lfs setstripe --count 2 -o -1 $(printf "/N/dc2/scratch/fuyuan/LBM/$directory/exp2/cid%03g " $k)
# 	done
# 	$my_run_exp2  $cubex $cubez $producer $prefetcher $step_stop $each_computer_group_size $num_analysis_nodes $lp
# 	# MODIFY k<2
# 	# echo "-----------Delete files-----------------"
# 	# for ((k=0;k<$num_compute_nodes; k++)); do
# 	#     $my_del_exp2 $(printf "/N/dc2/scratch/fuyuan/LBM/$directory/exp2/cid%03g" $k)
# 	# done
# 	# echo "-----------End Delete files-------------"
# 	echo
# done

# echo "-------------------------------------------"
# echo "4MB,8MB"
# echo "-------------------------------------------"
# # for ((k=0;k<$total_nodes; k++)); do
# # 	mkdir $(printf "/N/dc2/scratch/fuyuan/LBM/$directory/exp2/cid%03g " $k)
# # 	lfs setstripe --count 4 -o -1 $(printf "/N/dc2/scratch/fuyuan/LBM/$directory/exp2/cid%03g " $k)
# # done

# cubex=64
# cubez=64
# val=`expr $cubex \* $cubex \* $cubez \* 16 / 1024`
# for ((j=0; j<$maxj; j++)); do
# 	echo
# 	echo "----------------------------------"
# 	echo "**********LBM Disk Keep $val KB************"
# 	echo "----------Loop $j-----------------"
# 	echo "----------------------------------"
# 	for ((k=0;k<$num_compute_nodes; k++)); do
# 		# mkdir $(printf "/N/dc2/scratch/fuyuan/LBM/$directory/exp2/cid%03g " $k)
# 		lfs setstripe --count 4 -o -1 $(printf "/N/dc2/scratch/fuyuan/LBM/$directory/exp2/cid%03g " $k)
# 	done
# 	$my_run_exp2  $cubex $cubez $producer $prefetcher $step_stop $each_computer_group_size $num_analysis_nodes $lp
# 	# echo "-----------Delete files-----------------"
# 	# for ((k=0;k<$num_compute_nodes; k++)); do
# 	#     $my_del_exp2 $(printf "/N/dc2/scratch/fuyuan/LBM/$directory/exp2/cid%03g" $k)
# 	# done
# 	# echo "-----------End Delete files-------------"
# 	echo
# done

# cubex=64
# cubez=128
# val=`expr $cubex \* $cubex \* $cubez \* 16 / 1024`
# for ((j=0; j<$maxj; j++)); do
# 	echo
# 	echo "----------------------------------"
# 	echo "**********LBM Disk Keep $val KB************"
# 	echo "----------Loop $j-----------------"
# 	echo "----------------------------------"
# 	for ((k=0;k<$num_compute_nodes; k++)); do
# 		# mkdir $(printf "/N/dc2/scratch/fuyuan/LBM/$directory/exp2/cid%03g " $k)
# 		lfs setstripe --count 4 -o -1 $(printf "/N/dc2/scratch/fuyuan/LBM/$directory/exp2/cid%03g " $k)
# 	done
# 	$my_run_exp2  $cubex $cubez $producer $prefetcher $step_stop $each_computer_group_size $num_analysis_nodes $lp
# 	# echo "-----------Delete files-----------------"
# 	# for ((k=0;k<$num_compute_nodes; k++)); do
# 	#     $my_del_exp2 $(printf "/N/dc2/scratch/fuyuan/LBM/$directory/exp2/cid%03g" $k)
# 	# done
# 	# echo "-----------End Delete files-------------"
# 	echo
# done
