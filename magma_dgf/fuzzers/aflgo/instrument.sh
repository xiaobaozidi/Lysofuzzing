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


export CC="$FUZZER/repo/instrument/aflgo-clang"
export CXX="$FUZZER/repo/instrument/aflgo-clang++"
export AS="$FUZZER/repo/afl-2.57b/afl-as"
export LIBS="$LIBS -l:afl_driver.o -lstdc++"


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
              cp -r "$FUZZER/targets/$DRIVER/BBtargets.txt" "$folder/BBtargets.txt"
        fi
    )
    echo "copy the target to tmp file"
    done

}

generate_cg_cfg() {


        source "$TARGET/configrc"
        DRIVER=$(basename "$TARGET")
        for p in "${PROGRAMS[@]}"; do (
            if [[ $TARGET = *libpng* ]]; then
		$FUZZER/build_drivers/$DRIVER/$p-distance.sh $p
            elif [[ $TARGET = *libsndfile* ]]; then
		$FUZZER/build_drivers/$DRIVER/$p-distance.sh $p
            elif [[ $TARGET = *libtiff* ]]; then
		$FUZZER/build_drivers/$DRIVER/$p-distance.sh $p
            elif [[ $TARGET = *libxml2* ]]; then
		$FUZZER/build_drivers/$DRIVER/$p-distance.sh $p
            elif [[ $TARGET = *lua* ]]; then
		$FUZZER/build_drivers/$DRIVER/$p-distance.sh $p
            elif [[ $TARGET = *poppler* ]]; then
                $FUZZER/build_drivers/$DRIVER/$p-distance.sh $p
            elif [[ $TARGET = *openssl* ]]; then
                #$FUZZER/build_drivers/$DRIVER/$p-distance.sh $p
                continue
            elif [[ $TARGET = *php* ]]; then
                $FUZZER/build_drivers/$DRIVER/$p-distance.sh $p
            elif [[ $TARGET = *sqlite3* ]]; then
                $FUZZER/build_drivers/$DRIVER/$p-distance.sh $p
            else 
		echo "Could not support this target $TARGET"
                exit 1
            fi
         )
         done
}


instrument() {

	source "$TARGET/configrc"
        DRIVER=$(basename "$TARGET")
        for p in "${PROGRAMS[@]}"; do (
            if [[ $TARGET = *libpng* ]]; then
                $FUZZER/build_drivers/$DRIVER/$p-instrument.sh $p
            elif [[ $TARGET = *libsndfile* ]]; then
                $FUZZER/build_drivers/$DRIVER/$p-instrument.sh $p
            elif [[ $TARGET = *libtiff* ]]; then
                $FUZZER/build_drivers/$DRIVER/$p-instrument.sh $p
            elif [[ $TARGET = *libxml2* ]]; then
                $FUZZER/build_drivers/$DRIVER/$p-instrument.sh $p
            elif [[ $TARGET = *lua* ]]; then
                $FUZZER/build_drivers/$DRIVER/$p-instrument.sh $p
            elif [[ $TARGET = *poppler* ]]; then
                $FUZZER/build_drivers/$DRIVER/$p-instrument.sh $p
            elif [[ $TARGET = *openssl* ]]; then
                cp $FUZZER/openssl_binary/distance.cfg.txt $OUT
                $FUZZER/build_drivers/$DRIVER/$p-instrument.sh $p
            elif [[ $TARGET = *php* ]]; then
                $FUZZER/build_drivers/$DRIVER/$p-instrument.sh $p
            elif [[ $TARGET = *sqlite3* ]]; then
                $FUZZER/build_drivers/$DRIVER/$p-instrument.sh $p
            else
                echo "Could not support this target $TARGET"
                exit 1
            fi
         )
         done         
}


get_target
generate_cg_cfg
instrument
