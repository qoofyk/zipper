#!/usr/bin/env bash

# First created: 
# Last modified: 2018 Jan 26

# Author: Feng Li
# email: fengggli@yahoo.com
SPARK_ROOT=/home/ubuntu/Workspace/spark-standalone/spark-2.4.5-bin-hadoop2.7
K8SMASTER_IP=149.165.169.132
REMOTE_SPARK_HOME=/opt/spark/

echo $SPARK_BIN
${SPARK_ROOT}/bin/spark-submit \
		--deploy-mode cluster \
    --master k8s://https://${K8SMASTER_IP}:6443 \
    --name spark-pi \
    --class org.apache.spark.examples.SparkPi \
    --conf spark.executor.instances=2 \
    --conf spark.kubernetes.container.image=fengggli/spark:testing \
		--conf spark.kubernetes.namespace=spark-operator \
		--conf spark.kubernetes.authenticate.driver.serviceAccountName=sparkoperator \
		local://${REMOTE_SPARK_HOME}/examples/jars/spark-examples_2.11-2.4.5.jar
