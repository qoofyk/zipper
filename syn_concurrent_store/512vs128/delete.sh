num_compute_nodes=512
directory="mbexp512vs128"
my_del_exp2='time rsync -a --delete-before  /N/dc2/scratch/fuyuan/empty/ '

echo "remove all subdirectories"
echo "-----------Delete files-----------------"
for ((m=0;m<$num_compute_nodes;m++)); do
    $my_del_exp2 $(printf "/N/dc2/scratch/fuyuan/store/syn_concurrent_store/$directory/cid%03g" $m)
done
echo "-----------End Delete files-------------"
