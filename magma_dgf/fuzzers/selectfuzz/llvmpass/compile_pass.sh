#/bin/bash
set -e

export CXX=`which clang++`
export CC=`which clang`
export LLVM_CONFIG=`which llvm-config`
export BUILD_DIR=$PWD/build

######Remove the file#########
if [ -d $BUILD_DIR ];
 then rm -rf $BUILD_DIR;
fi

mkdir $BUILD_DIR
cd $BUILD_DIR

cmake ../
make clean; make -j4
