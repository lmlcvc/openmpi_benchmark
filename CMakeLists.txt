cmake_minimum_required(VERSION 3.0)
project(openmpi_benchmark)

set(CMAKE_CXX_STANDARD 17)

add_executable(openmpi_benchmark benchmark.cpp)
