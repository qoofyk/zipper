#! /bin/bash

# first job - no dependencies
CaseName="bridges/128v64"
jid1=$(sbatch scripts/${CaseName}/native_dspaces_nokeep.job|awk '{print $NF}')

jid2=$(sbatch --dependency=afterany:$jid1 scripts/${CaseName}/native_dimes_nokeep.job|awk '{print $NF}')

jid3=$(sbatch --dependency=afterany:$jid2 scripts/${CaseName}/adios_dspaces_nokeep.job|awk '{print $NF}')

jid4=$(sbatch --dependency=afterany:$jid3 scripts/${CaseName}/adios_dimes_nokeep.job|awk '{print $NF}')

jid5=$(sbatch --dependency=afterany:$jid4 scripts/${CaseName}/mpiio.job|awk '{print $NF}')

jid6=$(sbatch --dependency=afterany:$jid5 scripts/${CaseName}/adios_flexpath.job|awk '{print $NF}')

jid7=$(sbatch --dependency=afterany:$jid6 scripts/${CaseName}/sim_only.job|awk '{print $NF}')

# show dependencies in squeue output:
squeue -u $USER -o "%.8A %.4C %.10m %.20E"

