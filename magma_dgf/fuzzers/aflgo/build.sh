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
cd afl-2.57b
make clean all


cd "$FUZZER/repo"
cd  instrument
make clean all


cd "$FUZZER/repo"
cd  instrument_openssl
make clean all


cd "$FUZZER/repo"
cd  distance/distance_calculator
cmake ./
cmake --build ./


cd "$FUZZER/repo"
$FUZZER/repo/instrument/aflgo-clang++ $CXXFLAGS -std=c++11 -c "afl_driver.cpp" -fPIC -o "$OUT/afl_driver.o"
