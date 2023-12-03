cmake_minimum_required(VERSION 3.1)
project(CompressGraph 
LANGUAGES C CXX CUDA
)


set(CMAKE_CXX_STANDARD 17)

find_package(OpenMP REQUIRED)
if(OPENMP_FOUND)
message("OPENMP FOUND")
set(CMAKE_CXX_FLAGS "-fopenmp -LLIBDIR -Ofast")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

set(PROJECT_DEPS_DIR ${PROJECT_SOURCE_DIR}/deps)
include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_DEPS_DIR})

add_executable(compress src/compress.cpp core/compression/array.cc core/compression/basics.cc core/compression/hash.cc core/compression/heap.cc core/compression/records.cc)
add_executable(filter src/filter.cpp)
add_executable(filter_decomp src/filter_decomp.cpp)

add_subdirectory(utils)

option(LIGRA "compile CPU apps" OFF)

if(LIGRA)
    add_subdirectory(src/apps_ligra)
endif()




