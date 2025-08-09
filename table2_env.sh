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

export PATH=$HOME/cmake-3.29.8-linux-x86_64/bin:$PATH
export PATH=$LLVM_INSTALL_PATH/bin:$PATH
export PYENV_ROOT="$HOME/.pyenv"
export PATH="$PYENV_ROOT/bin:$PATH"
eval "$(pyenv init --path)"
eval "$(pyenv init -)"