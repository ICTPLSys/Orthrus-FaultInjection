#!/usr/bin/env bash

set -xe

mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release

find ../ -type f -exec touch {} \;

cmake --build . --target phoenix_rbv_fj --clean-first -j1
# cmake --build . --clean-first -j -- --output-sync=recurse