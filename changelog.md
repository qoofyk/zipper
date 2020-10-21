# Changelog
## new
[cloud]:
1. add support for dryrun for scala code, which only collects count for items received from each localid
2. change the synthetic code correspondingly.

## [0.2.8] - 2020-10-19
[orchestration]:
- switch back to kubespray (2.8)
- changed port to 30379
- cloud instances have 16 regions per VM, not 64

[hpc]:
- synthetic nr_fluid changed to 5000/2000 for 512 proc run.
- nr_regios varies form 64 to 16.
- tic toc bandwidth test:
  - code:  tests/test-iperf3
  - results: https://github.com/fengggli/zipper-runtime/issues/22
- collect stats from all proces and show std and avg.
- now uses communicator instead of rank/size in elasticbroker_init

[todo]:
- for synthetic calculation, can just choose some other methods than DMD
- add pipeline support for elasticbroker put.
- simpler wc spark.

## [0.2.7] - 2020-09-21
[added-cloud]:
- add jestream-tacc source file, but container orchestration is not there yet.
- connection between iu-jetstream and stampede2 is slow.
- switched back to magnum, need to allow secgroup to bypass 6379.
- prepared scripts to run in xsede, but very slow...

[added-hpc]:
- copy case to scratch and fetch result log back
- case M changed to 1000 iterations
- add windaroundbuildings M, L, see https://github.com/fengggli/zipper-runtime/issues/19#issuecomment-692958304
- python/mapper.py to generate endpoint mapping with mapper, works like the nodefile in mpirun

[todo]:
- why streams are only sent to one spark job?

## [0.2.6] - 2020-07-29
[Added]:
1. launch mpi to different nodes with correct redis bindings
- use hostNetwork to bind redis to each k8s node.
- Let spark-submit launch multiple clusters using different redis
- Can use opentack cmd to increase k8s nodes
- specify node selector for spark-submit
	for each: hostidx:
		redis_ip
		label-idx
- post-processing results to get min-start-time and max-end-time  in hpc
2. hpc 
- barrier before timing
- experiments with collated and uncollated write
- Now all methods export collated so that there are not many folder created
- script to run three methods all together.

[todo]:
1. shutdown cluster when
2. kubespray
- use zonca's kubespray instructions 
3. Post processing
- post-processing results in cloud 

## [0.2.5] - 2020-05-21
[added]:
1. build 1906 in karst. and adjust build so that can use mySimpleFoamCloud
[changed]:
1. remove argopt,becase it's interfere with openfoam -parallel.
2. move working directory to scratch at SLATE
3. changed to z axis cutting.
4. use region id as stream name
5. use redis.batch size to limit each microbatch = 50\*numregions
[todo]:
``processor0/100.4/uniform/functionObjects/functionObjectProperties creates so many folders!``
1. scalaing experiments
## [0.2.4] - 2020-04-29
[added]:
1. Add support for spark cluster in kubenete cluster deployed in Jetstream, it works like this:
  - setup k8s environment in jetstream, using openstack Magnum.
  - some yaml file to setup service account, and deploy and expose redis service.
  - a docker image is build, using the Dockerfile in cloud-components(shipped with python/jar dependencies)
2. Dependencies
  - jar dependencies(spark-redis-.jar) and python dependencies(requirement.txt) are copied to the docker image
  - During runtime, user will need to provide url to some runfiles, which can be published using the copy_deps.sh file.

2. HPC side: mysimpleFoam now can run with ./mysimpleFoam -p redis port redishostaddress.

## [0.2.3] - 2020-04-13
[added]:
1. Openfoam-redis-spark-pydmd pipeline:
  - Openfoam windAroundBuilding with simple Foam
  - spark pipe, which will direct output from spark stream to pydmd analysis.
  - pydmd analysis(in python dir), it will be packaged to python egg, to be used by spark pipe

[todo]:
1. Experiments with advanced cluster/mpi settings.
2. Explain more on the impact of doing dmd analysis on cloud.

## [0.2.2] - 2020-02-12
[added]:
1. add streaming processing example using spark-redis(https://www.infoq.com/articles/data-processing-redis-spark-streaming/)
  - A C client(putter) using hiredis, plus a mpi version, those will insert simulation(atom) data, contains (step, atomid, x, y, z)
  - customized streaming engine (getter), so it now get statistics info from redis input. (step, avg(x), avg(y), avg(z), count(atoms))
  - Verified correctness by viewing the output of streaming engine
  - learned how to use sbt build system(can create docker image directly!)

[todo]:
1. currently data is transmitted by text. (binary-save string needs some extra work)
2. can implement MSD analysis in spark.

[optional]:
2. containerize for more convenient deployment.

## [0.2.1] - 2020-01

[added]:
1. tcp socket support, so that hpc can send data to cloud.
2. bandwith remains to be the issue, but can reduce data sending frequency.

## [0.1] - 2019-12
[added]:
1. merge in previous code of zipper
