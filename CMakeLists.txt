# Combination of llvm-tutor and llvm-pass-skeleton

cmake_minimum_required(VERSION 3.6)
project(CSC412CourseProj)

# ==============================================================================
# LLVM Configuration
# ==============================================================================

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
# include_directories(SYSTEM ${LLVM_INCLUDE_DIRS}) //treat as system headers
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

#===============================================================================
# BUILD CONFIGURATION
#===============================================================================
# Use the same C++ standard as LLVM does
set(CMAKE_CXX_STANDARD 17 CACHE STRING "")

# Build type
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE
      STRING "Build type (default Debug):" FORCE)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall\
  -fdiagnostics-color=always")

# LLVM is normally built without RTTI. Be consistent with that.
if(NOT LLVM_ENABLE_RTTI)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()

# -fvisibility-inlines-hidden is set when building LLVM and on Darwin warnings
# are triggered if llvm-tutor is built without this flag (though otherwise it
# builds fine). For consistency, add it here too.
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-fvisibility-inlines-hidden" SUPPORTS_FVISIBILITY_INLINES_HIDDEN_FLAG)
if (${SUPPORTS_FVISIBILITY_INLINES_HIDDEN_FLAG} EQUAL "1")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility-inlines-hidden")
endif()

# Set the build directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")

#===============================================================================
# ADD SUB-TARGETS
# Doing this at the end so that all definitions and link/include paths are
# available for the sub-projects.
#===============================================================================
add_subdirectory(lib)
# add_subdirectory(tools)
# add_subdirectory(test)
# add_subdirectory(HelloWorld)