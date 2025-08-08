#!/bin/bash

export CWD=$(pwd)
export HOME_PATH=$HOME
export FRAMEWORK_PATH=$CWD/fw
export LLVM_PATH=$CWD/llvm
export LLVM_INSTALL_PATH=$HOME_PATH/.local/dev/llvm16

echo "CWD: $CWD"
echo "HOME_PATH: $HOME_PATH"
echo "FRAMEWORK_PATH: $FRAMEWORK_PATH"
echo "LLVM_PATH: $LLVM_PATH"