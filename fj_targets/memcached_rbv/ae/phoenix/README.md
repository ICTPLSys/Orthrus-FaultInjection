**Remember to mention all environment information by installing on a clean ubuntu!**

## Build

```bash
cmake -B build -S . -DCMAKE_C_COMPILER=clang-16 -DCMAKE_CXX_COMPILER=clang++-16 -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Release -G Ninja
172.16.2.58 / 172.16.2.74
```

## Download dataset

```bash
mkdir dataset
cd dataset
wget https://data.statmt.org/news-crawl/en/news.2024.en.shuffled.deduped.gz
gunzip news.2024.en.shuffled.deduped.gz
cd ..
```

## Performance Test

```bash
taskset -c 1-16 ./build/ae/phoenix/phoenix_vanilla # Performance like 28.9 s
./build/ae/phoenix/phoenix_loader -i dataset/news.shuffled.deduped

# echo "random 30" > sampling.config
taskset -c 1-34 ./build/ae/phoenix/phoenix_orthrus # Performance like 29.1 s
./build/ae/phoenix/phoenix_loader -i dataset/news.shuffled.deduped 

taskset -c 1-16 ./build/ae/phoenix/phoenix_rbv_replica # Take the replica time as result
taskset -c 25-40 ./build/ae/phoenix/phoenix_rbv_primary
./build/ae/phoenix/phoenix_loader -i dataset/news.shuffled.deduped
```

## Memory Test

```bash
export MIMALLOC_VERBOSE=1 MIMALLOC_SHOW_ERRORS=1 MIMALLOC_PURGE_DELAY=0
taskset -c 1-16 ./build/ae/phoenix/phoenix_vanilla_mem # Performance like 28.9 s
./build/ae/phoenix/phoenix_loader -i dataset/news.shuffled.deduped

# echo "random 30" > sampling.config
taskset -c 1-34 ./build/ae/phoenix/phoenix_orthrus_mem # Performance like 29.1 s
./build/ae/phoenix/phoenix_loader -i dataset/news.shuffled.deduped 

taskset -c 1-16 ./build/ae/phoenix/phoenix_rbv_replica_mem
taskset -c 25-40 ./build/ae/phoenix/phoenix_rbv_primary_mem # Then, sum up the peak memory
./build/ae/phoenix/phoenix_loader -i dataset/news.shuffled.deduped
```

## Validation Latency Test

```bash
# echo "random 30" > sampling.config
taskset -c 1-34 ./build/ae/phoenix/phoenix_orthrus_profile
./build/ae/phoenix/phoenix_loader -i dataset/news.shuffled.deduped

taskset -c 1-16 ./build/ae/phoenix/phoenix_rbv_replica_profile
taskset -c 25-40 ./build/ae/phoenix/phoenix_rbv_primary
./build/ae/phoenix/phoenix_loader -i dataset/news.shuffled.deduped
```

