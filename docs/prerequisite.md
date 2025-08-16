
# Prerequisites

We evaluate Orthrus throughly on Ubuntu 20.04. If you are using a similar OS release, you can refer to the commands below. Otherwise, you can download related packages yourself.

Note that you should use cmake version >= 3.20.

> Please make sure you have prepared the environment of performance, with the script **init.sh** in the main branch.

## Environment for Fault Injection Evaluation

If you already set up the env on the machine, you only need to execute the "mkdir " scripts in the setup.sh 

```shell
# !!!! DO NOT RUN IN ROOT USER!!!!
./fw/scripts/table2_setup.sh
# this shell will install our modified LLVM clang compiler and python testing platform. 

# The llvm install may stuck sometimes due to LLVM itself's bug. If stuck at "Downloading", you need to run the related ./build.sh manually in your terminal.
```
It will build and install our modified Clang compiler with fault injection features, and configure an automatic testing platform written in python.
