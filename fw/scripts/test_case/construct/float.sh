#!/bin/bash

# ../../../scripts/test_case/construct/float.sh

clang++ -S -emit-llvm -mavx -mfma float1.cpp   -o float1.ll && llc -o float1.mir float1.ll