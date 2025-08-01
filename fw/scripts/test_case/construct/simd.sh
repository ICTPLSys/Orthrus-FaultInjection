#!/bin/bash

# ../../../scripts/test_case/construct/simd.sh

clang++ -S -emit-llvm -O2 -mavx -mfma simd1.cpp  -o simd1.ll && llc -o simd1.mir simd1.ll