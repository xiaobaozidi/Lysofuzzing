#!/bin/bash
set -e
set -x
##
# Pre-requirements:
# - env FUZZER: path to fuzzer work dir
# - env TARGET: path to target work dir
# - env MAGMA: path to Magma support files
# - env PROGRAM: name of program to run (should be found in $OUT)
# - env OUT: path to directory where artifacts are stored
# - env CFLAGS and CXXFLAGS must be set to link against Magma instrumentation
##

export LIBS="$LIBS -l:afl_driver.o -lstdc++"
export LLVM_11=/llvm-11/bin

 
"$MAGMA/build.sh"

get_target() {
    cd $OUT
    source "$TARGET/configrc"
    for p in "${PROGRAMS[@]}"; do (
        folder=$OUT/tmp-$p
        if [ ! -d $folder ]; then
           mkdir $folder
        fi
        DRIVER=$(basename "$TARGET")
        if [[ -d "$FUZZER/targets/$DRIVER" ]]; then
           cp -r "$FUZZER/targets/$DRIVER/target_$p.txt" "$folder/target.txt"
        fi

    )
    done
}


configure_project() {
        unset LIBS
	echo -e "## Build by wllvm"
	export PATH="$1:$PATH"
	export CC="wllvm" # "wllvm"
	export CXX="wllvm++"
	export LLVM_COMPILER=clang
        $CXX $CXXFLAGS -std=c++11 -c "$FUZZER/src/afl_driver.cpp" -fPIC -o "$OUT/afl_driver.o"
	"$TARGET/build.sh"
}


insert_id(){
    export PATH="$1:$PATH"
    
    cd "$OUT"
    source "$TARGET/configrc" 
    for p in "${PROGRAMS[@]}"; do (
        extract-bc $p
        bc_path=$(find $OUT -name "$p.bc")
        $1/opt -load $FUZZER/repo/afl-llvm-pass.so -test -mode=is_insert $bc_path -o $OUT/$p-id.bc
    )
    done
}


distance_calculation(){
    export GRAPHPASS="$FUZZER/llvmpass/build/GraphPass/libGraphPass.so"
    export SCRIPTS="$FUZZER/scripts"
    export PATH="$1:$PATH"

    cd "$OUT"
    source "$TARGET/configrc"
    for p in "${PROGRAMS[@]}"; do (
        folder=$OUT/tmp-$p
        bc_path=$(find $OUT -name "$p-id.bc")
        $1/opt -load $GRAPHPASS -GraphPass -targets=$folder/target.txt -outdir=$folder $bc_path
        $SCRIPTS/distance_calculation_cg.py $SCRIPTS $folder
        $SCRIPTS/distance_calculation_cfg.py $SCRIPTS $folder
    )
    done
}


trace_instrumentation() {
    export PATH="$1:$PATH"
    export LIBS="-l:magma.o -lrt"
    cd "$OUT"
    source "$TARGET/configrc"
    for p in "${PROGRAMS[@]}"; do (
        folder=$OUT/tmp-$p
        dist_file=$folder/distance.cfg.txt
        trace_file=$folder/IntraBB.txt
        extract-bc $p
        bc_path=$(find $OUT -name "$p-id.bc")
        pushd "$TARGET/repo"
        if [[ $TARGET = *libpng* ]]; then
              $FUZZER/repo/afl-clang-fast++ $CXXFLAGS -std=c++11 -outdir=$folder -trace=$trace_file -distance=$dist_file -I. $bc_path -o $OUT/$p.fuzz $LDFLAGS $LIBS -lz -lm
        elif [[ $TARGET = *libsndfile* ]]; then
              $FUZZER/repo/afl-clang-fast++ $CXXFLAGS -std=c++11 -outdir=$folder -trace=$trace_file -distance=$dist_file -I. $bc_path -o $OUT/$p.fuzz $LDFLAGS $LIBS -lmp3lame -lmpg123 -lFLAC -lvorbis -lvorbisenc -lopus -logg -lm
        elif [[ $TARGET = *libtiff* ]]; then
              WORK="$TARGET/work"
              $FUZZER/repo/afl-clang-fast++ $CXXFLAGS -std=c++11 -outdir=$folder -trace=$trace_file -distance=$dist_file -I. $bc_path -o $OUT/$p.fuzz $LDFLAGS $LIBS $WORK/lib/libtiffxx.a $WORK/lib/libtiff.a -lm -lz -ljpeg -Wl,-Bstatic -llzma -Wl,-Bdynamic
        elif [[ $TARGET = *libxml2* ]]; then
              $FUZZER/repo/afl-clang-fast++ $CXXFLAGS -std=c++11 -outdir=$folder -trace=$trace_file -distance=$dist_file -I. $bc_path -o $OUT/$p.fuzz $LDFLAGS $LIBS .libs/libxml2.a -lz -llzma -lm
        elif [[ $TARGET = *lua* ]]; then
              $FUZZER/repo/afl-clang-fast++ $CXXFLAGS -std=c++11 -outdir=$folder -trace=$trace_file -distance=$dist_file -I. $bc_path -o $OUT/$p.fuzz $LDFLAGS $LIBS $TARGET/repo/liblua.a -DLUA_USE_LINUX -DLUA_USE_READLINE -lreadline -lm -ldl
        elif [[ $TARGET = *poppler* ]]; then
              WORK="$TARGET/work"
              $FUZZER/repo/afl-clang-fast++ $CXXFLAGS -std=c++11 -outdir=$folder -trace=$trace_file -distance=$dist_file -I. $bc_path -o $OUT/$p.fuzz $LDFLAGS $LIBS -I"$WORK/poppler/cpp" -I"$TARGET/repo/cpp" \
              "$WORK/poppler/cpp/libpoppler-cpp.a" "$WORK/poppler/libpoppler.a" "$WORK/lib/libfreetype.a" -lz -llzma -ljpeg -lz -lopenjp2 -lpng -ltiff -llcms2 -lm -lpthread -pthread
        elif [[ $TARGET = *openssl* ]]; then
              $FUZZER/repo/afl-clang-fast++ $CXXFLAGS -std=c++11 -outdir=$folder -trace=$trace_file -distance=$dist_file -I. $bc_path -o $OUT/$p.fuzz $LDFLAGS $LIBS -lpthread ./libcrypto.a ./libssl.a
        elif [[ $TARGET = *php* ]]; then
              $FUZZER/repo/afl-clang-fast++ $CXXFLAGS -std=c++11 -outdir=$folder -trace=$trace_file -distance=$dist_file -I. $bc_path -o $OUT/$p.fuzz $LDFLAGS $LIBS -lstdc++ -lpthread -lboost_fiber -lboost_context -ldl -licuuc -licui18n -licuio -licutu
        elif [[ $TARGET = *sqlite3* ]]; then
              WORK="$TARGET/work"
              $FUZZER/repo/afl-clang-fast++ $CXXFLAGS -std=c++11 -outdir=$folder -trace=$trace_file -distance=$dist_file -I. $bc_path -o $OUT/$p.fuzz $LDFLAGS $LIBS $WORK/.libs/libsqlite3.a -lpthread -pthread -ldl -lm -lz

        else
             echo "Could not support this target $TARGET"
        fi
        popd
    )
    done
}




get_target
configure_project $LLVM_11
insert_id $LLVM_11
distance_calculation $LLVM_11
trace_instrumentation $LLVM_11
