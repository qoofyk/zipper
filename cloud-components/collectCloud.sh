#!/usr/bin/env bash

# First created: 
# Last modified: 2018 Jan 26

# Author: Feng Li
# email: fengggli@yahoo.com

NR_NODES=8
time_str=$(date +"%Y%m%d-%H%M")
out_folder=logs/${time_str}.d
mkdir -pv $out_folder
for ((i=0;i<$NR_NODES;i++))
do
  out_file=$out_folder/analysis-${i}.log
  kubectl logs analysis-${i} > $out_file

  outline=`cat $out_file |grep finish-time |tail -1`
  echo "analysis-$i:  ${outline}"
done
