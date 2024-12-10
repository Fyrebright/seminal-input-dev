#!/usr/bin/env bash
# Given a C file, this script will compile it to LLVM IR with debug information, inline it, and run print<input-vals> on it.
# Usage: ./inline_run.sh <c-file>

if [ $# -lt 1 ]; then
    echo "Usage: ./inline_run.sh <c-file> <mode>"
    exit 1
fi

# ------------------------------------------
# Configuration
# ------------------------------------------

# Modes: 0-4 (0: print<input-vals>, 1: print<find-key-pts>, 2: print<find-io-val>, 3: print<seminal-input>, 4: graph<seminal-input>)

# Set mode 3 as default
if [ $# -lt 2 ]; then
    MODE=3
else
    MODE=$2
fi

BEFORE_PASS='function(mem2reg)'
# BEFORE_PASS='function(reg2mem)'
# BEFORE_PASS='sink'

USE_INLINE=0
OPTLEVEL=0

OPTS='-disable-output'
DEBUG_PM=0
DEBUG_PASS=0

# ------------------------------------------
# Process settings
# ------------------------------------------

# Set the plugin path based on the mode
case $MODE in
0)
    PLUGIN_PATH=build/lib/libFindInputValues.so
    PASSES='function(print<input-vals>)'
    ;;
1)
    PLUGIN_PATH=build/lib/libFindKeyPoints.so
    PASSES='function(print<find-key-pts>)'
    ;;
2)
    PLUGIN_PATH=build/lib/libFindIOVals.so
    PASSES='function(print<find-io-val>)'
    ;;
3)
    PLUGIN_PATH=build/lib/libSeminalInput.so
    PASSES='function(print<seminal-input>)'
    ;;
4)
    PLUGIN_PATH=build/lib/libSeminalInput.so
    PASSES='function(graph<seminal-input>)'
    ;;
*)
    echo "Invalid mode: $MODE"
    exit 1
    ;;
esac

# Add the before pass to the passes
if [ -n "$BEFORE_PASS" ]; then
    PASSES="$BEFORE_PASS,$PASSES"
fi

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

# ------------------------------------------
# Main script
# ------------------------------------------

# Compile the C file to LLVM IR with debug information and assign it to a variable
IR=$(clang -g -S -emit-llvm -O"$OPTLEVEL" -fno-discard-value-names -Xclang -disable-O0-optnone -o - "$file")
# echo "$IR"

# #################### TODO: not sure if this is preferable
# Run the before pass
# if [ -n "$BEFORE_PASS" ]; then
#     IR2=$(opt -S -passes="$BEFORE_PASS" <(echo "$IR"))

#     diff -s <(echo "$IR") <(echo "$IR2")
#     IR="$IR2"
# fi


if [ $USE_INLINE -eq 0 ]; then
    # Write the IR to a file
    echo "$IR" >out/"$basename".ll

    # echo command
    # echo "opt -load-pass-plugin=$PLUGIN_PATH -passes=$PASSES $OPTS out/$basename.ll"

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
    # echo "opt -load-pass-plugin=$PLUGIN_PATH -passes=$PASSES $OPTS out/$basename-inlined.ll"

    # Run print<input-vals> on the inlined IR
    opt \
        -load-pass-plugin=$PLUGIN_PATH \
        -passes=$PASSES \
        $OPTS \
        out/"$basename"-inlined.ll
fi
