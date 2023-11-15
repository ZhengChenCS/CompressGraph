#include <core/io.hpp>
#include <core/util.hpp>
#include <cstdio>
#include <vector>
#ifndef INIT
#define INIT 1000000
#endif

int main(int argc, char **argv){
    if(argc != 4){
        fprintf(stderr, "Usage: gene_rule_order <csr_vlist> <csr_elist> <info>\n");
    }
    std::string fvlist = argv[1];
    std::string felist = argv[2];
    std::string finfo = argv[3];
    std::vector<VertexT> csr_vlist;
    std::vector<VertexT> csr_elist;
    std::vector<VertexT> info;
    long n = read_binary2vector(fvlist, csr_vlist);
    long ecnt = read_binary2vector(felist, csr_elist);
    long ele_cnt = read_binary2vector(finfo, info);
    VertexT vcnt = info[0];
    VertexT rcnt = info[1];
    n--;
    std::vector<int> in_degree(rcnt, 0);
    std::vector<int> order(rcnt, INIT);

    VertexT vid, eid, src, dst, start, end;

    for(eid = 0; eid < csr_elist.size(); eid++){
        dst = csr_elist[eid];
        if(dst >= vcnt) { in_degree[dst-vcnt]++; }
    }

    std::vector<VertexT> in_worklist;
    std::vector<VertexT> out_worklist;

    for(vid = 0; vid < vcnt; vid++){
        in_worklist.emplace_back(vid);
    }
    int step = 0;
    int count = 0;
    while(!in_worklist.size() == 0) {
        VertexT w_size = in_worklist.size();
        for(vid = 0; vid < w_size; vid++){
            src = in_worklist[vid];
            start = csr_vlist[src];
            end = csr_vlist[src+1];
            for(eid = start; eid < end; eid++){
                dst = csr_elist[eid];
                if(dst >= vcnt && order[dst-vcnt] == INIT){
                    in_degree[dst-vcnt]--;
                    if(in_degree[dst-vcnt] == 0){
                        order[dst-vcnt] = step;
                        out_worklist.emplace_back(dst);
                        count++;
                    }
                }
            }
        }
        step++;
        std::swap(in_worklist, out_worklist);
        out_worklist.clear();
    }
    FILE* forder;
    if(!(forder = fopen("order.bin", "w"))){
        fprintf(stderr, "Cannot open file order.bin\n");
    }
    fwrite(&order[0], sizeof(VertexT), rcnt, forder);
    return 0;
}