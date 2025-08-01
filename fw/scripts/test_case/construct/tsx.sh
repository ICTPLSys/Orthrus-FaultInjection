#!/bin/bash

# ../../../scripts/test_case/construct/tsx.sh

clang++ -S -emit-llvm -mrtm tsx1.cpp -o tsx1.ll && llc -o tsx1.mir tsx1.ll