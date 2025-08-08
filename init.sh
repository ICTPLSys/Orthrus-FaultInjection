#!/usr/bin/env bash

set -xe


pushd $HOME

apt update

apt install -y openssh-server
apt install -y git

apt install -y curl wget gzip
apt install -y binutils
apt install -y zlib1g-dev
apt install -y lsb-release wget software-properties-common gnupg
yes "" | add-apt-repository ppa:ubuntu-toolchain-r/test

apt install -y libboost-dev

apt install -y libboost-program-options-dev

apt install -y binutils python3-toolz

apt install -y libboost-dev build-essential

apt install -y gcc-13 g++-13 ninja-build pkg-config

apt install -y libgtest-dev libgflags-dev vim


wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
./llvm.sh 16

wget https://cmake.org/files/v3.29/cmake-3.29.8-linux-x86_64.tar.gz
tar -zxvf cmake-3.29.8-linux-x86_64.tar.gz

export PATH=$HOME/cmake-3.29.8-linux-x86_64/bin:$PATH


curl --proto '=https' --tlsv1.2 -sSf https://just.systems/install.sh | bash -s -- --to /usr/bin

update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100
update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100

# export CC=/usr/bin/gcc-13
# export CXX=/usr/bin/g++-13

# ./fw/scripts/table2_env.sh

popd
