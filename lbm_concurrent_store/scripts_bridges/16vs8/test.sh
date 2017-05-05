
directory="LBMcon024vs012"
groupname=$(id -Gn)
SCRATCH_DIR=/pylon5/${groupname}/qoofyk/LBMconcurrentstore/

OUTPUT_DIR=${SCRATCH_DIR}/LBMconcurrentstore/${directory}

tune_stripe_count=-1

mkdir -pv ${OUTPUT_DIR}
lfs setstripe --stripe-size 1m --stripe-count ${tune_stripe_count} ${OUTPUT_DIR}
