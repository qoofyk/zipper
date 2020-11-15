### using kubesspray
I tried this when the magnum went down in Jetstream, but later it came back...
Zonca is really helpful in providing suggestions and helping trouble shooting.
1. Terraform
```
wget https://releases.hashicorp.com/terraform/0.11.13/terraform_0.11.13_linux_amd64.zip
```

2. follow instructions from zonca (use the new 2.13)
https://zonca.dev/2019/02/kubernetes-jupyterhub-jetstream-kubespray.html
use ssh -L so that the gateway node has access to the kubectl

Steps:
```
pip install -r requirement.sh # (or source corresponding venv)
export CLUSTER=fengggli # this env_var is important@!
cd  cluster/$CLUSTER
bash ./terraform_init.sh
bash ./terraform_apply.sh
cd ../../
ansible -i inventory/$CLUSTER/hosts -m ping all
ansible-playbook --become -i inventory/$CLUSTER/hosts cluster.yml #(might need multiple times in case of apt-get lock)
```

to connect: make soft links of the admin.conf, change ip from 10.0.0.x to 127.0.0.1

Note different version of jetstream kubespray uses different terraform and ansible.
1. Use too new ansible(>2.8) for kubespray 2.8 causes : https://github.com/kubernetes-sigs/image-builder/issues/92
2. Use too new terraform(>1.11)

The newer version (not suggested):
- https://zonca.dev/2020/06/kubernetes-jetstream-kubespray.html
v2.13.1 only supports k8s higher than 1.16, which has problem with 2.4.5 Spark 
- I forked his v2.13.1, only changed the cluster.tfvars (in this folder)
- if a ansible -m ping failed, i press control-c, the terminal returns control. However the ansible is not exiting, which can interfere with later execution. (https://docs.ansible.com/ansible/latest/collections/ansible/builtin/pause_module.html)
- different kubespray uses different terraform

Kill manually?
	``kill $(ps aux|grep ansible |awk '{print $2}') ``

3. the cluster will be up, but will complains about 8080: use root then kubectl get pods

4. To access the cluster, from another node
```
export KUBECONFIG=/home/ubuntu/Workspace/zipper-runtime/extern/kubespray/inventory/fengggli/artifacts/admin.conf
 kubectl --insecure-skip-tls-verify get pods
```

#### using magnum (deprecated)
1. create cluster template
```
openstack coe cluster template create --coe kubernetes --image Fedora-Atomic-29 --keypair mypi --external-network public --fixed-network fengggli-net --fixed-subnet fengggli-subnet --network-driver flannel --flavor m1.large --master-flavor m1.large  --docker-volume-size 10 --docker-storage-driver devicemapper --floating-ip-enabled template-k8s-fedora29-volume
```
I tried to avoid volume
```
openstack coe cluster template create --coe kubernetes --image Fedora-Atomic-29 --keypair mypi --external-network public --fixed-network fengggli-net --fixed-subnet fengggli-subnet --network-driver flannel --flavor m1.large --master-flavor m1.large  --docker-storage-driver overlay2 --floating-ip-enabled template-k8s-fedora29
```

2. create cluster
```
openstack coe cluster create --cluster-template template-k8s-20200526 --master-count 1 --node-count 8 k8s-cluster-8
```

3. To trouble shooting:
```
openstack stack list
openstack stack show xx
```

Using kubespray: see cluster-inventory

