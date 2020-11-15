#!/usr/bin/env bash

SPARK_ROOT=${PWD}/../extern/spark-3.0.1-bin-hadoop2.7
K8SMASTER_IP=localhost
REMOTE_SPARK_HOME=/opt/spark/

echo $SPARK_BIN
${SPARK_ROOT}/bin/spark-submit \
		--deploy-mode cluster \
    --master k8s://https://${K8SMASTER_IP}:6443 \
    --name spark-pi \
    --class org.apache.spark.examples.SparkPi \
    --conf spark.executor.instances=2 \
    --conf spark.kubernetes.container.image=fengggli/spark:v0.1.6 \
		--conf spark.kubernetes.namespace=spark-operator \
		--conf spark.kubernetes.authenticate.driver.serviceAccountName=sparkoperator \
		local://${SPARK_ROOT}/examples/jars/spark-examples_2.11-2.4.5.jar
