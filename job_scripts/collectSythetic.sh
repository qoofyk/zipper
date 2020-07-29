#!/usr/bin/env bash

# First created: 
# Last modified: 2018 Jan 26

# Author: Feng Li
# email: fengggli@yahoo.com
for logfile in $1/*
do
  echo "Parsing $logfile"
  start_time=`cat $logfile |grep "Simulation started"|awk '{print $7}'`
  #echo s$start_time
  end_time=`cat $logfile |grep "Simulation ended"|awk '{print $7}'`
  #echo e$end_time
  write_time=`cat $logfile | grep "Average time" |awk '{print $3}'`
  #echo w$write_time
  band_width=`cat $logfile |grep Throughput |awk '{print $4}'`

  start_times+=($start_time)
  end_times+=($end_time)
  write_times+=($write_time)
  band_widths+=($band_width)
done

IFS=","
echo "start times:${start_times[*]}"
echo "end times:${end_times[*]}"
echo "write times:${write_times[*]}"
echo "bandwidth(MB/s):${band_widths[*]}"
