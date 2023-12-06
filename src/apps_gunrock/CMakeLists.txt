if(NOT CMAKE_CUDA_ARCHITECTURES)
  set(CMAKE_CUDA_ARCHITECTURES 75)
  message(STATUS "Using default GPU Architecture: ${CMAKE_CUDA_ARCHITECTURES}")
else()
  message(STATUS "GPU Architecture: ${CMAKE_CUDA_ARCHITECTURES}")
endif()

include(${PROJECT_DEPS_DIR}/gunrock/cmake/FetchThrustCUB.cmake)
include(${PROJECT_DEPS_DIR}/gunrock/cmake/FetchModernGPU.cmake)
include(${PROJECT_DEPS_DIR}/gunrock/cmake/FetchCXXOpts.cmake)
include(${PROJECT_DEPS_DIR}/gunrock/cmake/FetchNlohmannJson.cmake)


add_library(essentials INTERFACE)

set_target_properties(essentials 
  PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF # Should this be turned on for MSVC?
    CUDA_STANDARD 17
    CUDA_STANDARD_REQUIRED ON
    CUDA_EXTENSIONS OFF
    CUDA_RESOLVE_DEVICE_SYMBOLS ON
    CUDA_SEPARABLE_COMPILATION ON
    CUDA_ARCHITECTURES ${CMAKE_CUDA_ARCHITECTURES} # Set required architecture.
    # CUDA_PTX_COMPILATION ON # Can only be applied to OBJ.
)

target_compile_definitions(essentials
  INTERFACE 
    SM_TARGET=${CMAKE_CUDA_ARCHITECTURES}
)

target_compile_features(essentials INTERFACE cxx_std_17)

set(ESSENTIALS_INCLUDE_DIR ${PROJECT_DEPS_DIR}/gunrock/include)

target_include_directories(essentials
  INTERFACE ${ESSENTIALS_INCLUDE_DIR}
  INTERFACE ${CXXOPTS_INCLUDE_DIR}
  INTERFACE ${THRUST_INCLUDE_DIR}
  INTERFACE ${CUB_INCLUDE_DIR}
  INTERFACE ${MODERNGPU_INCLUDE_DIR}
  # INTERFACE ${RAPIDJSON_INCLUDE_DIR}
  INTERFACE ${NHLOMANN_JSON_INCLUDE_DIR}
  INTERFACE ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES}
)

target_link_libraries(essentials
  INTERFACE curand
  INTERFACE cuda
  INTERFACE cusparse
)

target_sources(essentials
  INTERFACE "${ESSENTIALS_INCLUDE_DIR}/gunrock/util/gitsha1make.c"
  INTERFACE "${ESSENTIALS_INCLUDE_DIR}/gunrock/io/detail/mmio.cpp"
)

set(CXX_FLAGS 
  $<$<CXX_COMPILER_ID:MSVC>:
    /W4
  >
  $<$<CXX_COMPILER_ID:GNU>:
    -Wall 
    # -Wextra
    # -Wno-unused-result
    # -Wno-unused-local-typedefs
    # -Wno-strict-aliasing
    # -Wno-unused-function
    # -Wno-format-security
    -Wno-unused-variable
    # -Werror
    # -vvv
  >
)

set(CUDA_FLAGS
  --expt-extended-lambda 
  --expt-relaxed-constexpr
  --use_fast_math
)

target_compile_options(essentials INTERFACE
    $<$<COMPILE_LANGUAGE:CXX>:${CXX_FLAGS}>
    $<$<COMPILE_LANGUAGE:CUDA>:${CUDA_FLAGS}>
)

add_executable(bfs_gpu bfs.cu)
add_executable(sssp_gpu sssp.cu)
add_executable(cc_gpu cc.cu)
add_executable(pagerank_gpu pagerank.cu)
add_executable(hits_gpu hits.cu)
add_executable(topo_gpu topo.cu)

target_link_libraries(bfs_gpu PRIVATE essentials)
target_link_libraries(sssp_gpu PRIVATE essentials)
target_link_libraries(cc_gpu PRIVATE essentials)
target_link_libraries(pagerank_gpu PRIVATE essentials)
target_link_libraries(hits_gpu PRIVATE essentials)
target_link_libraries(topo_gpu PRIVATE essentials)



get_target_property(ESSENTIALS_ARCHITECTURES essentials CUDA_ARCHITECTURES)
set_target_properties(bfs_gpu PROPERTIES CUDA_ARCHITECTURES ${ESSENTIALS_ARCHITECTURES})
set_target_properties(sssp_gpu PROPERTIES CUDA_ARCHITECTURES ${ESSENTIALS_ARCHITECTURES})
set_target_properties(cc_gpu PROPERTIES CUDA_ARCHITECTURES ${ESSENTIALS_ARCHITECTURES})
set_target_properties(pagerank_gpu PROPERTIES CUDA_ARCHITECTURES ${ESSENTIALS_ARCHITECTURES})
set_target_properties(hits_gpu PROPERTIES CUDA_ARCHITECTURES ${ESSENTIALS_ARCHITECTURES})
set_target_properties(topo_gpu PROPERTIES CUDA_ARCHITECTURES ${ESSENTIALS_ARCHITECTURES})



