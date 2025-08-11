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
make clean all

cd "$FUZZER/repo/llvm_mode"
make clean all

cd "$FUZZER/llvmpass"
./compile_pass.sh

cd "$FUZZER/repo"
"./afl-clang-fast++" $CXXFLAGS -std=c++11 -c "afl_driver.cpp" -fPIC -o "$OUT/afl_driver.o"


