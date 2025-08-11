#!/bin/bash
set -e


apt-get update --fix-missing && \
    apt-get install -y build-essential make cmake git ninja-build subversion python2.7 binutils-gold binutils-dev curl wget

#Download Cmake 3.20
#wget https://github.com/Kitware/CMake/releases/download/v3.20.2/cmake-3.20.2-Linux-x86_64.sh
#chmod +x cmake-3.20.2-Linux-x86_64.sh
#sudo ./cmake-3.20.2-Linux-x86_64.sh --prefix=/usr/local --exclude-subdir

# Download llvm-4
#mkdir /magma/build; cd /magma/build
#mkdir llvm_tools; cd llvm_tools
#wget http://releases.llvm.org/4.0.0/llvm-4.0.0.src.tar.xz
#wget http://releases.llvm.org/4.0.0/cfe-4.0.0.src.tar.xz
#wget http://releases.llvm.org/4.0.0/compiler-rt-4.0.0.src.tar.xz
#wget http://releases.llvm.org/4.0.0/libcxx-4.0.0.src.tar.xz
#wget http://releases.llvm.org/4.0.0/libcxxabi-4.0.0.src.tar.xz
#tar xf llvm-4.0.0.src.tar.xz
#tar xf cfe-4.0.0.src.tar.xz
#tar xf compiler-rt-4.0.0.src.tar.xz
#tar xf libcxx-4.0.0.src.tar.xz
#tar xf libcxxabi-4.0.0.src.tar.xz
#mv cfe-4.0.0.src /magma/build/llvm_tools/llvm-4.0.0.src/tools/clang
#mv compiler-rt-4.0.0.src /magma/build/llvm_tools/llvm-4.0.0.src/projects/compiler-rt
#mv libcxx-4.0.0.src /build/llvm_tools/llvm-4.0.0.src/projects/libcxx
#mv libcxxabi-4.0.0.src /build/llvm_tools/llvm-4.0.0.src/projects/libcxxabi



mkdir -p $FUZZER/llvm_tools/build; cd $FUZZER/llvm_tools/build
cmake -G "Ninja" \
      -DCMAKE_CXX_FLAGS="-Wno-implicit-fallthrough" \
      -DCMAKE_C_FLAGS="-Wno-implicit-fallthrough" \
      -DLIBCXX_ENABLE_SHARED=OFF -DLIBCXX_ENABLE_STATIC_ABI_LIBRARY=ON \
      -DCMAKE_BUILD_TYPE=Release -DLLVM_TARGETS_TO_BUILD="X86" \
      -DLLVM_BINUTILS_INCDIR=/usr/include $FUZZER/llvm_tools/llvm-4.0.0.src

ninja; ninja install

# Install LLVMgold in bfd-plugins
mkdir -p /usr/lib/bfd-plugins
cp /usr/local/lib/libLTO.so /usr/lib/bfd-plugins
cp /usr/local/lib/LLVMgold.so /usr/lib/bfd-plugins


apt-get install -y python3-dev python3-pip pkg-config autoconf automake libtool-bin gawk libboost-all-dev
python3 -m pip install networkx pydot pydotplus
