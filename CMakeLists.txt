cmake_minimum_required(VERSION 3.6)
project(CSC412CourseProj)

# LLVM uses C++17.
set(CMAKE_CXX_STANDARD 17)

# Load LLVMConfig.cmake. If this fails, consider setting `LLVM_DIR` to point
# to your LLVM installation's `lib/cmake/llvm` directory.
find_package(LLVM REQUIRED CONFIG)

# Include the part of LLVM's CMake libraries that defines
# `add_llvm_pass_plugin`.
include(AddLLVM)

# Check the LLVM config
include(CMakePrintHelpers)

message(STATUS "Values read from LLVM config:")
list(APPEND CMAKE_MESSAGE_INDENT "\t")
cmake_print_variables(LLVM_INCLUDE_DIRS)
cmake_print_variables(LLVM_DEFINITIONS)
cmake_print_variables(LLVM_LIBRARY_DIRS)
list(POP_BACK CMAKE_MESSAGE_INDENT)

# Use LLVM's preprocessor definitions, include directories, and library search
# paths.
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

# Our pass lives in this subdirectory.
add_subdirectory(skeleton)
add_subdirectory(branch-ptr-trace)
