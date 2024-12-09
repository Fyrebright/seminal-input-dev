#!/usr/bin/env bash
# Given a C file, this script will compile it to LLVM IR with debug information, inline it, and run print<input-vals> on it.
# Usage: ./inline_run.sh <c-file>

# Path to the plugin
PLUGIN_PATH=build/lib/libFindKeyPoints.so
# PLUGIN_PATH=build/lib/libFindIOVals.so
# PLUGIN_PATH=build/lib/libFindInputValues.so

USE_INLINE=0
OPTLEVEL=0

PASSES='function(mem2reg),function(print<find-key-pts>)'
# PASSES='function(mem2reg),function(print<find-io-val>)'
# PASSES='function(print<find-io-val>)'
# PASSES='function(mem2reg),print<input-vals>'

OPTS='-disable-output'
DEBUG_PM=0
DEBUG_PASS=0

# Get file path from input
file=$1
basename=$(basename "$file")
mkdir -p out

# Check if the input file exists
if [ ! -f "$file" ]; then
    echo "File not found: $file"
    exit 1
fi

# Build the project
build_output=$(cmake --build build/)
if [ $? -ne 0 ];
then
    printf "Error building the project:\n %s\n" "$build_output"
    exit 1
fi

# Check if the plugin exists
if [ ! -f $PLUGIN_PATH ]; then
    echo "Plugin not found at $PLUGIN_PATH"
    exit 1
fi

# Add PM debug flag
if [ $DEBUG_PM -eq 1 ]; then
    OPTS="$OPTS -debug-pass-manager"
fi

# Add pass debug flag
if [ $DEBUG_PASS -eq 1 ]; then
    OPTS="$OPTS -debug"
fi

# Compile the C file to LLVM IR with debug information and assign it to a variable
IR=$(clang -g -S -emit-llvm -O"$OPTLEVEL" -fno-discard-value-names -Xclang -disable-O0-optnone -o - "$file")
# echo "$IR"

if [ $USE_INLINE -eq 0 ]; then
    # Write the IR to a file
    echo "$IR" >out/"$basename".ll

    opt \
        -load-pass-plugin=$PLUGIN_PATH \
        -passes=$PASSES \
        $OPTS \
        out/"$basename".ll
else
    # Remove noinline attribute from functions in IR
    IR_sed=$(sed 's/noinline//g' <<<"$IR")
    # Write the IR to a file
    echo "$IR_sed" >out/"$basename".ll

    # Inline the LLVM IR and assign it to a variable
    INLINE=$(opt -S -passes="inline" <(echo "$IR_sed"))

    # Write the inlined IR to a file
    echo "$INLINE" >out/"$basename"-inlined.ll

    # echo command
    echo "opt -load-pass-plugin=$PLUGIN_PATH -passes=$PASSES $OPTS out/$basename-inlined.ll"

    # Run print<input-vals> on the inlined IR
    opt \
        -load-pass-plugin=$PLUGIN_PATH \
        -passes=$PASSES \
        $OPTS \
        out/"$basename"-inlined.ll
fi
