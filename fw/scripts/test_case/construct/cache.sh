#!/bin/bash

# ../../../scripts/test_case/construct/cache.sh

clang++ -S -emit-llvm cache_coherence1.cpp -o cache_coherence1.ll && llc -o cache_coherence1.mir cache_coherence1.ll