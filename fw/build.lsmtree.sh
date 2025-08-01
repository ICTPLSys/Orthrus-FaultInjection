#!/usr/bin/env bash

set -xe

mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_LSMTREE=ON

find ../ -type f -exec touch {} \;

# cmake  --build . --target lsmtree_fj --clean-first -j1
cmake --build . --target lsmtree_fj_scee --clean-first -j1

# cmake --build . --clean-first -j -- --output-sync=recurse