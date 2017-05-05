groupname=$(id -Gn)
SCRATCH_DIR=/pylon5/${groupname}/qoofyk/

OUTPUT_DIR=${SCRATCH_DIR}/LBMconcurrentstore/
EMPTY_DIR=${SCRATCH_DIR}/empty/

rm -rf ${EMPTY_DIR}
mkdir ${EMPTY_DIR}

time rsync -a --delete-before ${EMPTY_DIR} ${OUTPUT_DIR}
# time rsync -a --delete-before /oasis/scratch/comet/qoofyk/temp_project/empty /oasis/scratch/comet/qoofyk/temp_project/LBMconcurrentstore

# rm -rf ${OUTPUT_DIR}

# ls -lh ${OUTPUT_DIR}
