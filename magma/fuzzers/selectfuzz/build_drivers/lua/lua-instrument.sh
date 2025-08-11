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

# build lua library
cd "$TARGET/repo"
make -j$(nproc) clean
make -j$(nproc) liblua.a

# build driver
make -j$(nproc) lua.o
$CXX $CXXFLAGS -std=c++11 -I. \
     -o $OUT/lua \
     $LDFLAGS lua.o liblua.a $LIBS -ldl -lreadline

