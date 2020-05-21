#!/usr/bin/env bash

# First created: 
# Last modified: 2018 Jan 26

# Author: Feng Li
# email: fengggli@yahoo.com

DEPS_DIR=/home/ubuntu/Workspace/spark-standalone/spark-2.4.5-bin-hadoop2.7/dmd-deps
RUNFILES_DIR=build/runfiles

mkdir -pv $DEPS_DIR $RUNFILES_DIR
rm -rf $DEPS_DIR/* $RUNFILES_DIR/*


dep_list="tests/test-redis-spark/lib/spark-redis_2.11-2.4.3-SNAPSHOT-jar-with-dependencies.jar \
 python/requirements.txt "

for filename in $dep_list
do
  echo "copy $filename to $DEPS_DIR"
  cp -r $filename $DEPS_DIR
done

runfile_list="
 tests/test-redis-spark/target/scala-2.11/fluidanalysis_2.11-0.1.0-SNAPSHOT.jar \
 python/run_fluiddmd.py \
 python/wc.py \
 tests/test-redis-spark/conf/log4j.properties"

for filename in $runfile_list
do
  echo "copy $filename to $RUNFILES_DIR"
  cp -r $filename $RUNFILES_DIR
done

# python -m http.server 8080
(
  cd $RUNFILES_DIR
  echo "trying to serving runfiles at $RUNFILES_DIR"
  python -m http.server 8080
)

