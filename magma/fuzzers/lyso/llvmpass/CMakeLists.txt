cmake_minimum_required(VERSION 3.4)
project(GraphPass)

set(CMAKE_BUILD_TYPE Debug)
# support C++14 features used by LLVM 10.0.0
set(CMAKE_CXX_STANDARD 14) 

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

message(STATUS "LLVM definitions ${LLVM_DEFINITIONS}")
message(STATUS "LLVM Include Dir: ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM LIBRARY Dir: ${LLVM_LIBRARY_DIRS}")

find_package(LLVM REQUIRED CONFIG)
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

add_subdirectory(GraphPass)  # Use your pass name here.
