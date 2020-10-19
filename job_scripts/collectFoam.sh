#!/usr/bin/env bash

# First created: 
# Last modified: 2018 Jan 26

# Author: Feng Li
# email: fengggli@yahoo.com
logfolder=$1
for logfile in $logfolder/*
do
  echo "Parsing $logfile"
  start_time=`cat $logfile |grep "Simulation started"|awk '{print $NF}'`
  #echo s$start_time
  end_time=`cat $logfile |grep "Simulation Ended"|awk '{print $NF}'`
  #echo e$end_time
  write_time=`cat $logfile | grep "total send time" |awk '{print $(NF-1)}'`

  echo "Simu end2end time ($start_time, $end_time)"
  echo "Simu write time $write_time"

  start_times+=($start_time)
  end_times+=($end_time)
  write_times+=($write_time)
done


#IFS=","
#echo "start times: ${start_times[*]}"
#echo "end times: ${end_times[*]}"
#echo "write times:${write_times[*]}"
