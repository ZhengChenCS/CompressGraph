#pragma once

#include "filesystem.hpp"
#include <cstdint>
#include <vector>

int read_binary2vector(std::string filename, std::vector<int> &out) {
    if (!file_exists(filename)) {
        fprintf(stderr, "file:%s not exist.\n", filename.c_str());
        exit(0);
    }
    long fs = file_size(filename);
    long ele_cnt = fs / sizeof(int);
    out.resize(ele_cnt);
    FILE *fp;
    fp = fopen(filename.c_str(), "r");
    if (fread(&out[0], sizeof(int), ele_cnt, fp) < ele_cnt) {
        fprintf(stderr, "Read failed.\n");
    }
    return ele_cnt;
}
