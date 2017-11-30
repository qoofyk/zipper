#! /bin/bash

# first job - no dependencies
jid1=$(sbatch scripts/bridges/512v256/native_dspaces_nokeep.job|awk '{print $NF}')

jid2=$(sbatch --dependency=afterany:$jid1 scripts/bridges/512v256/native_dimes_nokeep.job|awk '{print $NF}')

jid3=$(sbatch --dependency=afterany:$jid2 scripts/bridges/512v256/adios_dspaces_nokeep.job|awk '{print $NF}')

jid4=$(sbatch --dependency=afterany:$jid3 scripts/bridges/512v256/adios_dimes_nokeep.job|awk '{print $NF}')

jid5=$(sbatch --dependency=afterany:$jid4 scripts/bridges/512v256/mpiio.job|awk '{print $NF}')

jid6=$(sbatch --dependency=afterany:$jid5 scripts/bridges/512v256/adios_flexpath.job|awk '{print $NF}')

jid7=$(sbatch --dependency=afterany:$jid6 scripts/bridges/512v256/sim_only.job|awk '{print $NF}')

# show dependencies in squeue output:
squeue -u $USER -o "%.8A %.4C %.10m %.20E"

