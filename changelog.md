# Changelog

## [0.2.2] - 2020-04-13
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
