#include "core/batch/compress.hpp"
#include <iostream>
#include <random>
#include <stdexcept>
using namespace compressgraph;
static Csr rows(const std::vector<std::vector<int32_t>> &input) {
  Csr graph{{0}, {}};
  for (const auto &row : input) {
    graph.col.insert(graph.col.end(), row.begin(), row.end());
    graph.rowptr.push_back(graph.col.size());
  }
  return graph;
}
static void equal(const Result &a, const Result &b) {
  if (a.vertices != b.vertices || a.rules != b.rules ||
      a.graph.rowptr != b.graph.rowptr || a.graph.col != b.graph.col)
    throw std::runtime_error("Backend/thread output mismatch");
}
int main() {
  try {
    std::vector<Csr> cases{rows({}), rows({{}}),
                           rows({std::vector<int32_t>(200, 0)})};
    std::vector<std::vector<int32_t>> repeated(512);
    for (int row = 0; row < 64; ++row)
      for (int i = 0; i < 512; ++i)
        repeated[row].push_back(i);
    cases.push_back(rows(repeated));
    std::vector<std::vector<int32_t>> boundary(1024);
    for (int v = 1; v < 1024; ++v)
      for (int rep = 0; rep < 15 + v % 3; ++rep) {
        boundary[0].push_back(0);
        boundary[0].push_back(v);
      }
    cases.push_back(rows(boundary));
    std::mt19937 rng(73);
    for (int trial = 0; trial < 30; ++trial) {
      int n = 2 + rng() % 40;
      std::vector<std::vector<int32_t>> graph(n);
      for (auto &row : graph)
        for (int j = 0, degree = rng() % 100; j < degree; ++j)
          row.push_back(rng() % n);
      cases.push_back(rows(graph));
    }
#ifdef COMPRESSGRAPH_TEST_CUDA
    warmup_cuda();
#endif
    int checked = 0;
    for (const auto &graph : cases)
      for (int threshold : {3, 16, 255, 256}) {
        Options opt;
        opt.threads = 1;
        opt.min_frequency = threshold;
        auto reference = compress_cpu(graph, opt);
        verify(graph, reference);
        opt.threads = 4;
        auto parallel = compress_cpu(graph, opt);
        verify(graph, parallel);
        equal(reference, parallel);
#ifdef COMPRESSGRAPH_TEST_CUDA
        auto gpu = compress_cuda(graph, opt);
        verify(graph, gpu);
        equal(reference, gpu);
#endif
        ++checked;
      }
    for (auto bad : std::vector<Csr>{{{}, {}},
                                     {{1}, {}},
                                     {{0, 2, 1}, {0}},
                                     {{0, 1}, {-1}},
                                     {{0, 1}, {1}}}) {
      bool rejected = false;
      try {
        compress_cpu(bad);
      } catch (const std::invalid_argument &) {
        rejected = true;
      }
      if (!rejected)
        throw std::runtime_error("Malformed CSR was accepted");
#ifdef COMPRESSGRAPH_TEST_CUDA
      rejected = false;
      try {
        compress_cuda(bad);
      } catch (const std::invalid_argument &) {
        rejected = true;
      }
      if (!rejected)
        throw std::runtime_error("Malformed CUDA CSR was accepted");
#endif
    }
    for (int which = 0; which < 3; ++which) {
      Options bad;
      if (which == 0)
        bad.threads = 0;
      if (which == 1)
        bad.min_frequency = 2;
      if (which == 2)
        bad.rounds = 0;
      bool rejected = false;
      try {
        compress_cpu(cases[0], bad);
      } catch (const std::invalid_argument &) {
        rejected = true;
      }
      if (!rejected)
        throw std::runtime_error("Invalid option was accepted");
    }
    std::cout << checked
              << " fixture/threshold combinations passed exact expansion and "
                 "backend/thread equivalence\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
