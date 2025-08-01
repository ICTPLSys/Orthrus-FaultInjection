
# Prerequisities

We evaluate Orthrus throughly on Ubuntu 20.04. If you are using a similiar OS release, you can refer to commands below. Else, you can download related packages yourself.

Not that you should use camke version >= 3.20.

## Enviroment For Performance Test

```shell
apt update

apt install -y openssh-server
apt install -y git
apt install -y just
apt install -y curl wget

# install shuf
apt install -y binutils

# the lib zlib
apt install -y zlib1g-dev
apt install -y lsb-release wget software-properties-common gnupg

# install taskset
apt install -y util-linux

# add gcc-13 into apt
yes "" | sudo add-apt-repository ppa:ubuntu-toolchain-r/test

# install libboost and libboost-program-options
apt install -y libboost-dev libboost-program-options-dev

# gcc-13 compiler
apt install -y gcc-13 g++-13

# toolz
apt install -y python3-toolz

# install llvm-16
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 16

# install cmake (optional, you can install by yourself)
wget https://cmake.org/files/v3.29/cmake-3.29.8-linux-x86_64.tar.gz
tar -zxvf cmake-3.29.8-linux-x86_64.tar.gz
export PATH=$PWD/cmake-3.29.8-linux-x86_64/bin:$PATH
```

## Environment for Fault Injection Evaluation

run table2_env.sh.

It will build and install our modified Clang compiler with fault injection features, and configure an automatic testing platform written in python.
