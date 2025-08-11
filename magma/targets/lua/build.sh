#!/bin/bash
set -e

##
# Pre-requirements:
# - env TARGET: path to target work dir
# - env OUT: path to directory where artifacts are stored
# - env CC, CXX, FLAGS, LIBS, etc...
##

if [ ! -d "$TARGET/repo" ]; then
    echo "fetch.sh must be executed first."
    exit 1
fi

if [[ $FUZZER = *lyso* ]] || [[ $FUZZER = *fishfuzz* ]] ; then
    export LIBS="-l:magma.o -lrt -l:afl_driver.o -lstdc++ -l:afl-llvm-rt.o"
    echo "LIBS variable set to $LIBS"
fi


# build lua library
cd "$TARGET/repo"
make -j$(nproc) clean
make -j$(nproc) liblua.a

cp liblua.a "$OUT/"

# build driver
make -j$(nproc) lua
cp lua "$OUT/"
