#!/bin/bash
set -e

apt-get update --fix-missing && \
    apt-get install -y make build-essential git wget cmake gawk libboost-all-dev gdb 


# Dowdload llvm-11

mkdir -p /llvm-11
wget https://github.com/llvm/llvm-project/releases/download/llvmorg-11.0.1/clang+llvm-11.0.1-x86_64-linux-gnu-ubuntu-16.04.tar.xz
tar -xf clang+llvm-11.0.1-x86_64-linux-gnu-ubuntu-16.04.tar.xz  -C /llvm-11 --strip-components=1
rm clang+llvm-11.0.1-x86_64-linux-gnu-ubuntu-16.04.tar.xz

#wget https://github.com/llvm/llvm-project/releases/download/llvmorg-11.1.0/llvm-project-11.1.0.src.tar.xz
#tar -xf llvm-project-11.1.0.src.tar.xz -C /llvm-11 --strip-components=1
#cd /llvm-11 && mkdir build
#cd build
#cmake -G "Unix Makefiles" -DLLVM_ENABLE_PROJECTS='clang;compiler-rt' -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi" -DCMAKE_BUILD_TYPE=Debug -DLLVM_ENABLE_ASSERTIONS=On ../llvm
#make -j10



# Dowdload llvm-15
#mkdir -p /llvm-15
#wget https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.6/clang+llvm-15.0.6-x86_64-linux-gnu-ubuntu-18.04.tar.xz
#tar -xf clang+llvm-15.0.6-x86_64-linux-gnu-ubuntu-18.04.tar.xz -C /llvm-15 --strip-components=1
#rm clang+llvm-15.0.6-x86_64-linux-gnu-ubuntu-18.04.tar.xz


apt-get install -y python3 python3-dev python3-pip
pip3 install --upgrade pip
pip3 install networkx pydot
pip3 install wllvm
