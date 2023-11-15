set(LIGRA_FLAG "-DOPENMP")

add_executable(bfs_cpu bfs.cpp)
add_executable(cc_cpu cc.cpp)
add_executable(sssp_cpu sssp.cpp)
add_executable(pagerank_cpu pagerank.cpp)
add_executable(topo_cpu topo.cpp)
add_executable(hits_cpu hits.cpp)

target_compile_definitions(bfs_cpu PRIVATE ${LIGRA_FLAG})
target_compile_definitions(cc_cpu PRIVATE ${LIGRA_FLAG})
target_compile_definitions(sssp_cpu PRIVATE ${LIGRA_FLAG})
target_compile_definitions(pagerank_cpu PRIVATE ${LIGRA_FLAG})
target_compile_definitions(topo_cpu PRIVATE ${LIGRA_FLAG})
target_compile_definitions(hits_cpu PRIVATE ${LIGRA_FLAG})



