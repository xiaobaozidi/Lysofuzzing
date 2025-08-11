#!/bin/bash
set -e

##
# Pre-requirements:
# - env FUZZER: path to fuzzer work dir
##

if [ ! -d "$FUZZER/repo" ]; then
     echo "fetch.sh must be executed first."
     exit 1
 fi


cd "$FUZZER/llvmpass"
./compile_graphpass.sh /llvm-11/bin



cd "$FUZZER"
./compile_fuzzer.sh /llvm-11/bin

