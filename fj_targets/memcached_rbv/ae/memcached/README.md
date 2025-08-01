TODO: RBV

## Performance

```bash
# For the performance, average the PUT for UPDATE and GET, stored in client.log
# Currently, writing to client.log is w+: meaning append
taskset -c 1-4 ./build/ae/memcached/memcached_vanilla 23456
taskset -c 24-47 ./build/ae/memcached/memcached_client localhost 23456

taskset -c 1-8 ./build/ae/memcached/memcached_orthrus 23456
taskset -c 24-47 ./build/ae/memcached/memcached_client localhost 23456

taskset -c 24-27 ./build/ae/memcached/memcached_rbv_replica
taskset -c 1-4 ./build/ae/memcached/memcached_rbv_primary 23456
taskset -c 28-47,88-95 ./build/ae/memcached/memcached_client localhost 23456
```

## Throughput vs Latency

Run the same server, but replace the client to:
```bash
taskset -c 24-47 ./build/ae/memcached/memcached_client localhost 23456 client.log 3 32 24 19 rps
```
where **3 * rps** is the target throughput. Then, collect data from `client.log` with the actual throughput and corresponding percentage latency.

## Validation Latency Test

```bash
taskset -c 1-8 ./build/ae/memcached/memcached_orthrus_profile
taskset -c 24-47 ./build/ae/memcached/memcached_client localhost 23456
```