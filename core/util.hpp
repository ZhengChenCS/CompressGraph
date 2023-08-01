#pragma once
#include <assert.h>
#include <omp.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <vector>
#define VertexT int
#define INIT 100000
#define INF 1e-6
#define OMP_THRESHOLD 1024

/*
 *  convert csr to double vector
 */

double timestamp() {
    timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec + 1e-6 * t.tv_usec;
}

/*
 *  convert csr to double vector out-edge
 */
void csr_convert(std::vector<std::vector<VertexT>> &graph,
                 std::vector<VertexT> &vlist, std::vector<VertexT> &elist,
                 VertexT vertex_cnt) {
    // #pragma omp parallel for schedule(dynamic)
    for (VertexT v = 0; v < vertex_cnt; v++) {
        VertexT start = vlist[v];
        VertexT end = vlist[v + 1];
        for (VertexT e = start; e < end; e++) {
            graph[v].push_back(elist[e]);
        }
    }
}

/*
 * convert csr to double vector in-edge
 */

void csr_convert_idx(std::vector<std::vector<VertexT>> &graph,
                     std::vector<VertexT> &vlist, std::vector<VertexT> &elist,
                     VertexT vertex_cnt) {
    // #pragma omp parallel for schedule(dynamic)
    for (VertexT v = 0; v < vertex_cnt; v++) {
        VertexT start = vlist[v];
        VertexT end = vlist[v + 1];
        for (VertexT e = start; e < end; e++) {
            if (elist[e] > vertex_cnt)
                std::cout << elist[e] << std::endl;
            graph[elist[e]].push_back(v);
        }
    }
}

void insert_spliter(std::vector<int> &vlist, std::vector<int> &elist,
                    std::vector<int> &re, int max_symbol) {
    int v, e;
    int start, end;
    if (vlist.size() == 0)
        return;
    int cur = max_symbol;
    for (v = 0; v < vlist.size() - 1; v++) {
        start = vlist[v];
        end = vlist[v + 1];
        for (e = start; e < end; e++) {
            re.push_back(elist[e]);
        }
        re.push_back(cur++);
    }
}

void genNewIdForRule(std::vector<VertexT> &newRuleId, VertexT &newRule_cnt,
                     std::vector<bool> &merge_flag, VertexT vertex_cnt,
                     VertexT rule_cnt) {
    VertexT cur = vertex_cnt;
    for (VertexT i = 0; i < rule_cnt; i++) {
        newRuleId[i] = cur;
        if (merge_flag[i] == false) {
            cur++;
        }
    }
    newRule_cnt = cur - vertex_cnt; // now rule count
}

void genNewGraphCSR(std::vector<VertexT> &vlist, std::vector<VertexT> &elist,
                    std::vector<std::vector<VertexT>> &graph,
                    std::vector<VertexT> &newRuleId,
                    std::vector<bool> &merge_flag, VertexT vertex_cnt) {
    VertexT g_size = graph.size();
    vlist.push_back(0);
    VertexT e_size = 0;
    for (VertexT v = 0; v < g_size; ++v) {
        VertexT srcID = v;
        VertexT e_size = graph[srcID].size();
        if (srcID >= vertex_cnt && merge_flag[srcID - vertex_cnt] == true)
            continue;
        for (VertexT e = 0; e < e_size; e++) {
            VertexT dstID = graph[srcID][e];
            VertexT n_dstID =
                dstID >= vertex_cnt ? newRuleId[dstID - vertex_cnt] : dstID;
            elist.push_back(n_dstID);
        }
        e_size = elist.size();
        vlist.push_back(e_size);
    }
}
