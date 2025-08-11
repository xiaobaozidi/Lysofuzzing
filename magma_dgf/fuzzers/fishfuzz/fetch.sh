#!/bin/bash
set -e

##
# Pre-requirements:
# - env FUZZER: path to fuzzer work dir
##

cp "$FUZZER/src/afl_driver.cpp" "$FUZZER/repo/afl_driver.cpp"
 
