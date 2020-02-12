#!/usr/bin/env bash

# First created: 
# Last modified: 2018 Jan 26

# Author: Feng Li
# email: fengggli@yahoo.com
#spark-submit --class ClickAnalysis --jars ./lib/spark-redis-2.4.1-SNAPSHOT-jar-with-dependencies.jar --master local[*] ./target/scala-2.12/redisexample_2.12-1.0.jar
spark-submit --driver-java-options "-Dlog4j.configuration=file://$PWD/conf/log4j.properties" --class AtomAnalysis --jars ./lib/spark-redis-2.4.1-SNAPSHOT-jar-with-dependencies.jar --master local[*] ./target/scala-2.12/atomanalysis_2.12-1.0.jar
