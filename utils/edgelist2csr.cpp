#include <cstdio>
#include <cstdint>
#include <iostream>
#include <vector>
#include <core/util.hpp>
#include <algorithm>

int main(int argc, char **argv){
    VertexT src, dst;
    std::vector<std::pair<VertexT, VertexT>> edgelist;
    std::vector<std::vector<VertexT>> graph;
    VertexT v_cnt = 0;
    while((std::cin >> src >> dst))
    {
        v_cnt = std::max(v_cnt, src);
        v_cnt = std::max(v_cnt, dst);
        edgelist.push_back(std::make_pair(src, dst));
    }
    graph.resize(v_cnt+1);
    for(uint64_t i = 0; i < edgelist.size(); i++){
        src = edgelist[i].first;
        dst = edgelist[i].second;
        graph[src].emplace_back(dst);
    }
    for(VertexT vid = 0; vid < graph.size(); vid++){
        std::sort(graph[vid].begin(), graph[vid].end());
    }
    std::vector<VertexT> csr_vlist;
    std::vector<VertexT> csr_elist;
    csr_vlist.emplace_back(0);
    uint64_t offset = 0;
    for(VertexT vid = 0; vid < graph.size(); vid++){
        for(int eid = 0; eid < graph[vid].size(); eid++){
            csr_elist.emplace_back(graph[vid][eid]);
        }
        offset += graph[vid].size();
        csr_vlist.emplace_back(offset);
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
                                        