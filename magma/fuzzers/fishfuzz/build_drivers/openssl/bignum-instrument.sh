#!/bin/bash
set -e

##
# Pre-requirements:
# - env TARGET: path to target work dir
# - env OUT: path to directory where artifacts are stored
# - env CC, CXX, FLAGS, LIBS, etc...
##


export TMP_DIR=$OUT/tmp-$1
export CC="$FUZZER/repo/instrument_openssl/aflgo-clang"
export CXX="$FUZZER/repo/instrument_openssl/aflgo-clang++"


# Set aflgo-instrumentation flags
$export CFLAGS="$CFLAGS -distance=$TMP_DIR/distance.cfg.txt"
$export CXXFLAGS="$CXXFLAGS -distance=$TMP_DIR/distance.cfg.txt"

cd "$TARGET/repo"

CONFIGURE_FLAGS=""
if [[ $CFLAGS = *sanitize=memory* ]]; then
  CONFIGURE_FLAGS="no-asm"
fi

# the config script supports env var LDLIBS instead of LIBS
export LDLIBS="$LIBS"

./config --debug enable-fuzz-libfuzzer enable-fuzz-afl disable-tests -DPEDANTIC \
    -DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION no-shared no-module \
    enable-tls1_3 enable-rc5 enable-md2 enable-ec_nistp_64_gcc_128 enable-ssl3 \
    enable-ssl3-method enable-nextprotoneg enable-weak-ssl-ciphers \
    $CFLAGS -fno-sanitize=alignment $CONFIGURE_FLAGS

make -j$(nproc) clean
make -j$(nproc) LDCMD="$CXX $CXXFLAGS"

fuzzers=$(find fuzz -executable -type f '!' -name \*.py '!' -name \*-test '!' -name \*.pl)
for f in $fuzzers; do
    fuzzer=$(basename $f)
    cp $f "$OUT/"
done

