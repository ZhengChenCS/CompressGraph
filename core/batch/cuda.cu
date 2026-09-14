#include "compress.hpp"
#include <algorithm>
#include <chrono>
#include <climits>
#include <cub/cub.cuh>
#include <cuda_runtime.h>
#include <stdexcept>
#include <vector>
namespace compressgraph {
#define CK(call)                                                               \
  do {                                                                         \
    auto err = (call);                                                         \
    if (err != cudaSuccess)                                                    \
      throw std::runtime_error(cudaGetErrorString(err));                       \
  } while (0)
struct Memory {
  std::vector<void *> blocks;
  template <class T> T *get(size_t n) {
    void *p;
    CK(cudaMalloc(&p, std::max(size_t(1), n) * sizeof(T)));
    blocks.push_back(p);
    return (T *)p;
  }
  ~Memory() {
    for (auto p : blocks)
      cudaFree(p);
  }
};
__device__ uint64_t mix(uint64_t x) {
  x += 0x9e3779b97f4a7c15ULL;
  x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
  x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
  return x ^ (x >> 31);
}
__global__ void sides(unsigned char *side, int n, int round) {
  int i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i < n)
    side[i] = mix(uint64_t(i) ^ mix(round + 17)) & 1;
}
// Each warp owns one row; contiguous lanes access contiguous neighbors.
__global__ void count_rows(const int *ptr, const int *text,
                           const unsigned char *side, int *count, int n,
                           bool pairs) {
  int row = (blockIdx.x * blockDim.x + threadIdx.x) / 32,
      lane = threadIdx.x % 32;
  if (row >= n)
    return;
  int value = 0;
  for (int j = ptr[row] + lane; j < ptr[row + 1]; j += 32)
    value += pairs
                 ? (j + 1 < ptr[row + 1] && !side[text[j]] && side[text[j + 1]])
                 : (text[j] >= 0);
  for (int d = 16; d; d /= 2)
    value += __shfl_down_sync(0xffffffff, value, d);
  if (lane == 0)
    count[row] = value;
}
__global__ void fill_pairs(const int *ptr, const int *text,
                           const unsigned char *side, const int *offsets,
                           uint64_t *key, int *pos, int n) {
  int row = (blockIdx.x * blockDim.x + threadIdx.x) / 32,
      lane = threadIdx.x % 32;
  if (row >= n)
    return;
  int out = offsets[row];
  for (int base = ptr[row]; base < ptr[row + 1]; base += 32) {
    int j = base + lane;
    bool yes = j + 1 < ptr[row + 1] && !side[text[j]] && side[text[j + 1]];
    unsigned mask = __ballot_sync(0xffffffff, yes);
    if (yes) {
      int at = out + __popc(mask & ((1u << lane) - 1));
      key[at] = (uint64_t(uint32_t(text[j])) << 32) | uint32_t(text[j + 1]);
      pos[at] = j;
    }
    out += __popc(mask);
  }
}
__global__ void eligible(const int *freq, int *flag, int n, int minfreq) {
  int i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i < n)
    flag[i] = freq[i] >= minfreq;
}
__global__ void totals(const int *prefix, const int *flag, int n, int *total) {
  if (threadIdx.x == 0 && blockIdx.x == 0)
    *total = n ? prefix[n - 1] + flag[n - 1] : 0;
}
__global__ void replace_pairs(const uint64_t *unique, const int *freq,
                              const int *begin, const int *flag,
                              const int *prefix, const int *pos, int *text,
                              int *rules, int n, int oldrules, int groups) {
  int g = (blockIdx.x * blockDim.x + threadIdx.x) / 32, lane = threadIdx.x % 32;
  if (g >= groups || !flag[g])
    return;
  int r = oldrules + prefix[g], id = n + r;
  if (lane == 0) {
    rules[2 * r] = unique[g] >> 32;
    rules[2 * r + 1] = uint32_t(unique[g]);
  }
  for (int j = begin[g] + lane; j < begin[g] + freq[g]; j += 32) {
    int p = pos[j];
    text[p] = id;
    text[p + 1] = -1;
  }
}
__global__ void compact(const int *ptr, const int *text, const int *newptr,
                        int *next, int n) {
  int row = (blockIdx.x * blockDim.x + threadIdx.x) / 32,
      lane = threadIdx.x % 32;
  if (row >= n)
    return;
  int out = newptr[row];
  for (int base = ptr[row]; base < ptr[row + 1]; base += 32) {
    int j = base + lane;
    bool yes = j < ptr[row + 1] && text[j] >= 0;
    unsigned mask = __ballot_sync(0xffffffff, yes);
    if (yes)
      next[out + __popc(mask & ((1u << lane) - 1))] = text[j];
    out += __popc(mask);
  }
}
static double now() {
  CK(cudaDeviceSynchronize());
  return std::chrono::duration<double>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}
