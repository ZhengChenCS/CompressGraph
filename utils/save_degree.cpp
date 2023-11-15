#include <core/io.hpp>
#include <cstdio>
#include <core/util.hpp>

int main(int argc, char **argv){
    std::string fvlist = argv[1];
    std::vector<VertexT> csr_vlist;
    VertexT vcnt = read_binary2vector(fvlist, csr_vlist);
    std::vector<VertexT> degree(vcnt-1);
    for(VertexT vid = 0; vid < vcnt-1; ++vid){
        degree[vid] = csr_vlist[vid+1] - csr_vlist[vid];
    }
    FILE* fdegree;
    if(!(fdegree = fopen("degree.bin", "w"))){
        fprintf(stderr, "Cannot open file degree.bin\n");
    }
    fwrite(&degree[0], sizeof(VertexT), vcnt-1, fdegree);
    return 0;
}