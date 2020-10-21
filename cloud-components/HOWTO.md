## To monitor
```
watch -n 1 "kubectl logs -l spark-role=driver |tail -10"
```
## dev-env
#### prepare
```
sudo apt-get install python3-venv openjdk-8-jdk
python3 -m venv .venv
source .venv/bin/activate
pip install python-openstackclient python-magnumclient
```
#### access k8s client from remote machine(download kubectl binary)
1. I can use openstack api to access remote cluster: https://docs.openstack.org/magnum/latest/user/#native-clients (I tried to configuration authentification but it's to difficult!)
```
source TG*.sh
```
Then you shall be able to list server by using ``openstack server list``

Then
```
(.venv) lifen@in-csci-20wk300(:):~/Workspace/zipper-runtime/openstack$openstack coe cluster config k8s-cluster
export KUBECONFIG=/home/ubuntu/Workspace/zipper-runtime/cloud-components/config
```
I can then: kubectl proxy

Increase num of nodes?
```
openstack coe cluster update mycluster replace node_count=2
```
#### docker: can be good to add user to docker group, as described in (https://docs.docker.com/engine/install/linux-postinstall/)

## Notes
### Jetstream
1. I used the GUI one:
  - [Openstack Commandline](https://iujetstream.atlassian.net/wiki/spaces/JWT/pages/35913730/OpenStack+command+line)
  - [Openstack GUI]:Setup network, subnet, etc: https://iujetstream.atlassian.net/wiki/spaces/JWT/pages/44826638/Using+the+OpenStack+Horizon+GUI+Interface
2. Use ssh to login: ssh ubuntu@149.165.169.185

### K8s cluster
1. Create the jetstream guide. (https://iujetstream.atlassian.net/wiki/spaces/JWT/pages/94175233/Container+Orchestration+on+Jetstream)
2. "The connection to the server localhost:8080 was refused - did you specify the right host or port?"
    - if I reboot the master, I shall do:sudo systemctl restart kube-apiserver
    - See here: https://kubernetes.io/docs/tasks/debug-application-cluster/debug-cluster/
    - I might just shutdown worker node instead
3. To access it from local machine: I shall have local kubectl installation(https://kubernetes.io/docs/tasks/tools/install-kubectl/#kubectl-install-0)
4. Run run a shell in the pod:
```
kubectl exec -ti $POD_NAME bash
```

Kube cheetsheet: https://kubernetes.io/docs/reference/generated/kubectl/kubectl-commands#get

#### Service account:
https://kubernetes.io/docs/reference/access-authn-authz/authentication/

#### using the rbac file for service account
kubectl apply -f /path/to/yaml
then in spark-submit, specify serviceaccount name and namespaces

switch the default namespaces of kubectl:
```
kubectl config set-context --current --namespace=spark-operator
```

### Redis service

Start a redis instance in each worker node 
```
cd cloud-components
kubectl apply -k .
```

- Then you need to change security group settings of minor nodes in jetstream portal to allow 6379
- Also i am using a password


Launch spark instances



### spark
0. deploy-mode: Whether to deploy your driver on the worker nodes (cluster) or locally as an external client (client) (default: client) 
1. Run analysis:
  - label the nodes using label_nodes.sh
  - run analayis with n spark job: ./launch_analysis.sh n; by default , each node will have 16 instances of spark executors, and accepts 64 stream simutenuously.
2. delete pods by label name
```
 kubectl delete pods -l spark-role=driver
```
3. the docker image
copy all needed file(copy deps for docker image and publish runfile in html)
```
./copy_deps.sh
```
in the spark downloaded pre-built dir
export tag=v0.1.5
```
bin/docker-image-tool.sh -t $tag -p ../../zipper-runtime/cloud-components/Dockerfile build
```

run a session first(docker run -it spark-py:$tag bash):

push to docker hub
```
docker tag spark-py:$tag fengggli/spark:$tag
docker push fengggli/spark
```

launch spark gui port
```
kubectl port-forward fluid-analysis-1589987658645-driver 4040:4040
```


### more with redis
just do kubectl apply -f those two yaml file (to delete run kubectl delete -f)
(https://github.com/kubernetes/examples/tree/master/guestbook)
```
kubectl run redis-server --image=redis:5.0
```
Can check logs using kubectl logs redis-server

Expose the service(https://kubernetes.io/docs/tutorials/kubernetes-basics/expose/expose-interactive/):

Get the node:
```
 kubectl get pods -o wide
```

test with in another container, by connecting to it's ip
```
ubuntu@spark-master:~/Workspace/cloud-component$ kubectl run -it redis-client --image=redis:5.0 bash
If you don't see a command prompt, try pressing enter.
root@redis-client:/data# redis-
redis-benchmark  redis-check-aof  redis-check-rdb  redis-cli        redis-sentinel   redis-server
root@redis-client:/data# redis-
redis-benchmark  redis-check-aof  redis-check-rdb  redis-cli        redis-sentinel   redis-server
root@redis-client:/data# redis-cli -h 10.254.247.140
10.254.247.140:6379> ping
PONG

```

Can check current contents or delete fluids if last run failed
```
xrange fluids - +
del fluids
root@redis-client:/data# redis-cli -h 10.100.17.4 keys region\* | xargs redis-cli -h 10.100.17.4 del
```

port:
redis port is using hostNetwork, so, edit the security group in horizon, to allow 6379 for minor nodes

```
ubuntu@spark-master:~/Workspace/cloud-component$ kubectl get services 
NAME                                TYPE        CLUSTER-IP       EXTERNAL-IP   PORT(S)             AGE
redis-master                        NodePort    10.254.247.140   <none>        6379:32385/TCP      2h
spark-pi-1587560414973-driver-svc   ClusterIP   None             <none>        7078/TCP,7079/TCP   9h
spark-pi-1587565931783-driver-svc   ClusterIP   None             <none>        7078/TCP,7079/TCP   7h

```

redis-cluster:
1. https://redis.io/topics/cluster-tutorial
2. https://github.com/spotahome/redis-operator
3. need to create the operator in default context 

#### sparkshell

Run spark shell:
```
kubectl run -it spark-shell --image=fengggli/spark:v1.1 bash
```
Then in the bash
```
root@spark-shell:/opt/spark/work-dir# ../bin/spark-shell
```


```
kubectl run -i --tty busybox --image=busybox --restart=Never -- sh

```

#### Test
In the build dir
```
mpirun -n 4 ./tests/test-redis-spark/c-clients/test-put-mpi-foam -n 4 -i 100 -p 30379 149.165.171.123
```

watch cloud logs: elapsed time
```
watch -n 1 "kubectl logs -l spark-role=driver |tail -9"
```

#### shelve
```
for server in `openstack server list -c Name --name fengggli-k8s-* -f value`; do openstack server shelve $server; done
```

#### Shutdown
```
kubectl delete pods -l spark-role=driver
kubectl delete -k .
openstack coe cluster update elasticbroker-cluster replace node_count=1
shutoff the only minion and master node (or shelve if not using for long period)
```

### restart
```
start instance or unshelve (wait until kubectl get nodes)
openstack coe cluster update elasticbroker-cluster replace node_count=8
./label_nodes.sh
.gen_endpointfile.sh
(change the endpoint file in HPC correspondingly.)
start redis
```
cd yaml
kubectl apply -f ./spark-operator-rbac.yaml 
kubectl config set-context --current --namespace=spark-operator
kubectl apply -k .
```
start spark (run analysis)

```


note:
```
#(or unshelve)
for server in `openstack server list -c Name --name fengggli-k8s-* -f value`; do openstack server unshelve $server; done
# shelve some worknodes
for server in `openstack server list -c Name --name fengggli-k8s-node-[2-8] -f value`; do openstack server shelve $server; done
```
