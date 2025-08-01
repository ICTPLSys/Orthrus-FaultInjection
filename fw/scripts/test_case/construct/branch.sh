#!/bin/bash

# ../../../scripts/test_case/construct/branch.sh

clang++ -S -emit-llvm branch1.cpp -o branch1.ll && llc -o branch1.mir branch1.ll