#/bin/bash
set -e

export CXX=`which clang++`
export CC=`which clang`
export LLVM_CONFIG=`which llvm-config`


##
# Pre-requirements:
# - env FUZZER: path to fuzzer work dir
##

if [ ! -d "$FUZZER/repo" ]; then
    echo "fetch.sh must be executed first."
    exit 1
fi

cd "$FUZZER/repo"
make -C llvm_mode
make -C dyncfg
make

cp afl-llvm-rt.o $OUT/afl-llvm-rt.o
