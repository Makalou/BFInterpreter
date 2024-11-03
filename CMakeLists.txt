cmake_minimum_required(VERSION 3.10)

# Project name
project(BFInterpreter)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)

# Set LLVM dependency
add_subdirectory(llvm-project/llvm)
message("LLVM build directory: ${CMAKE_CURRENT_BINARY_DIR}")
set(LLVM_DIR "${CMAKE_CURRENT_BINARY_DIR}/llvm-project/llvm/lib/cmake/llvm" CACHE PATH "Path to LLVMConfig.cmake")
message("LLVM directory: ${LLVM_DIR}")
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# Add executable
add_executable(bfi main.cpp)
add_executable(bfc compiler/main.cpp)

# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_libs support core irreader)

# Link against LLVM libraries
target_link_libraries(bfc ${llvm_libs})