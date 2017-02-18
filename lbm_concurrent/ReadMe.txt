LBM-No-Keep
Compute: generator <--Compute_Ringbuffer--> sender<->writer
Analysis: receiver reader <--Analysis_Ringbuffer--> consumer

2016/06/30
Can handle special case: If special case happens, sender will start to rob writer's workload.
