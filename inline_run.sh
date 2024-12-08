# /usr/bin/bash
# Given a C file, this script will compile it to LLVM IR with debug information, inline it, and run print<input-vals> on it.
# Usage: ./inline_run.sh <c-file>

cmake --build build/
if [ $? -ne 0 ]; then
    echo "Error building the project"
    exit 1
fi

# Get file path from input
file=$1
basename=$(basename $file)
mkdir -p out

# Compile the C file to LLVM IR with debug information and assign it to a variable
IR=$(clang -g -S -emit-llvm -O0 -fno-discard-value-names -Xclang -disable-O0-optnone -o - $file)
# echo "$IR"

# Remove noinline attribute from functions in IR
IR_sed=$(sed 's/noinline//g' <<< "$IR")
# Write the IR to a file
echo "$IR_sed" > out/$basename.ll

# Inline the LLVM IR and assign it to a variable
INLINE=$(opt -S -passes="inline" <(echo "$IR_sed"))

# Write the inlined IR to a file
echo "$INLINE" > out/$basename-inlined.ll

# Run print<input-vals> on the inlined IR
opt \
    -load-pass-plugin=build/key-pts/FindInputVals.so \
    -passes='function(mem2reg),print<input-vals>' \
    -disable-output \
    out/$basename-inlined.ll  

    # out/$basename.ll 
    # <(echo "$INLINE")
    # -debug-pass-manager
    # -passes='function(mem2reg),print<input-vals>' \