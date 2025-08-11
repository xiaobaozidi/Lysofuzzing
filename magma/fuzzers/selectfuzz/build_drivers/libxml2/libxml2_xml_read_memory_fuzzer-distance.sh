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
export ADDITIONAL="-targets=$TMP_DIR/BBtargets.txt -outdir=$TMP_DIR -flto -fuse-ld=gold -Wl,-plugin-opt=save-temps"
export CFLAGS="$CFLAGS $ADDITIONAL"
export CXXFLAGS="$CXXFLAGS $ADDITIONAL"


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
cp *.bc "$OUT/"

for fuzzer in libxml2_xml_read_memory_fuzzer libxml2_xml_reader_for_file_fuzzer; do
  $CXX $CXXFLAGS -std=c++11 -Iinclude/ -I"$TARGET/src/" \
      "$TARGET/src/$fuzzer.cc" -o "$OUT/$fuzzer" \
      .libs/libxml2.a $LDFLAGS $LIBS -lz -llzma
done


# Clean up
cat $TMP_DIR/BBnames.txt | rev | cut -d: -f2- | rev | sort | uniq > $TMP_DIR/BBnames2.txt && mv $TMP_DIR/BBnames2.txt $TMP_DIR/BBnames.txt
cat $TMP_DIR/BBcalls.txt | sort | uniq > $TMP_DIR/BBcalls2.txt && mv $TMP_DIR/BBcalls2.txt $TMP_DIR/BBcalls.txt



$FUZZER/repo/scripts/genDistance.sh $OUT $TMP_DIR $1

