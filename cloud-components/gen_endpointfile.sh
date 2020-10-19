#!/usr/bin/env bash

# First created: 
# Last modified: 2018 Jan 26

# Author: Feng Li
# email: fengggli@yahoo.com

# run this after nodes are labeled: sh gen_endpointfile.sh nr_mpi_procs
EXTERNAL_IPS=$(kubectl get nodes --selector=minion-idx -o jsonpath={.items[*].status.addresses[?\(@.type==\"ExternalIP\"\)].address})
echo "${EXTERNAL_IPS[*]}"

outfile=endpoints.ini
python3 ../python/mapper.py ${1} ${EXTERNAL_IPS[*]} > $outfile
echo "$1 entries haven been written in endpoint file at $PWD/$outfile"
