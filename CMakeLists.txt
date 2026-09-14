cmake_minimum_required(VERSION 3.9)
project(CompressGraph 
LANGUAGES C CXX
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
option(GUNROCK "compile GPU apps" OFF)

if(LIGRA)
    add_subdirectory(src/apps_ligra)
endif()

if(GUNROCK)
    enable_language(CUDA)
    add_subdirectory(src/apps_gunrock)
endif()





# Independent compression backends; no Ligra, Gunrock or Python dependency.
option(BATCH_COMPRESSION "Build parallel CPU batch compression" ON)
option(BATCH_CUDA "Build CUDA batch compression" OFF)
if(BATCH_CUDA AND NOT BATCH_COMPRESSION)
    message(FATAL_ERROR "BATCH_CUDA requires BATCH_COMPRESSION")
endif()
if(BATCH_COMPRESSION)
    add_library(compressgraph_batch core/batch/cpu.cpp core/batch/common.cpp)
    target_include_directories(compressgraph_batch PUBLIC ${PROJECT_SOURCE_DIR})
    target_link_libraries(compressgraph_batch PUBLIC OpenMP::OpenMP_CXX)
    add_executable(compress_batch_cpu src/compress_batch.cpp)
    target_link_libraries(compress_batch_cpu PRIVATE compressgraph_batch)
    if(BATCH_CUDA)
        if(CMAKE_VERSION VERSION_LESS 3.18)
            message(FATAL_ERROR "BATCH_CUDA requires CMake 3.18 or later")
        endif()
        enable_language(CUDA)
        add_library(compressgraph_batch_cuda core/batch/cuda.cu)
        target_compile_options(compressgraph_batch_cuda PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-O3>)
        set_target_properties(compressgraph_batch_cuda PROPERTIES CUDA_STANDARD 17 CUDA_STANDARD_REQUIRED ON)
        target_link_libraries(compressgraph_batch_cuda PUBLIC compressgraph_batch)
        add_executable(compress_batch_gpu src/compress_batch.cpp)
        target_compile_definitions(compress_batch_gpu PRIVATE COMPRESSGRAPH_BATCH_GPU_CLI)
        target_link_libraries(compress_batch_gpu PRIVATE compressgraph_batch_cuda)
    endif()
    include(CTest)
    if(BUILD_TESTING)
        add_executable(test_batch tests/test_batch.cpp)
        set_target_properties(test_batch PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tests)
        target_link_libraries(test_batch PRIVATE compressgraph_batch)
        if(BATCH_CUDA)
            target_compile_definitions(test_batch PRIVATE COMPRESSGRAPH_TEST_CUDA)
            target_link_libraries(test_batch PRIVATE compressgraph_batch_cuda)
        endif()
        add_test(NAME batch_exactness COMMAND test_batch)
        find_package(PythonInterp 3 QUIET)
        if(PYTHONINTERP_FOUND)
            add_test(NAME batch_cli COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tests/test_cli.py $<TARGET_FILE:compress_batch_cpu> $<TARGET_FILE:filter> $<TARGET_FILE:compress>)
            if(BATCH_CUDA)
                add_test(NAME batch_gpu_cli COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tests/test_cli.py $<TARGET_FILE:compress_batch_gpu> $<TARGET_FILE:filter>)
            endif()
        endif()
    endif()
endif()
