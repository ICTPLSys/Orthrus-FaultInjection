#!/bin/bash

LLVM_DIR=$HOME/.local/opt/llvm16

export CC=$LLVM_DIR/bin/clang
export CXX=$LLVM_DIR/bin/clang++


# export LD_LIBRARY_PATH="$LLVM_DIR/lib:$LD_LIBRARY_PATH"
# export LD_LIBRARY_PATH="$LLVM_DIR/lib/x86_64-unknown-linux-gnu:$LD_LIBRARY_PATH"
# 
# # export CFLAGS=$CFLAGS
# 
# export CXXFLAGS="-L$LLVM_DIR/lib $CXXFLAGS"
# export CXXFLAGS="-L$LLVM_DIR/x86_64-unknown-linux-gnu $CXXFLAGS"
# 
# export CXXFLAGS="-I$LLVM_DIR/include $CXXFLAGS"
# export CXXFLAGS="-I$LLVM_DIR/include/c++/v1 $CXXFLAGS"
# export CXXFLAGS="-I$LLVM_DIR/include/x86_64-unknown-linux-gnu/c++/v1 $CXXFLAGS"
# export CXXFLAGS="-stdlib=libstdc++ $CXXFLAGS"
# export CXXFLAGS="-Wno-unused-command-line-argument $CXXFLAGS"
# 
# export CPPFLAGS=$CXXFLAGS
# 
# # export LDFLAGS="-L$HOME/.local/opt/llvm16/lib $LDFLAGS"
# # export LDFLAGS="-L$HOME/.local/opt/llvm16/lib/x86_64-unknown-linux-gnu $LDFLAGS"
# unset LDFLAGS
# export LDFLAGS="-stdlib=libstdc++ $LDFLAGS"


# export CC=$HOME/.local/opt/llvm16/bin/clang
# export CXX=$HOME/.local/opt/llvm16/bin/clang++
#
# LLVM_DIR=/lustre/S/liuchenxiao/Projects/llvm/llvm-project/build.release
#
# export LD_LIBRARY_PATH="$HOME/.local/opt/llvm16/lib:$LD_LIBRARY_PATH"
# export LD_LIBRARY_PATH="$HOME/.local/opt/llvm16/lib/x86_64-unknown-linux-gnu:$LD_LIBRARY_PATH"
#
# # export CFLAGS=$CFLAGS
#
# export CXXFLAGS="-L$HOME/.local/opt/llvm16/lib $CXXFLAGS"
# export CXXFLAGS="-L$HOME/.local/opt/llvm16/lib/x86_64-unknown-linux-gnu $CXXFLAGS"
#
# export CXXFLAGS="-I$HOME/.local/opt/llvm16/include $CXXFLAGS"
# export CXXFLAGS="-I$HOME/.local/opt/llvm16/include/c++/v1 $CXXFLAGS"
# export CXXFLAGS="-I$HOME/.local/opt/llvm16/include/x86_64-unknown-linux-gnu/c++/v1 $CXXFLAGS"
# export CXXFLAGS="-stdlib=libc++ $CXXFLAGS"
#
# export CPPFLAGS=$CXXFLAGS
#
# # export LDFLAGS="-L$HOME/.local/opt/llvm16/lib $LDFLAGS"
# # export LDFLAGS="-L$HOME/.local/opt/llvm16/lib/x86_64-unknown-linux-gnu $LDFLAGS"
# export LDFLAGS="-stdlib=libc++ $LDFLAGS"
