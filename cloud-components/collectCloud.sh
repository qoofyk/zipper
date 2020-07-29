#!/usr/bin/env bash

# First created: 
# Last modified: 2018 Jan 26

# Author: Feng Li
# email: fengggli@yahoo.com

NR_NODES=8
for ((i=0;i<$NR_NODES;i++))
do
  outline=`kubectl logs analysis-${i} |grep finish-time |tail -1`
  echo "analysis-$i:  ${outline}"
done
