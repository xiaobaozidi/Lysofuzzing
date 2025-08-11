#!/bin/bash
set -e

##
# Pre-requirements:
# - env TARGET: path to target work dir
# - env OUT: path to directory where artifacts are stored
# - env CC, CXX, FLAGS, LIBS, etc...
##


export TMP_DIR=$OUT/tmp-$1

# Set aflgo-instrumentation flags
export CFLAGS="$CFLAGS -distance=$TMP_DIR/distance.cfg.txt"
export CXXFLAGS="$CXXFLAGS -distance=$TMP_DIR/distance.cfg.txt"

cd "$TARGET/repo"
./autogen.sh
./configure --disable-shared --enable-ossfuzzers
make -j$(nproc) clean
make -j$(nproc) ossfuzz/sndfile_fuzzer

cp -v ossfuzz/sndfile_fuzzer $OUT/
