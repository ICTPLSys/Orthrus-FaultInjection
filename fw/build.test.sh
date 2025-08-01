#!/usr/bin/env bash

set -xe

mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug

find ../ -type f -exec touch {} \;

cmake --build . --clean-first -j
# cmake --build . --clean-first -j -- --output-sync=recurse