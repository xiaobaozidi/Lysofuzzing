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

WORK="$TARGET/work"
rm -rf "$WORK"
mkdir -p "$WORK"
mkdir -p "$WORK/lib" "$WORK/include"

cd "$TARGET/repo"
./autogen.sh
./configure --disable-shared --prefix="$WORK"
make -j$(nproc) clean
make -j$(nproc)
make install

cp "$WORK/bin/tiffcp" "$OUT/"
$CXX $CXXFLAGS -std=c++11 -I$WORK/include \
    contrib/oss-fuzz/tiff_read_rgba_fuzzer.cc -o $OUT/tiff_read_rgba_fuzzer \
    $WORK/lib/libtiffxx.a $WORK/lib/libtiff.a -lz -ljpeg -Wl,-Bstatic -llzma -Wl,-Bdynamic \
    $LDFLAGS $LIBS

