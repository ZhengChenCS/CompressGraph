#include <iostream>
#include <cstdio>
#include <cstdint>
#include <core/util.hpp>
#include <vector>

int main()
{
    VertexT v_cnt, e_cnt;
    std::string format;
    std::cin >> format;
    std::cin >> v_cnt;
    std::cin >> e_cnt; 
    std::vector<VertexT> csr_vlist;
    std::vector<VertexT> csr_elist;
    VertexT offset;
    for(VertexT vid = 0; vid < v_cnt; ++vid){
        std::cin >> offset;
        csr_vlist.emplace_back(offset);
    }   
    csr_vlist.emplace_back(e_cnt);
    VertexT vid;
    for(VertexT eid = 0; eid < e_cnt; ++eid){
        std::cin >> vid;
        csr_elist.emplace_back(vid); 
    }
    FILE* fvlist;
    FILE* felist;
    if(!(fvlist = fopen("csr_vlist.bin", "w"))){
        fprintf(stderr, "Cannot open file csr_vlist.bin\n");
    }
    if(!(felist = fopen("csr_elist.bin", "w"))){
        fprintf(stderr, "Cannot open file csr_vlist.bin\n");
    }
    fwrite(&csr_vlist[0], sizeof(VertexT), csr_vlist.size(), fvlist);
    fwrite(&csr_elist[0], sizeof(VertexT), csr_elist.size(), felist);
    return 0;
}