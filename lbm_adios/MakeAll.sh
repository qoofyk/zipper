#!/bin/bash

PBS_O_HOME=$HOME
PBS_O_WORKDIR=$(pwd)
rm -rf make.log
for CASE_NAME in dataspaces_nokeep dataspaces_keep dimes_nokeep dimes_keep mpiio sim_only
do
    BUILD_DIR=${PBS_O_WORKDIR}/build_${CASE_NAME}
    cd ${BUILD_DIR}
    echo "case ${CASE_NAME} is built"
    make >> make.log
done
