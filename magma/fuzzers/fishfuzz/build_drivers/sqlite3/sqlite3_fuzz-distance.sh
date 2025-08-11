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

# build the sqlite3 library
cd "$TARGET/repo"

export WORK="$TARGET/work"
rm -rf "$WORK"
mkdir -p "$WORK"
cd "$WORK"

export CFLAGS="$CFLAGS -DSQLITE_MAX_LENGTH=128000000 \
               -DSQLITE_MAX_SQL_LENGTH=128000000 \
               -DSQLITE_MAX_MEMORY=25000000 \
               -DSQLITE_PRINTF_PRECISION_LIMIT=1048576 \
               -DSQLITE_DEBUG=1 \
               -DSQLITE_MAX_PAGE_COUNT=16384"

"$TARGET/repo"/configure --disable-shared --enable-rtree
make clean
make -j$(nproc)
make sqlite3.c

$CC $CFLAGS -I. \
    "$TARGET/repo/test/ossfuzz.c" "./sqlite3.o" \
    -o "$OUT/sqlite3_fuzz" \
    $LDFLAGS $LIBS -pthread -ldl -lm


# Clean up
cat $TMP_DIR/BBnames.txt | grep -v "^$"| rev | cut -d: -f2- | rev | sort | uniq > $TMP_DIR/BBnames2.txt && mv $TMP_DIR/BBnames2.txt $TMP_DIR/BBnames.txt

cat $TMP_DIR/BBcalls.txt | grep -Ev "^[^,]*$|^([^,]*,){2,}[^,]*$"| sort | uniq > $TMP_DIR/BBcalls2.txt && mv $TMP_DIR/BBcalls2.txt $TMP_DIR/BBcalls.txt

$FUZZER/repo/distance/gen_distance_fast.py $OUT $TMP_DIR $1

