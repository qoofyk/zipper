directory="LBM200vs100"
total_nodes=300
my_del_exp2='time rsync -a --delete-before  /N/dc2/scratch/fuyuan/empty/ '
$my_del_exp2 /N/dc2/scratch/fuyuan/LBM/$directory/
# echo "-----------Delete files-----------------"
# for ((k=0;k<$total_nodes;k++)); do
#     $my_del_exp2 $(printf "/N/dc2/scratch/fuyuan/LBM/$directory/cid%03g" $k)
# done
# echo "-----------End Delete files-------------"
