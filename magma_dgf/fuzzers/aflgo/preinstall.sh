#!/bin/bash
set -e

apt-get update --fix-missing && \
    apt-get install -y build-essential make cmake ninja-build git binutils-gold binutils-dev curl wget python3


# Download llvm-11
mkdir -p /llvm-11
mkdir -p /llvm-11/llvm
mkdir -p /llvm-11/clang
mkdir -p /llvm-11/compiler-rt
wget https://github.com/llvm/llvm-project/releases/download/llvmorg-11.0.0/llvm-11.0.0.src.tar.xz
wget https://github.com/llvm/llvm-project/releases/download/llvmorg-11.0.0/clang-11.0.0.src.tar.xz
wget https://github.com/llvm/llvm-project/releases/download/llvmorg-11.0.0/compiler-rt-11.0.0.src.tar.xz

tar -xf llvm-11.0.0.src.tar.xz -C /llvm-11/llvm --strip-components=1
tar -xf clang-11.0.0.src.tar.xz -C /llvm-11/clang --strip-components=1
tar -xf compiler-rt-11.0.0.src.tar.xz -C /llvm-11/compiler-rt --strip-components=1

pushd /llvm-11

mkdir -p build

pushd build

cmake -G "Ninja" \
      -DCMAKE_BUILD_TYPE=Release \
      -DLLVM_TARGETS_TO_BUILD="X86" \
      -DLLVM_BINUTILS_INCDIR=/usr/include \
      -DLLVM_ENABLE_PROJECTS="clang;compiler-rt" \
      -DLLVM_BUILD_TESTS=OFF \
      -DLLVM_INCLUDE_TESTS=OFF \
      -DLLVM_BUILD_BENCHMARKS=OFF \
      -DLLVM_INCLUDE_BENCHMARKS=OFF \
      ../llvm
ninja; ninja install


# Install LLVMgold in bfd-plugins
mkdir -p /usr/lib/bfd-plugins
cp /usr/local/lib/libLTO.so /usr/lib/bfd-plugins
cp /usr/local/lib/LLVMgold.so /usr/lib/bfd-plugins


apt-get install -y python3-dev python3-pip pkg-config autoconf automake libtool-bin gawk libboost-all-dev
python3 -m pip install networkx pydot pydotplus
