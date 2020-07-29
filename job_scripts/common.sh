#!/usr/bin/env bash
SIM_PROCS=16 # square of proc_side

PBS_RESULTDIR=${PBS_O_WORKDIR}/results/${PBS_JOBID}

mkdir -pv ${PBS_RESULTDIR}
cd $PBS_O_WORKDIR

echo "hostfile in $PBS_NODEFILE"
allnodes=`cat ${PBS_NODEFILE} |uniq | awk 'BEGIN { FS = "." }{print $1}'`

NUM_NODES=`echo $allnodes|wc -w`
echo "Using all nodes at $allnodes, ($NUM_NODES intotal)"
# num of process in each side

# kubectl get nodes --selector=beta.kubernetes.io/instance-type=3 -o jsonpath={.items[*].status.addresses[?\(@.type==\"ExternalIP\"\)].address}
REDIS_LIST=(sievert.cs.iupui.edu)
#REDIS_LIST=(149.165.168.179 149.165.168.151 149.165.168.36 149.165.156.99 149.165.170.173 149.165.169.96 149.165.168.247 149.165.157.130)
