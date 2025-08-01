#!/usr/bin/env bash

set -xe

apt update

apt install -y openssh-server
apt install -y git
apt install -y just
apt install -y curl wget gzip
apt install -y binutils
apt install -y zlib1g-dev
apt install -y lsb-release wget software-properties-common gnupg
yes "" | sudo add-apt-repository ppa:ubuntu-toolchain-r/test

apt install -y libboost-dev
apt install -y gcc-13 g++-13

apt install -y libboost-program-options-dev

apt install -y binutils python3-toolz

apt install -y libboost-dev

wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 16

wget https://cmake.org/files/v3.29/cmake-3.29.8-linux-x86_64.tar.gz
tar -zxvf cmake-3.29.8-linux-x86_64.tar.gz
export PATH=$PWD/cmake-3.29.8-linux-x86_64/bin:$PATH


./fw/scripts/table2_env.sh


