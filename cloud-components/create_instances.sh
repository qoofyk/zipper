#!/usr/bin/env bash

# First created: 
# Last modified: 2018 Jan 26

# Author: Feng Li
# email: fengggli@yahoo.com
# .sh c 0 4 (create 0,1,2,3,4,5,)
# .sh c 5 8 (create 5~8)
# .sh d 5 8 (delete 5-8)  
nr_master_vm=1
tmpfile=/tmp/fengggli.tmp

ips=($(openstack floating ip list |awk '{print $4" "$6}'|grep None | awk '{print $1}'))
echo available floating ips:${ips[*]}

if [ "$1" == "l" ]; then
  openstack server list |grep k8s-vm | sed -e 's/=/ /g' > $tmpfile 
  echo "IPS: (run with CONFIG_FILE=inventory/mycluster/hosts.yaml python3 contrib/inventory_builder/inventory.py) "
  cat $tmpfile  |sed -e 's/,/ /g' |awk '{print $4" ansible_host="$9}' |sort -V
  echo "Internal IP:"
  cat $tmpfile  | sed -e 's/,/ /g' |awk '{print $4" "$9" "$10}' |sort | awk 'BEGIN{ORS=" "}{print $2}END{print "\n"}'

  echo "External IP:"
  cat $tmpfile | sed -e 's/,/ /g' |awk '{print $4" "$9" "$10}' |sort | awk 'BEGIN{ORS=" "}{print $3}END{print "\n"}'
  exit
fi

idx=0
for i in $(seq $2 $3 $END)
do
  vm_name=k8s-vm-$i
  if [ "$1" == "d" ]; then
    echo "delete vm $vm_name"
    openstack server delete $vm_name

  elif [ "$1" == "i" ]; then
    echo "associating ip${ips[idx]} to  $vm_name"
    openstack server add floating ip $vm_name ${ips[idx]}

  else
    echo "create vm $vm_name"
    openstack server create $vm_name \
      --flavor m1.large \
      --image JS-API-Featured-Ubuntu18-Latest \
      --key-name jetstream-dev-key \
      --security-group fengggli-secGroup \
      --nic net-id=fengggli-net
  fi
  idx=$((idx+1))
done



