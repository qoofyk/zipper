#!/usr/bin/env bash

# First created: 
# Last modified: 2018 Jan 26

# Author: Feng Li
# email: fengggli@yahoo.com
JOBID=$1

for logfile in results/$JOBID/log.*; do
  echo `basename $logfile`:
  #cat $logfile| grep sender|tail -1 ;
  cat $logfile| grep requests|tail -1 ;
done



