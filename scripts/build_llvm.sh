mkdir $LLVM_BUILDDIR
cd $LLVM_BUILDDIR

# Configure
cmake -G "Ninja" \
    -S $LLVM_SRC/llvm \
    -B $LLVM_BUILDDIR \
    -DCMAKE_INSTALL_PREFIX=$LLVM_HOME \
    -DCMAKE_BUILD_TYPE=$LLVM_BUILD_TYPE \
    -DLLVM_TARGETS_TO_BUILD=X86 \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build
cmake --build .

# Install to $LLVM_HOME
cmake --build . --target install