directory="LBMcon512vs128"
date
num_compute_nodes=512

my_del_exp2='time rsync -a --delete-before  /N/dc2/scratch/fuyuan/empty/ '

# rm -rf /N/dc2/scratch/fuyuan/concurrent/syn/$directory/
mkdir /N/dc2/scratch/fuyuan/LBMconcurrentstore/
mkdir /N/dc2/scratch/fuyuan/LBMconcurrentstore/$directory/

echo "remove all subdirectories"
date
echo "-----------Delete files-----------------"
for ((m=0;m<$num_compute_nodes;m++)); do
    $my_del_exp2 $(printf "/N/dc2/scratch/fuyuan/LBMconcurrentstore/$directory/cid%03g" $m)
done
echo "-----------End Delete files-------------"
# $my_del_exp2  /N/dc2/scratch/fuyuan/LBMconcurrentstore/$directory/
date
