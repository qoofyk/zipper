#!/usr/bin/env bash
# from https://iujetstream.atlassian.net/wiki/spaces/JWT/pages/35913730/OpenStack+command+line
# excute it only once!
echo initialize using name=$OS_USERNAME
openstack security group create --description "ssh & icmp enabled" ${OS_USERNAME}-global-ssh
openstack security group rule create --protocol tcp --dst-port 22:22 --remote-ip 0.0.0.0/0 ${OS_USERNAME}-global-ssh
openstack security group rule create --protocol icmp ${OS_USERNAME}-global-ssh
openstack keypair create --public-key ~/.ssh/id_rsa.pub ${OS_USERNAME}-api-key
openstack network create ${OS_USERNAME}-api-net
openstack subnet create --network ${OS_USERNAME}-api-net --subnet-range 10.0.0.0/24 ${OS_USERNAME}-api-subnet1
openstack router create ${OS_USERNAME}-api-router
openstack router add subnet ${OS_USERNAME}-api-router ${OS_USERNAME}-api-subnet1
openstack router set --external-gateway public ${OS_USERNAME}-api-router
openstack router show ${OS_USERNAME}-api-router
#openstack router list
#openstack subnet list
#echo $IMAGE-name
#echo ${IMAGE_NAME}
#echo $IMAGE_NAME
#openstack flavor list
#openstack image list --limit 1000  |  grep JS-API-Featured
#openstack server create ${OS_USERNAME}-api-U-1 --flavor m1.tiny --image IMAGE-NAME --key-name ${OS_USERNAME}-api-key --security-group ${OS_USERNAME}-global-ssh \

  

# create a sample instance:
# openstack server create ${OS_USERNAME}-api-U-1 --flavor m1.tiny --image JS-API-Featured-Ubuntu18-Latest --key-name ${OS_USERNAME}-api-key --security-group ${OS_USERNAME}-global-ssh --nic net-id=${OS_USERNAME}-api-net

