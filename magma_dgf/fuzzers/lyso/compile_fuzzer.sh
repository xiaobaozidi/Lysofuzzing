#!/bin/bash
export LLVM=$1 
export PATH=${LLVM}:$PATH

cd "$FUZZER/repo"
CC=clang make -j $(nproc)
CC=clang make -j $(nproc) -C llvm_mode

cp afl-llvm-rt.o $OUT/afl-llvm-rt.o
