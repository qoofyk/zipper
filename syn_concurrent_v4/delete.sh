directory="mbexp008vs008"
num_compute_nodes=16
my_del_exp2='time rsync -a --delete-before  /N/dc2/scratch/fuyuan/empty/ '
echo "remove all subdirectories"
# $my_del_exp2 /N/dc2/scratch/fuyuan/concurrent/syn/$directory

echo "-----------Delete files-----------------"
for ((m=0;m<$num_compute_nodes;m++)); do
   $my_del_exp2 $(printf "/N/dc2/scratch/fuyuan/concurrent/syn/$directory/cid%03g" $m)
done
echo "-----------End Delete files-------------"
