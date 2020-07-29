#### magnum (deprecated)
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

### using kubesspray
1. Terraform
```
wget https://releases.hashicorp.com/terraform/0.11.13/terraform_0.11.13_linux_amd64.zip
```

2. follow instructions from zonca
- https://zonca.dev/2018/09/kubernetes-jetstream-kubespray.html
- https://zonca.dev/2018/09/kubernetes-jetstream-kubespray.html

3. the cluster will be up, but will complains about 8080: use root then kubectl get pods

4. To access the cluster, from another node
```
export KUBECONFIG=/home/ubuntu/Workspace/zipper-runtime/extern/kubespray/inventory/fengggli/artifacts/admin.conf
 kubectl --insecure-skip-tls-verify get pods

```
