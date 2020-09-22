#!/usr/bin/env bash

# First created: 
# Last modified: 2018 Jan 26

# Author: Feng Li
# email: fengggli@yahoo.com
jobid=$1
nr_node=`ls $jobid |wc -l`
for logfile in $jobid/*
do
  echo "Parsing $logfile"
  start_time=`cat $logfile |grep "Simulation started"|awk '{print $NF}'`
  #echo s$start_time
  end_time=`cat $logfile |grep "Simulation ended"|awk '{print $NF}'`
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
echo "start times: ${start_times[*]}"
echo "end times: ${end_times[*]}"
echo "write times:${write_times[*]}"
echo "bandwidth(MB/s):${band_widths[*]}"

echo ""
echo "Summary:"
sim_start=`echo ${start_times[*]} |tr ' ' '\n' | sort |head -1`
sim_stop=`echo ${end_times[*]} |tr ' ' '\n' | sort |tail -1`
avg_write_time=`echo ${write_times[*]} |tr ' ' '\n' | awk '{i=i+1; sum+=$1} END{print sum/i}' `
agg_bandwidth=`echo ${band_widths[*]} |tr ' ' '\n' | awk '{sum+=$1} END{print sum}'` 

echo jobid nr_proc sim-start sim-end write-time  bandwidth
echo $jobid $((nr_node*16)) $sim_start $sim_stop $avg_write_time $agg_bandwidth
