#!/usr/bin/env bash
spark-submit --driver-java-options "-Dlog4j.configuration=file://$PWD/conf/log4j.properties" --class click.ClickAnalysis --jars ./lib/spark-redis-2.4.1-SNAPSHOT-jar-with-dependencies.jar --master local[*] ./target/scala-2.12/atomanalysis_2.12-1.0.jar
