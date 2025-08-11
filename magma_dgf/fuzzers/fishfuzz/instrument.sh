#!/bin/bash
set -e

##
# Pre-requirements:
# - env FUZZER: path to fuzzer work dir
# - env TARGET: path to target work dir
# - env MAGMA: path to Magma support files
# - env OUT: path to directory where artifacts are stored
# - env CFLAGS and CXXFLAGS must be set to link against Magma instrumentation
##


export AS="$FUZZER/repo/afl-as"
export LIBS="$LIBS -l:afl_driver.o -lstdc++"


"$MAGMA/build.sh"


get_target() {
    cd $OUT
    source "$TARGET/configrc"
    DRIVER=$(basename "$TARGET")
    for p in "${PROGRAMS[@]}"; do (
        folder=$OUT/tmp-$p
        if [ ! -d $folder ]; then
             mkdir $folder
        fi
        if [[ -d "$FUZZER/targets/$DRIVER" ]]; then
              cp -r "$FUZZER/targets/$DRIVER/BBtargets.txt" "$folder/BBtargets.txt"
        fi
    )
    echo "copy the target to tmp file"
    done

}


generate_bc() {
  
  unset LIBS
  echo -e "## Build by wllvm"
  export CC="wllvm" # "wllvm"
  export CXX="wllvm++"
  export LLVM_COMPILER=clang

  $CXX $CXXFLAGS -std=c++11 -c "$FUZZER/src/afl_driver.cpp" -fPIC -o "$OUT/afl_driver.o"

  "$TARGET/build.sh"

}


instrumentation_analysis(){
 
  cd "$OUT"
 
  source "$TARGET/configrc"
  for p in "${PROGRAMS[@]}"; do (
      extract-bc $p
      bc_path=$(find $OUT -name "$p.bc")
      TMP_DIR=$OUT/tmp-$p

      export ADDITIONAL_COV="-load $FUZZER/repo/afl-llvm-pass.so -test -outdir=$TMP_DIR -pmode=conly"
      export ADDITIONAL_ANALYSIS="-load $FUZZER/repo/afl-llvm-pass.so -test -outdir=$TMP_DIR -pmode=aonly -targets=$TMP_DIR/BBtargets.txt"
      opt $ADDITIONAL_COV $bc_path -o $OUT/$p-final.bc
      opt $ADDITIONAL_ANALYSIS $OUT/$p-final.bc -o $OUT/$p-temp.bc
  )
  done
}

distance_calculation(){
 
  cd "$OUT"
 
  source "$TARGET/configrc"
  for p in "${PROGRAMS[@]}"; do (
      bc_path=$(find $OUT -name "$p.bc")
      TMP_DIR="$OUT/tmp-$p"
      
      opt -dot-callgraph $bc_path 
      mv "$p.bc.callgraph.dot" $TMP_DIR/dot-files/callgraph.dot
      $FUZZER/repo/scripts/gen_initial_distance.py $TMP_DIR
  )
  done 
}

instrument(){

  export LIBS="-l:magma.o -lrt"
  cd "$OUT" 

  source "$TARGET/configrc"

  for p in "${PROGRAMS[@]}"; do (
     bc_path=$(find $OUT -name "$p-final.bc")
     TMP_DIR=$OUT/tmp-$p
     export ADDITIONAL_FUNC="-pmode=fonly -funcid=$TMP_DIR/funcid.csv -outdir=$TMP_DIR"
     pushd "$TARGET/repo"
     if [[ $TARGET = *libpng* ]]; then
              $FUZZER/repo/afl-clang-fast++ $CXXFLAGS -std=c++11 $ADDITIONAL_FUNC -I. $bc_path -o $OUT/$p.fuzz $LDFLAGS $LIBS -lz -lm
        elif [[ $TARGET = *libsndfile* ]]; then
              $FUZZER/repo/afl-clang-fast++ $CXXFLAGS -std=c++11 $ADDITIONAL_FUNC -I. $bc_path -o $OUT/$p.fuzz $LDFLAGS $LIBS -lmp3lame -lmpg123 -lFLAC -lvorbis -lvorbisenc -lopus -logg -lm
        elif [[ $TARGET = *libtiff* ]]; then
              WORK="$TARGET/work"
              $FUZZER/repo/afl-clang-fast++ $CXXFLAGS -std=c++11 $ADDITIONAL_FUNC -I. $bc_path -o $OUT/$p.fuzz $LDFLAGS $LIBS $WORK/lib/libtiffxx.a $WORK/lib/libtiff.a -lm -lz -ljpeg -Wl,-Bstatic -llzma -Wl,-Bdynamic
        elif [[ $TARGET = *libxml2* ]]; then
              $FUZZER/repo/afl-clang-fast++ $CXXFLAGS -std=c++11 $ADDITIONAL_FUNC -I. $bc_path -o $OUT/$p.fuzz $LDFLAGS $LIBS .libs/libxml2.a -lz -llzma -lm
        elif [[ $TARGET = *lua* ]]; then
              $FUZZER/repo/afl-clang-fast++ $CXXFLAGS -std=c++11 $ADDITIONAL_FUNC -I. $bc_path -o $OUT/$p.fuzz $LDFLAGS $LIBS $TARGET/repo/liblua.a -DLUA_USE_LINUX -DLUA_USE_READLINE -lreadline -lm -ldl
        elif [[ $TARGET = *poppler* ]]; then
              WORK="$TARGET/work"
              $FUZZER/repo/afl-clang-fast++ $CXXFLAGS -std=c++11 $ADDITIONAL_FUNC -I. $bc_path -o $OUT/$p.fuzz $LDFLAGS $LIBS -I"$WORK/poppler/cpp" -I"$TARGET/repo/cpp" \
              "$WORK/poppler/cpp/libpoppler-cpp.a" "$WORK/poppler/libpoppler.a" "$WORK/lib/libfreetype.a" -lz -llzma -ljpeg -lz -lopenjp2 -lpng -ltiff -llcms2 -lm -lpthread -pthread
        elif [[ $TARGET = *openssl* ]]; then
              $FUZZER/repo/afl-clang-fast++ $CXXFLAGS -std=c++11 $ADDITIONAL_FUNC -I. $bc_path -o $OUT/$p.fuzz $LDFLAGS $LIBS -lpthread ./libcrypto.a ./libssl.a
        elif [[ $TARGET = *php* ]]; then
              $FUZZER/repo/afl-clang-fast++ $CXXFLAGS -std=c++11 $ADDITIONAL_FUNC -I. $bc_path -o $OUT/$p.fuzz $LDFLAGS $LIBS -lstdc++ -lpthread -lboost_fiber -lboost_context -ldl -licuuc -licui18n -licuio -licutu
        elif [[ $TARGET = *sqlite3* ]]; then
              WORK="$TARGET/work"
              $FUZZER/repo/afl-clang-fast++ $CXXFLAGS -std=c++11 $ADDITIONAL_FUNC -I. $bc_path -o $OUT/$p.fuzz $LDFLAGS $LIBS $WORK/.libs/libsqlite3.a -lpthread -pthread -ldl -lm -lz

        else
             echo "Could not support this target $TARGET"
        fi
        popd
  )
  done 
}

get_target
generate_bc
instrumentation_analysis
distance_calculation
instrument
