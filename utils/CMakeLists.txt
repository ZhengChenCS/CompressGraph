add_executable(edgelist2csr edgelist2csr.cpp)
add_executable(adj2csr adj2csr.cpp)
add_executable(save_degree save_degree.cpp)
add_executable(gene_rule_order gene_rule_order.cpp)

if(LIGRA)
    add_executable(convert2ligra convert2ligra.cpp)
endif()

if(GUNROCK)
    add_executable(convert2gunrock convert2gunrock.cpp)
endif()