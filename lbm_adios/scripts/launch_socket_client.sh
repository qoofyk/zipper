#!/bin/bash
#SBATCH --job-name="launch_socket_client.sh"
#SBATCH --output="logs/launch_socket_client.sh.out.%j"
#SBATCH --partition=debug
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --export=ALL
#SBATCH --mail-type=ALL
#SBATCH --mail-user=lf921227@gmail.com
#SBATCH -t 00:01:00

## Execution commands are as below:
build/bin/socket_client 149.165.168.149 -n 10000 -s 16384
