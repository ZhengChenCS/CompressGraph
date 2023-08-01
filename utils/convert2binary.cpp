#include <ostream>
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <cstdint>


int main(int argc, char **argv){
    std::string filename = argv[1];
    std::ifstream infile(filename);
    uint vertex_cnt, edge_cnt;
    infile >> vertex_cnt >> edge_cnt;
    std::vector<int> vlist;
    std::vector<int> elist;
    int dst;
    vlist.emplace_back(0);
    uint cur_pos = 0;
    while(infile >> dst){
        if(dst >= vertex_cnt){
            vlist.emplace_back(cur_pos);
        }else{
            elist.emplace_back(dst);
            cur_pos++;
        }
    }
    vlist.emplace_back(cur_pos);
    FILE *fvlist;
    FILE *felist;
    fvlist = fopen("csr_vlist.bin", "w");
    felist = fopen("csr_elist.bin", "w");
    fwrite(&vlist[0], sizeof(int), vlist.size(), fvlist);
    fwrite(&elist[0], sizeof(int), elist.size(), felist);
    return 0;
}