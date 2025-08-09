#!/bin/bash


CWD=$(pwd)
HOME_PATH=$HOME
FRAMEWORK_PATH=$CWD/fw
LLVM_PATH=$CWD/llvm

echo "HOME_PATH: $HOME_PATH"

sudo apt install -y python3-pip libssl-dev lzma libsqlite3-dev

mkdir -p $HOME_PATH/data

cp $CWD/fj_targets/wordcount_orthrus/benchmarks/word_count/example.txt $HOME_PATH/data/example.txt

git submodule init
git submodule update --recursive

# Setup LLVM
pushd $LLVM_PATH
echo "Building LLVM CLANG 16 for FJ"

./build.sh

echo "Build Finish"
popd

# Setup Python Automated testing environment
pushd $FRAMEWORK_PATH

git clone https://github.com/pyenv/pyenv.git $HOME/.pyenv

export PYENV_ROOT="$HOME/.pyenv"
export PATH="$PYENV_ROOT/bin:$PATH"
eval "$(pyenv init --path)"
eval "$(pyenv init -)"

pyenv install 3.11.10
pyenv global 3.11.10

wait 

pip install tqdm testresources poetry coloredlogs pydantic

mkdir -p logs
mkdir -p output
mkdir -p tests
# ./scripts/env.sh
popd


echo "Prepare Finish"