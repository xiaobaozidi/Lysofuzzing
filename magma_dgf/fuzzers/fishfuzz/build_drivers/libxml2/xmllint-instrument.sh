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
./autogen.sh \
        --with-http=no \
        --with-python=no \
        --with-lzma=yes \
        --with-threads=no \
        --disable-shared
make -j$(nproc) clean
make -j$(nproc) all

cp xmllint "$OUT/"

for fuzzer in libxml2_xml_read_memory_fuzzer libxml2_xml_reader_for_file_fuzzer; do
  $CXX $CXXFLAGS -std=c++11 -Iinclude/ -I"$TARGET/src/" \
      "$TARGET/src/$fuzzer.cc" -o "$OUT/$fuzzer" \
      .libs/libxml2.a $LDFLAGS $LIBS -lz -llzma
done
