#!/usr/bin/env bash

# First created: 
# Last modified: 2018 Jan 26

# Author: Feng Li
# email: fengggli@yahoo.com


NR_NODES=${1:=8}

worker_nodes=($(kubectl get nodes -l  magnum.openstack.org/role=worker -o jsonpath={.items[*].metadata.name}))

for ((i=0;i<$NR_NODES;i++))
do
echo setting for node ${i}}
#kubectl label nodes ${worker_nodes[i]}  minion-idx=${i}
kubectl label --overwrite nodes k8s-vm-$((i+1))  minion-idx=${i}
done