Result compress_cuda(const Csr &input, const Options &options) {
  validate(input, options);
  int minfreq = options.min_frequency, rounds = options.rounds;
  int n = input.rowptr.size() - 1, E = input.col.size();
  std::vector<int> hp(input.rowptr.begin(), input.rowptr.end());
  int maxrules = E / (minfreq - 2);
  int nr = 0, edges = E;
  std::vector<int> roots, hostrules;
  std::vector<std::vector<double>> stats;
  {
    Memory mem;
    int *ptr = mem.get<int>(n + 1);
    int *newptr = mem.get<int>(n + 1);
    int *offsets = mem.get<int>(n + 1);
    int *rowcounts = mem.get<int>(n + 1);
    int *text = mem.get<int>(E);
    int *next = mem.get<int>(E);
    int *rules = mem.get<int>(size_t(maxrules) * 2);
    auto side = mem.get<unsigned char>(size_t(n) + maxrules);
    int cap = std::max(1, E / 2);
    auto k0 = mem.get<uint64_t>(cap);
    auto k1 = mem.get<uint64_t>(cap);
    auto unique = mem.get<uint64_t>(cap);
    int *p0 = mem.get<int>(cap);
    int *p1 = mem.get<int>(cap);
    int *freq = mem.get<int>(cap);
    int *starts = mem.get<int>(cap);
    int *flag = mem.get<int>(cap);
    int *prefix = mem.get<int>(cap);
    int *scalar = mem.get<int>(1);
    size_t bytes = 0, b = 0;
    CK(cub::DeviceRadixSort::SortPairs(nullptr, b, k0, k1, p0, p1, cap));
    bytes = std::max(bytes, b);
    CK(cub::DeviceRunLengthEncode::Encode(nullptr, b, k1, unique, freq, scalar,
                                          cap));
    bytes = std::max(bytes, b);
    CK(cub::DeviceScan::ExclusiveSum(nullptr, b, freq, starts, cap));
    bytes = std::max(bytes, b);
    CK(cub::DeviceScan::ExclusiveSum(nullptr, b, rowcounts, offsets, n + 1));
    bytes = std::max(bytes, b);
    void *scratch = mem.get<unsigned char>(bytes);
    CK(cudaMemcpy(ptr, hp.data(), size_t(n + 1) * 4, cudaMemcpyHostToDevice));
    CK(cudaMemcpy(text, input.col.data(), size_t(E) * 4,
                  cudaMemcpyHostToDevice));
    CK(cudaMemset(rowcounts + n, 0, 4));
    for (int round = 0; round < rounds && edges; round++) {
      double t0 = now();
      sides<<<(n + nr + 255) / 256, 256>>>(side, n + nr, round);
      count_rows<<<(n + 3) / 4, 128>>>(ptr, text, side, rowcounts, n, true);
      b = bytes;
      CK(cub::DeviceScan::ExclusiveSum(scratch, b, rowcounts, offsets, n + 1));
      int count;
      CK(cudaMemcpy(&count, offsets + n, 4, cudaMemcpyDeviceToHost));
      fill_pairs<<<(n + 3) / 4, 128>>>(ptr, text, side, offsets, k0, p0, n);
      double t1 = now();
      int groups = 0, added = 0, oldedges = edges;
      double t2 = t1, t3 = t1, t4 = t1, t5 = t1;
      if (count) {
        b = bytes;
        CK(cub::DeviceRadixSort::SortPairs(scratch, b, k0, k1, p0, p1, count));
        t2 = now();
        b = bytes;
        CK(cub::DeviceRunLengthEncode::Encode(scratch, b, k1, unique, freq,
                                              scalar, count));
        CK(cudaMemcpy(&groups, scalar, 4, cudaMemcpyDeviceToHost));
        eligible<<<(groups + 255) / 256, 256>>>(freq, flag, groups, minfreq);
        b = bytes;
        CK(cub::DeviceScan::ExclusiveSum(scratch, b, flag, prefix, groups));
        totals<<<1, 1>>>(prefix, flag, groups, scalar);
        CK(cudaMemcpy(&added, scalar, 4, cudaMemcpyDeviceToHost));
        b = bytes;
        CK(cub::DeviceScan::ExclusiveSum(scratch, b, freq, starts, groups));
        t3 = now();
        if (added) {
          if (int64_t(nr) + added > maxrules)
            throw std::runtime_error("Rule capacity exceeded");
          replace_pairs<<<(groups + 3) / 4, 128>>>(unique, freq, starts, flag,
                                                   prefix, p1, text, rules, n,
                                                   nr, groups);
          nr += added;
        }
        t4 = now();
        if (added) {
          count_rows<<<(n + 3) / 4, 128>>>(ptr, text, side, rowcounts, n,
                                           false);
          b = bytes;
          CK(cub::DeviceScan::ExclusiveSum(scratch, b, rowcounts, newptr,
                                           n + 1));
          CK(cudaMemcpy(&edges, newptr + n, 4, cudaMemcpyDeviceToHost));
          t5 = now();
          compact<<<(n + 3) / 4, 128>>>(ptr, text, newptr, next, n);
          std::swap(ptr, newptr);
          std::swap(text, next);
        } else
          t5 = t4;
      }
      double end = now();
      stats.push_back({double(round), double(count), double(added),
                       double(oldedges - edges), t1 - t0, t2 - t1, end - t2,
                       end - t0, t3 - t2, t4 - t3, t5 - t4, end - t5,
                       double(count)});
    }
    CK(cudaMemcpy(hp.data(), ptr, size_t(n + 1) * 4, cudaMemcpyDeviceToHost));
    roots.resize(edges);
    hostrules.resize(size_t(nr) * 2);
    CK(cudaMemcpy(roots.data(), text, size_t(edges) * 4,
                  cudaMemcpyDeviceToHost));
    CK(cudaMemcpy(hostrules.data(), rules, size_t(nr) * 8,
                  cudaMemcpyDeviceToHost));
  }
  Result result;
  result.vertices = n;
  result.rules = nr;
  result.stats = std::move(stats);
  result.graph.rowptr = std::move(hp);
  for (int r = 0; r < nr; r++)
    result.graph.rowptr.push_back(int64_t(edges) + 2 * (r + 1));
  result.graph.col = std::move(roots);
  result.graph.col.insert(result.graph.col.end(), hostrules.begin(),
                          hostrules.end());
  return result;
}
void warmup_cuda() { CK(cudaFree(0)); }
} // namespace compressgraph
