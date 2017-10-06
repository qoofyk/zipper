SLURM_JOBID=111

##################parameter setting#################################################
directory="0008v0004"
nproc_per_mac=14

CASE_NAME=Syn_DataBroker_concurrent_NoKeep

compute_generator_num=1
compute_writer_num=1
analysis_reader_num=1
analysis_writer_num=1

compute_group_size=2
num_comp_proc=8
num_ana_proc=$((${num_comp_proc} / ${compute_group_size}))
total_proc=$((${num_comp_proc} + ${num_ana_proc}))

maxp=1 #control how many times to run each configuration
lp=5 #verification computation repeat times in ana_consumer_thread

#alg: O(n), O(nlgn), O(n^3/2)
#int=4B
#n = 1MB--2^18, 2MB--2^19, 4MB--2^20, 8MB--2^21

#core:                   1,   2,   4,  8, 16,  32,  64
#O(n^3/2) 1MB--  2^9 = 512, 256, 128, 64, 32,  16,   8
#O(nlogn) 1MB- 		    18,   9,   5,  2,  1,

#O(n^3/2) 2MB--2^8.5 = 724, 362, 181, 90, 45,  22,  11  60(12cores)
#O(nlogn) 2MB- 		    19,   9,   4,  2,  1,

#O(n^3/2) 4MB-- 2^10 =1024, 512, 256, 128,64,  32,  16
#O(nlogn) 4MB- 		    20,  10,   5,  2,  1,

#O(n^3/2) 8MB--2^10.5=1448, 724, 362, 181, 90, 45,  22  72(20cores)
#O(nlogn) 8MB- 		    21,  11,   5,  2,  1,

# computation_lp=(1 18 64 1 19 60 1 20 64 1 21 72)
computation_lp=(1 1 1 1)

#one_file
# utime1=(5000 7000 500 1000) #sleep for microseconds
# block_size1=(16 16 32 32 64 64 128 128) #1->64KB, 2->128KB, 4->256KB, 8->512KB, 16->1MB, 32->2MB, 64->4MB, 128->8MB, 256->16MB
# cpt_total_blks1=(4000 4000 2000 2000 1000 1000 500 500)

block_size1=(16 32 64 128) #1->64KB, 2->128KB, 4->256KB, 8->512KB, 16->1MB, 32->2MB, 64->4MB, 128->8MB, 256->16MB
cpt_total_blks1=(4000 2000 1000 500)

#sepfile
# utime2=(5000 4500 14000 12000) #sleep for microseconds

# block_size2=(16 16 16 32 32 32 64 64 64 128 128 128) #1->64KB, 2->128KB, 4->256KB, 8->512KB, 16->1MB, 32->2MB, 64->4MB, 128->8MB, 256->16MB
# cpt_total_blks2=(4000 4000 4000 2000 2000 2000 1000 1000 1000 500 500 500)

# block_size2=(32 32 32 64 64 64 128 128 128) #1->64KB, 2->128KB, 4->256KB, 8->512KB, 16->1MB, 32->2MB, 64->4MB, 128->8MB, 256->16MB
# cpt_total_blks2=(2000 2000 2000 1000 1000 1000 500 500 500)


writer_thousandth=(999)
writer_prb_thousandth=(0)

# writer_thousandth=(400 999)
# writer_prb_thousandth=(1000 1000)

tune_stripe_count=-1
####################################################################################

echo "-----------case=$CASE_NAME---------------"
echo "num_comp_proc=$num_comp_proc num_ana_proc=$num_ana_proc stripe_count=$tune_stripe_count"

PBS_O_HOME=$HOME
PBS_O_WORKDIR=$(pwd)

# comet
# SCRATCH_DIR=/oasis/scratch/comet/qoofyk/temp_project/syn

# bridges
groupname=$(id -Gn)
export SCRATCH_DIR=/pylon5/cc4s86p/qoofyk/syn/${SLURM_JOBID}
EMPTY_DIR=/pylon5/cc4s86p/qoofyk/empty/

# OUTPUT_DIR=${SCRATCH_DIR}/syn/${directory}

BIN1=${PBS_O_HOME}/General_Data_Broker/syn_concurrent_store/build_nokeep/syn_concurrent_nokeep_one_file
BIN2=${PBS_O_HOME}/General_Data_Broker/syn_concurrent_store/build_nokeep/syn_concurrent_nokeep_sep_file

source ${PBS_O_WORKDIR}/scripts_bridges/common.sh
