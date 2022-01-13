cmake_minimum_required(VERSION 3.0.0)

project(part1 VERSION 0.1.0)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)

set(MTM_FLAGS_DEBUG "")
set(MTM_FLAGS_RELEASE "${MTM_FLAGS_DEBUG} -DNDEBUG")
set(CMAKE_C_FLAGS "-std=c99 ${MTM_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS "-std=c++11 ${MTM_FLAGS_DEBUG}")

#add_subdirectory(partA)

add_executable(HW3 main.c core_api.cpp sim_api.c)