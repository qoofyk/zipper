#### magnum
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

