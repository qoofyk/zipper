import sys
import math
def contiguous_mapper(mpi_rank, mpi_size, num_endpoints):
    group_size = math.ceil(mpi_size/num_endpoints) # round up
    local_id = mpi_rank % group_size
    group_id = mpi_rank // group_size
    return (group_id, local_id)

def generate_endpoint_file(endpoint_list, mpi_size, mapper_func):
    for i in range(mpi_size):
        group_id, local_id = contiguous_mapper(i, mpi_size, len(endpoint_list))
        print(endpoint_list[group_id], local_id)


# run like python 3 mapper.py 3 ip1 ip2 ip3 ip4...
if __name__ == "__main__":
    #print("Running: ", sys.argv)
    #print("Number of arguments: ", len(sys.argv))
    mpi_size = int(sys.argv[1])
    endpoint_list = sys.argv[2:]
    #print("The endpoints are: " , endpoint_list)
    generate_endpoint_file(endpoint_list, mpi_size, contiguous_mapper)
