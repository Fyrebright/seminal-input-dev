# /usr/bin/bash
# Given a C file, this script will compile it to LLVM IR with debug information, inline it, and run print<input-vals> on it.
# Usage: ./inline_run.sh <c-file>

# Get file path from input
file=$1

# Compile the C file to LLVM IR with debug information and assign it to a variable
IR=$(clang -g -S -emit-llvm -O0 -fno-discard-value-names -Xclang -disable-O0-optnone -o - $file)
# echo "$IR"

# Remove noinline attribute from functions in IR
IR_sed=$(sed 's/noinline//g' <<< "$IR")
# echo "$IR_sed"

# Inline the LLVM IR and assign it to a variable
INLINE=$(opt -S -passes="inline" <(echo "$IR"))

# echo "$INLINE"

# Run print<input-vals> on the inlined IR
opt -load-pass-plugin=build/key-pts/FindInputVals.so -passes='function(reg2mem),print<input-vals>' <(echo "$INLINE") -debug-pass-manager -disable-output