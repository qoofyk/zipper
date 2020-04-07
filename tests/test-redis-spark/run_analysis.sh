#!/usr/bin/env bash

# First created: 
# Last modified: 2018 Jan 26

# Author: Feng Li
# email: fengggli@yahoo.com
#spark-submit --class ClickAnalysis --jars ./lib/spark-redis-2.4.1-SNAPSHOT-jar-with-dependencies.jar --master local[*] ./target/scala-2.12/redisexample_2.12-1.0.jar
SCALA_VERSION=2.11
spark-submit  \
    --driver-java-options "-Dlog4j.configuration=file://$PWD/conf/log4j.properties" \
    --class FluidAnalysis --jars ./lib/spark-redis_${SCALA_VERSION}-2.4.3-SNAPSHOT-jar-with-dependencies.jar \
    --master local[*] \
    ./target/scala-${SCALA_VERSION}/fluidanalysis_$SCALA_VERSION-0.1.0-SNAPSHOT.jar \
    --files compute_dmd.py \
    --py-files /home/lifen/Workspace/zipper-runtime/pydmd_env.zip

# fli5@149.165.156.187
#--class FluidAnalysis --jars ./lib/spark-redis-2.4.1-SNAPSHOT-jar-with-dependencies.jar
