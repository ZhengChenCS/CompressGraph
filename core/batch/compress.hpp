#pragma once
#include <cstdint>
#include <vector>

namespace compressgraph {
struct Csr {
  std::vector<int32_t> rowptr;
  std::vector<int32_t> col;
};
struct Options {
  int threads = 4;
  int min_frequency = 16;
  int rounds = 12;
};
struct Result {
  Csr graph;
  int32_t vertices = 0;
  int32_t rules = 0;
  // round, sorted candidates, new rules, replacements, candidate seconds,
  // sort seconds, remaining seconds, round seconds, group, mark, size,
  // compact seconds, candidates before prefilter.
  std::vector<std::vector<double>> stats;
};
void validate(const Csr &input, const Options &options);
Result compress_cpu(const Csr &input, const Options &options = {});
// Defined only by the optional compressgraph_batch_cuda target.
Result compress_cuda(const Csr &input, const Options &options = {});
void warmup_cuda();
// Throws if any expanded row differs, including ordering and multiplicity.
void verify(const Csr &input, const Result &result);
} // namespace compressgraph
