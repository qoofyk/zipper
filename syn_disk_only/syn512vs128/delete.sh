directory="syn080vs080"
num_compute_nodes=80
my_del_exp2='time rsync -a --delete-before  /N/dc2/scratch/fuyuan/empty/ '
# MODIFY {0..1}
echo "remove all subdirectories"
echo "-----------Delete files-----------------"
for ((k=0;k<$num_compute_nodes;k++)); do
    $my_del_exp2 $(printf "/N/dc2/scratch/fuyuan/syn/$directory/exp2/cid%03g" $k)
done
echo "-----------End Delete files-------------"
