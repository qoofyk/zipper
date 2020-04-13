#!/usr/bin/env bash

# First created: 
# Last modified: 2018 Jan 26

# Author: Feng Li
# email: fengggli@yahoo.com

spark-sql --driver-java-options "-Dlog4j.configuration=file://$PWD/conf/log4j.properties" --jars ./lib/spark-redis-2.4.1-SNAPSHOT-jar-with-dependencies.jar 
