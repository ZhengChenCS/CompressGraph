#include "compress.hpp"
#include <climits>
#include <stdexcept>
namespace compressgraph {
void validate(const Csr &input, const Options &options) {
  if (input.rowptr.empty() || input.rowptr.size() > INT_MAX ||
      input.col.size() > INT_MAX - 64 || options.threads < 1 ||
      options.min_frequency < 3 || options.rounds < 1)
    throw std::invalid_argument("Invalid CSR/options: int32 CSR, threads >= 1, "
                                "min_frequency >= 3, rounds >= 1 required");
  const int64_t n = input.rowptr.size() - 1;
  if (input.rowptr.front() != 0 ||
      input.rowptr.back() != int64_t(input.col.size()))
    throw std::invalid_argument("CSR offsets do not match column count");
  for (size_t i = 1; i < input.rowptr.size(); ++i)
    if (input.rowptr[i] < input.rowptr[i - 1])
      throw std::invalid_argument("CSR offsets must be nondecreasing");
  for (auto v : input.col)
    if (v < 0 || v >= n)
      throw std::invalid_argument("Vertex ID outside input graph");
  // Every rule reduces stored edges by at least min_frequency - 2.
  if (n + input.col.size() / (options.min_frequency - 2) > INT_MAX - 256)
    throw std::invalid_argument(
        "Conservative rule/symbol capacity exceeds int32");
}
} // namespace compressgraph
namespace compressgraph {
void verify(const Csr &input, const Result &result) {
  const auto &v = result.graph.rowptr;
  const auto &e = result.graph.col;
  const int64_t n = input.rowptr.size() - 1;
  if (result.vertices != n || result.rules < 0 ||
      v.size() != size_t(n + result.rules + 1) || v.empty() || v.front() != 0 ||
      v.back() != int64_t(e.size()))
    throw std::runtime_error("Invalid compressed graph dimensions");
  for (size_t i = 1; i < v.size(); ++i)
    if (v[i] < v[i - 1])
      throw std::runtime_error("Invalid compressed offsets");
  for (auto symbol : e)
    if (symbol < 0 || symbol >= n + result.rules)
      throw std::runtime_error("Invalid compressed symbol");
  for (int64_t r = n; r < n + result.rules; ++r)
    for (int j = v[r]; j < v[r + 1]; ++j)
      if (e[j] >= r)
        throw std::runtime_error("Rule is not acyclic in creation order");
  std::vector<int32_t> stack;
  for (int64_t row = 0; row < n; ++row) {
    stack.clear();
    for (int j = v[row + 1]; j > v[row];)
      stack.push_back(e[--j]);
    int64_t offset = input.rowptr[row];
    while (!stack.empty()) {
      int symbol = stack.back();
      stack.pop_back();
      if (symbol < n) {
        if (offset >= input.rowptr[row + 1] || input.col[offset++] != symbol)
          throw std::runtime_error("Expanded sequence differs from input");
      } else {
        for (int j = v[symbol + 1]; j > v[symbol];)
          stack.push_back(e[--j]);
      }
    }
    if (offset != input.rowptr[row + 1])
      throw std::runtime_error("Expanded row has wrong length");
  }
}
} // namespace compressgraph
