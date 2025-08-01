**Remember to mention all environment information by installing on a clean ubuntu!**

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
taskset -c 0-16 ./build/ae/phoenix/phoenix_vanilla # Time taken like 28918 ms
./build/ae/phoenix/phoenix_loader -i dataset/news.shuffled.deduped
# echo "random 20" > sampling.config
taskset -c 0-33 ./build/ae/phoenix/phoenix_orthrus # Performance oscillating between 28.7s to 30s
./build/ae/phoenix/phoenix_loader -i dataset/news.shuffled.deduped 
```

## Memory Test

```bash
export MIMALLOC_VERBOSE=1 MIMALLOC_SHOW_ERRORS=1 MIMALLOC_PURGE_DELAY=0
echo "random 0" > sampling.config 
    # if not 0 sampling rate, the memory overhead will be incorrect
    # a solution to this is to use only one core for validation, and only let that core acquire the memory
```

## Profiling

Similarly, but for having a correct validation overhead, need to

