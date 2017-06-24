cmake_minimum_required(VERSION 3.6)
project(llvm_ir_tracer)

set(CMAKE_CXX_COMPILER "/usr/local/bin/clang++")
set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp)
add_executable(llvm_ir_tracer ${SOURCE_FILES})
