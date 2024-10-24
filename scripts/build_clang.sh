mkdir $CLANG_BUILDDIR
cd $CLANG_BUILDDIR

export CC=clang
export CXX=clang++

cmake -G "Ninja" \
    -S $LLVM_SRC/clang \
    -B $CLANG_BUILDDIR \
    -DCMAKE_INSTALL_PREFIX=$LLVM_HOME \
    -DCMAKE_BUILD_TYPE=$LLVM_BUILD_TYPE \
    -DLLVM_TARGETS_TO_BUILD=X86 \
    -DLLVM_INCLUDE_TESTS=OFF \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build
cmake --build . #-j4

# Install to $LLVM_HOME
#cmake --build . --target install
