#include <cstdio>
#include <cstdint>
#include <core/util.hpp>
#include <core/io.hpp>
#include <fstream>

int main(int argc, char **argv){
    if(argc != 3){
        fprintf(stderr, "Usage: %s <csr_vlist.bin> <csr_elist.bin>\n", argv[0]);
        return 0;
    }
    std::string fvlist = argv[1];
    std::string felist = argv[2];
    std::vector<VertexT> csr_vlist;
    std::vector<VertexT> csr_elist;

    VertexT v_cnt = read_binary2vector(fvlist, csr_vlist);
    VertexT e_cnt = read_binary2vector(felist, csr_elist);
    std::cout << "%%MatrixMarket matrix coordinate pattern general" << std::endl;
    v_cnt -= 1;
    std::cout << v_cnt << " " << v_cnt << " " << e_cnt << std::endl;
    VertexT vid, eid, start, end, src, dst;
    for(vid = 0; vid < v_cnt; ++vid){
        src = vid;
        start = csr_vlist[src];
        end = csr_vlist[src+1];
        for(eid = start; eid < end; eid++){
            dst = csr_elist[eid];
            std::cout << src+1 << " " << dst+1 << std::endl;
        }
    }
    return 0;
}