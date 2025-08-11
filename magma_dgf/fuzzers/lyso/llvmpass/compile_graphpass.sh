#!/bin/bash

export BUILD_DIR=$PWD/build
export LLVM=$1
export PATH=${LLVM_15}:$PATH

######Remove the file#########
if [ -d $BUILD_DIR ];
 then rm -rf $BUILD_DIR;
fi

mkdir $BUILD_DIR

export CC=${LLVM}/clang; export CXX=${LLVM}/clang++

cd $BUILD_DIR

cmake ../

make clean; make -j4
