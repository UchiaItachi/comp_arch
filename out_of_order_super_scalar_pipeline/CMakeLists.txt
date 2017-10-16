cmake_minimum_required(VERSION 3.8)
project(out_of_order_super_scalar_pipeline)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES rat.cpp rest.cpp rob.cpp sim.cpp pipeline.cpp exeq.cpp trace.h utils.h)
add_executable(out_of_order_super_scalar_pipeline ${SOURCE_FILES})