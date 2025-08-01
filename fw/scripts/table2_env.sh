#!/bin/bash

HOME_PATH=
FRAMEWORK_PATH=
LLVM_PATH=


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