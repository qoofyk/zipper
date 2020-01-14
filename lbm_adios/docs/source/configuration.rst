.. _zipper_runconf:

Configurations
****************************

Runtime configurations
==========================

Load the required modules as described in :ref:`zipper_env`


Run jobs
==========================

I. Performance comparison
--------------------------

We run performance comparison scripts  in PSC bridges only, since Stampede2 (KNL + OminiPath) does have good support for some of the transport libraries.

Scripts are in scripts/bridges/

II. Scalability experiments
-----------------------------

We run the  scalability experiments in stampede2.

Scripts are in lbm_adios/zipper/scripts/_stampede2/68v34/68v34.nokeep.pbs

To launch it 

.. code-block::

	tg837458@login2(:):~/Workspaces/zipper-runtime/lbm_adios/zipper$mkdir results
	tg837458@login2(:):~/Workspaces/zipper-runtime/lbm_adios/zipper$sbatch scripts_stampede2/
	1088v544/       16v8/           22v11/          2924v1462/      34v17/          44v22/          68v34/          common.sh       
	136v68/         2176v1088/      272v136/        32v16/          4352v2176/      544v272/        8v4/            .common.sh.swp  
	tg837458@login2(:):~/Workspaces/zipper-runtime/lbm_adios/zipper$sbatch scripts_stampede2/68v34/
	68v34.keep.pbs         .68v34.keep.pbs.swp    68v34.nokeep.itac.pbs  68v34.nokeep.pbs       68v34.nokeep.tau.pbs   
	tg837458@login2(:):~/Workspaces/zipper-runtime/lbm_adios/zipper$sbatch scripts_stampede2/68v34/68v34.nokeep.pbs 

	-----------------------------------------------------------------
						Welcome to the Stampede2 Supercomputer                 
	-----------------------------------------------------------------

	No reservation for this job
	--> Verifying valid submit host (login2)...OK
	--> Verifying valid jobname...OK
	--> Enforcing max jobs per user...OK
	--> Verifying availability of your home dir (/home1/04446/tg837458)...OK
	--> Verifying availability of your work dir (/work/04446/tg837458/stampede2)...OK
	--> Verifying availability of your scratch dir (/scratch/04446/tg837458)...OK
	--> Verifying valid ssh keys...OK
	--> Verifying access to desired queue (normal)...OK
	--> Verifying job request is within current queue limits...OK
	--> Checking available allocation (TG-ASC170037)...OK
	Submitted batch job 5058718
	tg837458@login2(:):~/Workspaces/zipper-runtime/lbm_adios/zipper$less results/LBM.NoKeep.68v34.5058718.out 


