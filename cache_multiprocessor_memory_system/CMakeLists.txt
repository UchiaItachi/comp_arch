cmake_minimum_required(VERSION 3.8)
project(cache_multiprocessor_memory_system)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES core.cpp core.h dram.cpp dram.h memsys.cpp memsys.h sim.cpp dram.cpp dram.h types.h cache.cpp cache.h)
add_executable(cache_multiprocessor_memory_system ${SOURCE_FILES})