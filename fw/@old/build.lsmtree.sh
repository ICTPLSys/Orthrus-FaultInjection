#!/bin/bash

set -x
# killall operation_test

set -e
ROOT=$(dirname $(readlink -f $0))

source $ROOT/env.sh

# cmake --trace -G Ninja ..
# rm -rf CMakeCache.txt CMakeFiles _deps
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
# cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug ..

#ninja clean
# touch $ROOT/../lib/cache.cc
# touch $ROOT/../lib/hashmap.c
# touch $ROOT/../lib/hashmap.cc
find $ROOT/../lib -type f -exec touch {} \;
# find $ROOT/../isal -type f -exec touch {} \;
rm -rf tests
KDEBUG=ON ninja
#KDEBUG=ON ninja
# ninja
