#!/bin/bash


CWD=$(pwd)
HOME_PATH=$HOME
FRAMEWORK_PATH=$CWD/fw
LLVM_PATH=$CWD/llvm


apt install -y python3-pip libssl-dev lzma
# Setup LLVM
pushd $LLVM_PATH
echo "Building LLVM CLANG 16 for FJ"

./build.sh

echo "Build Finish"
popd

# Setup Python Automated testing environment
pushd $FRAMEWORK_PATH

sudo apt-get install libsqlite3-dev
pyenv install 3.11.10
pyenv global 3.11.10
pip install tqdm testresources poetry coloredlogs pydantic

git clone https://github.com/pyenv/pyenv.git ~/.pyenv

mkdir -p logs
mkdir -p output
mkdir -p tests
# ./scripts/env.sh
popd


echo "Prepare Finish"