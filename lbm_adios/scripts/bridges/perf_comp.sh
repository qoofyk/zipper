#! /bin/bash
#launch_cmd="HAS_TRACE=1 sbatch"
launch_cmd="sbatch"

#USE_RES="--reservation=fli5"
unset USE_RES # do not use reservation
# first job - no dependencies
CaseName="bridges/256v128"
jid1=$(${launch_cmd} ${USE_RES} scripts/${CaseName}/native_dspaces_nokeep.job|awk '{print $NF}')

jid2=$(${launch_cmd} ${USE_RES} --dependency=afterany:$jid1 scripts/${CaseName}/native_dimes_nokeep.job|awk '{print $NF}')

jid3=$(${launch_cmd} ${USE_RES} --dependency=afterany:$jid2 scripts/${CaseName}/adios_dspaces_nokeep.job|awk '{print $NF}')

jid4=$(${launch_cmd} ${USE_RES} --dependency=afterany:$jid3 scripts/${CaseName}/adios_dimes_nokeep.job|awk '{print $NF}')

jid5=$(${launch_cmd} ${USE_RES} --dependency=afterany:$jid4 scripts/${CaseName}/mpiio.job|awk '{print $NF}')

jid6=$(${launch_cmd} ${USE_RES} --dependency=afterany:$jid5 scripts/${CaseName}/adios_flexpath.job|awk '{print $NF}')

jid7=$(${launch_cmd} ${USE_RES} --dependency=afterany:$jid6 scripts/${CaseName}/sim_only.job|awk '{print $NF}')

# show dependencies in squeue output:
squeue -u $USER -o "%.8A %.4C %.10m %.20E"

