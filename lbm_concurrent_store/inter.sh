SLURM_JOBID=111
##################parameter setting#################################################
directory="0008v0004"
nproc_per_mac=14

CASE_NAME=LBM_DataBroker_concurrent_Keep

FILESIZE2PRODUCE=256 # 64*64*256*2*8 = 16MB per proc
compute_generator_num=1
compute_writer_num=1
analysis_reader_num=1
analysis_writer_num=1
# writer_thousandth=300
compute_group_size=2
num_comp_proc=8
num_ana_proc=$((${num_comp_proc} / ${compute_group_size}))
total_proc=$((${num_comp_proc} + ${num_ana_proc}))

maxp=1 #control how many times to run each configuration
n_moments=4   #n_moment
NSTOP=100 # how many steps

cubex=(32 32 64 64)
cubez=(64 128 64 128)
writer_thousandth=(0 999) #upper_limit_hint
writer_prb_thousandth=(1000 1000)

tune_stripe_count=-1
####################################################################################

echo "-----------case=$CASE_NAME---------------"
echo "datasize=$FILESIZE2PRODUCE nstops=$NSTOP num_comp_proc=$num_comp_proc num_ana_proc=$num_ana_proc n_moments=$n_moments NSTOP=$NSTOP stripe_count=$tune_stripe_count"

PBS_O_HOME=$HOME
PBS_O_WORKDIR=$(pwd)

# comet
# SCRATCH_DIR=/oasis/scratch/comet/qoofyk/temp_project/lbm

# bridges
groupname=$(id -Gn)
export SCRATCH_DIR=/pylon5/cc4s86p/qoofyk/lbm/${SLURM_JOBID}
EMPTY_DIR=/pylon5/cc4s86p/qoofyk/empty/

BIN1=${PBS_O_HOME}/General_Data_Broker/lbm_concurrent_store/build_keep/lbm_concurrent_keep_onefile
BIN2=${PBS_O_HOME}/General_Data_Broker/lbm_concurrent_store/build_keep/lbm_concurrent_keep_sep_file

source ${PBS_O_WORKDIR}/scripts_bridges/common.sh

