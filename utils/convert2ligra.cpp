#include <cstdio>
#include <cstdint>
#include <core/util.hpp>
#include <core/io.hpp>
#include <fstream>

int main(int argc, char **argv){
    if(argc != 3){
        fprintf(stderr, "Usage: convert2ligra <csr_vlist.bin> <csr_elist.bin>\n");
        return 0;
    }
    std::string fvlist = argv[1];
    std::string felist = argv[2];
    std::vector<VertexT> csr_vlist;
    std::vector<VertexT> csr_elist;

    VertexT v_cnt = read_binary2vector(fvlist, csr_vlist);
    VertexT e_cnt = read_binary2vector(felist, csr_elist);

    v_cnt -= 1;

    std::cout << "AdjacencyGraph" << std::endl;
    std::cout << v_cnt << std::endl;
    std::cout << e_cnt << std::endl;

    for(VertexT vid = 0; vid < v_cnt; ++vid){
        std::cout << csr_vlist[vid] << std::endl;
    }

    for(VertexT eid = 0; eid < e_cnt; ++eid){
        std::cout << csr_elist[eid] << std::endl;
    }
    return 0;
}