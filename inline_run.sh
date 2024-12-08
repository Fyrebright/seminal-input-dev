#!/usr/bin/env bash
# Given a C file, this script will compile it to LLVM IR with debug information, inline it, and run print<input-vals> on it.
# Usage: ./inline_run.sh <c-file>

# Path to the plugin
PLUGIN_PATH=build/lib/libFindIOVals.so
USE_INLINE=0
PASSES='function(mem2reg),function(print<find-io-val>)'
OPTS='-disable-output'
DEBUG_PM=1

# Check if the plugin exists
if [ ! -f $PLUGIN_PATH ]; then
    echo "Plugin not found at $PLUGIN_PATH"
    exit 1
fi

# Check if the input file exists
if [ ! -f "$1" ]; then
    echo "File not found: $1"
    exit 1
fi

# Build the project
build_output=$(cmake --build build/)
if [ $? -ne 0 ];
then
    printf "Error building the project:\n %s\n" "$build_output"
    exit 1
fi

# Add PM debug flag
if [ $DEBUG_PM -eq 1 ]; then
    OPTS="$OPTS -debug-pass-manager"
fi

# Get file path from input
file=$1
basename=$(basename "$file")
mkdir -p out

# Compile the C file to LLVM IR with debug information and assign it to a variable
IR=$(clang -g -S -emit-llvm -O0 -fno-discard-value-names -Xclang -disable-O0-optnone -o - "$file")
# echo "$IR"

if [ $USE_INLINE -eq 0 ]; then
    # Write the IR to a file
    echo "$IR" >out/"$basename".ll

    opt \
        -load-pass-plugin=$PLUGIN_PATH \
        -passes=$PASSES \
        -disable-output \
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

    # Run print<input-vals> on the inlined IR
    opt \
        -load-pass-plugin=$PLUGIN_PATH \
        -passes=$PASSES \
        -disable-output \
        out/"$basename"-inlined.ll
fi
