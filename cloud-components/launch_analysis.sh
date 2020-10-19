#!/usr/bin/env bash

# run with ./launchAnalysis.sh
NR_NODES=8
NR_REGIONS=16  # this number of region, each will initiate a stream and process as different partitions in spark
#let NR_SPARK_INSTANCES="($NR_REGIONS + 4 -1)/4" # run with launch_analysis.sh nr_instances
let NR_SPARK_INSTANCES="16" # run with launch_analysis.sh nr_instances
IMAGE_VERSION=v0.1.5 # use hostpath, and use py image
RUNFILES_DIR="http://149.165.169.185:8080/"
SPARK_ROOT=/home/ubuntu/Workspace/spark-standalone/spark-2.4.5-bin-hadoop2.7
REMOTE_SPARK_HOME=/opt/spark/
SCALA_VERSION=2.11

# they will be labeled at minion-idx=0,1,2

#kubectl get nodes -l  magnum.openstack.org/role=worker -o jsonpath={.items[*].status.addresses[?\(@.type==\"InternalIP\"\)].address}
REDIS_IPS_INTERNAL=($(kubectl get pods --selector=app=redis,role=master -o jsonpath={.items[*].status.podIP}))
#K8SMASTER_IP=$(kubectl get nodes --selector=node-role.kubernetes.io/master -o jsonpath={.items[*].status.addresses[?\(@.type==\"ExternalIP\"\)].address})
K8SMASTER_IP=localhost
REDIS_PASS=`cat redis.pass`

for ((i=0;i<$NR_NODES;i++))
do
	#REDIS_IP=${REDIS_IPS_INTERNAL[i]}
	# redis using this cluster's IP
	REDIS_IP=$(kubectl get nodes -l minion-idx=${i} -o jsonpath={.items[*].status.addresses[?\(@.type==\"InternalIP\"\)].address})
	echo "Use k8s cluster at ${K8SMASTER_IP}, redis server at $REDIS_IP, run spark-submit with $NR_SPARK_INSTANCES instances, with $NR_REGIONS regions"

	${SPARK_ROOT}/bin/spark-submit \
			--deploy-mode cluster \
			--master k8s://https://${K8SMASTER_IP}:6443 \
			--name fluid-analysis-copy-${i} \
			--class FluidAnalysis \
			--driver-java-options "-Dlog4j.configuration=${RUNFILES_DIR}/log4j.properties" \
			--conf spark.kubernetes.driver.pod.name=analysis-${i} \
			--conf spark.executor.instances=$NR_SPARK_INSTANCES \
			--conf spark.kubernetes.container.image=fengggli/spark:${IMAGE_VERSION} \
			--conf "spark.redis.host=${REDIS_IP}" \
			--conf "spark.redis.port=30379" \
			--conf spark.kubernetes.namespace=spark-operator \
			--conf spark.kubernetes.authenticate.driver.serviceAccountName=sparkoperator \
			--conf "spark.redis.auth=${REDIS_PASS}" \
			--conf "spark.kubernetes.node.selector.minion-idx=${i}" \
			--conf "spark.kubernetes.executor.request.cores=0.5" \
			--conf "stream.read.batch.size=$((200))" \
			--conf "stream.read.block=250" \
			--jars ${REMOTE_SPARK_HOME}/work-dir/deps/spark-redis_${SCALA_VERSION}-2.4.3-SNAPSHOT-jar-with-dependencies.jar \
			--files  ${RUNFILES_DIR}/run_fluiddmd.py,${RUNFILES_DIR}/wc.py \
			${RUNFILES_DIR}/fluidanalysis_$SCALA_VERSION-0.1.0-SNAPSHOT.jar  \
			$NR_REGIONS &>tmp/log.node${i} &
done



echo "all spark tasks launched"
wait

# if deployed in docker: 
#    --jars ${REMOTE_SPARK_HOME}/work-dir/deps/spark-redis_${SCALA_VERSION}-2.4.3-SNAPSHOT-jar-with-dependencies.jar \
#    --jars ${RUNFILES_DIR}/spark-redis_${SCALA_VERSION}-2.4.3-SNAPSHOT-jar-with-dependencies.jar \
#    --py-files ${RUNFILES_DIR}/fluiddmd-0.1-py3.6.egg \

