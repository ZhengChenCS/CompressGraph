#include <omp.h>
#include <string.h>
#include <unistd.h>
#include <cstdlib>

#include <algorithm>
#include <iostream>
#include <tuple>
#include <vector>

#include <core/io.hpp>
#include <core/util.hpp>

typedef struct rule_info { // rule infomation
    int len;
    int freq;
    rule_info() {
        len = 0;
        freq = 0;
    }
} rule_info;

int freq_threshold = 4;
int len_threshold = 4;

inline bool judgeRule(int len, int freq) {
    return (len < len_threshold) && (freq < freq_threshold);
}

void mergeRule(VertexT rule_id, std::vector<std::vector<VertexT>> &graph,
               std::vector<std::vector<VertexT>> &graphT,
               std::vector<rule_info> &rif, std::vector<bool> &merge_flag,
               VertexT vertex_cnt, VertexT rule_cnt) {
    VertexT ID = rule_id + vertex_cnt;
    // insert childnode to parent node
    for (VertexT i = 0; i < graphT[ID].size(); ++i) {
        VertexT node = graphT[ID][i];
        std::vector<VertexT>::iterator it =
            find(graph[node].begin(), graph[node].end(), ID);
        std::vector<VertexT>::iterator pos = graph[node].erase(it);
        graph[node].insert(pos, graph[ID].begin(), graph[ID].end());
        // update len of parent node if parent node is rule
        if (node >= vertex_cnt) {
            rif[node - vertex_cnt].len += (graph[ID].size() - 1);
            merge_flag[node - vertex_cnt] = judgeRule(
                rif[node - vertex_cnt].len, rif[node - vertex_cnt].freq);
        }
    }
    // update child node idx
    for (VertexT i = 0; i < graph[ID].size(); ++i) {
        VertexT child_id = graph[ID][i];
        std::vector<VertexT>::iterator it =
            find(graphT[child_id].begin(), graphT[child_id].end(), ID);
        // delete origin idx in ruleID
        std::vector<VertexT>::iterator pos = graphT[child_id].erase(it);
        // insert new idx to child id
        graphT[child_id].insert(pos, graphT[ID].begin(), graphT[ID].end());
        // update freq of child node if the child node is rule
        if (child_id >= vertex_cnt) {
            rif[child_id - vertex_cnt].freq = graphT[child_id].size();
            merge_flag[child_id - vertex_cnt] =
                judgeRule(rif[child_id - vertex_cnt].len,
                          rif[child_id - vertex_cnt].freq);
        }
    }
}

void initInfoForRule(std::vector<std::vector<VertexT>> &graph,
                     std::vector<std::vector<VertexT>> &graphT,
                     std::vector<rule_info> &rif, std::vector<bool> &merge_flag,
                     VertexT vertex_cnt,
                     VertexT rule_cnt) { // init len&freq for each rule
    for (VertexT i = 0; i < rule_cnt; ++i) {
        rif[i].freq = graphT[i + vertex_cnt].size();
        rif[i].len = graph[i + vertex_cnt].size();
        merge_flag[i] = judgeRule(rif[i].len, rif[i].freq);
    }
}

int main(int argc, char **argv) {
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <vlist> <elsit> <info> <freq_threadshold> <len_threshold>\n", argv[0]);
        return 0;
    }
    std::string pvlist = argv[1];
    std::string pelist = argv[2];
    std::string pinfo = argv[3];
    freq_threshold = atoi(argv[4]);
    len_threshold = atoi(argv[5]);
    std::vector<VertexT> vlist;
    std::vector<VertexT> elist;
    std::vector<VertexT> info;
    int v_cnt = read_binary2vector(pvlist, vlist);
    int e_cnt = read_binary2vector(pelist, elist);
    int n = read_binary2vector(pinfo, info);
    int vertex_cnt = info[0];
    int rule_cnt = info[1];
    fprintf(stderr, "filter start...\n");
    double start = timestamp();
    std::vector<std::vector<VertexT>> graph(vertex_cnt + rule_cnt);
    csr_convert(graph, vlist, elist, vertex_cnt + rule_cnt);
    std::vector<std::vector<VertexT>> graphT(vertex_cnt + rule_cnt);
    csr_convert_idx(graphT, vlist, elist, vertex_cnt + rule_cnt);
    std::vector<rule_info> rif;
    rif.resize(rule_cnt);
    std::vector<bool> merge_flag(rule_cnt);
    initInfoForRule(graph, graphT, rif, merge_flag, vertex_cnt, rule_cnt);
    for (VertexT i = 0; i < rule_cnt; i++) {
        if (merge_flag[i] == true) {
            mergeRule(i, graph, graphT, rif, merge_flag, vertex_cnt, rule_cnt);
        }
    }
    // gen new ID for rule
    VertexT newRule_cnt;
    std::vector<VertexT> newRuleId(rule_cnt);
    genNewIdForRule(newRuleId, newRule_cnt, merge_flag, vertex_cnt, rule_cnt);
    std::vector<VertexT> new_vlist;
    std::vector<VertexT> new_elist;
    genNewGraphCSR(new_vlist, new_elist, graph, newRuleId, merge_flag,
                   vertex_cnt);
    double end = timestamp();
    fprintf(stderr, "Filter time : %.4f(s)\n", end - start);
    FILE *fvlist;
    FILE *felist;
    FILE *finfo;
    fvlist = fopen("csr_vlist.bin", "w");
    felist = fopen("csr_elist.bin", "w");
    fwrite(&new_vlist[0], sizeof(int), new_vlist.size(), fvlist);
    fwrite(&new_elist[0], sizeof(int), new_elist.size(), felist);
    finfo = fopen("info.bin", "w");
    fwrite(&vertex_cnt, sizeof(int), 1, finfo);
    fwrite(&newRule_cnt, sizeof(int), 1, finfo);
    return 0;
}
