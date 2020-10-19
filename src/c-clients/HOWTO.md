Using elasticBroker with C/CPP applications
## prepare c/cpp applications
1. use elastic_broker functions defined in src/c-clients/elastic_broker.h
2. link with *hiredis elastic_broker*.
3. generate a "endpoint file" and place it running directory.
  - can run this command in the server which has k8s connection: sh cloud-components/gen_endpointfile.sh 512, where 512 is the number of mpi procs.

## Run with
```
mpirun -n 4 ./bin/test-put-mpi-foam -n 4 -i 100
```
where: 
  -n is the number of fluid in each process
  -i is the iteration.
