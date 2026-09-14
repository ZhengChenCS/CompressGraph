#include "compress.hpp"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <limits>
#include <omp.h>
#include <stdexcept>
#include <vector>
namespace compressgraph {
struct Occ {
  uint64_t key;
  int64_t pos;
};
struct Group {
  int64_t begin, end;
  int32_t id;
};
static uint64_t mix(uint64_t x) {
  x += 0x9e3779b97f4a7c15ULL;
  x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
  x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
  return x ^ (x >> 31);
}
// One parallel high-key partition, followed by independent cache-sized sorts.
static size_t radix_sort(std::vector<Occ> &a, std::vector<Occ> &tmp,
                         int threads, uint64_t symbols, int minfreq) {
  auto less = [](const Occ &x, const Occ &y) { return x.key < y.key; };
  if (a.size() < 4096) {
    std::sort(a.begin(), a.end(), less);
    return a.size();
  }
  tmp.resize(a.size());
  const int buckets = 256;
  std::vector<uint64_t> hist(threads * buckets), dest(threads * buckets),
      bounds(buckets + 1);
  int bits = 0;
  for (uint64_t x = symbols - 1; x; x >>= 1)
    bits++;
  int shift = 32 + std::max(0, bits - 8);
#pragma omp parallel for schedule(static)
  for (int t = 0; t < threads; t++) {
    auto h = hist.data() + t * buckets;
    for (size_t j = a.size() * t / threads; j < a.size() * (t + 1) / threads;
         j++)
      h[(a[j].key >> shift) & 255]++;
  }
  uint64_t sum = 0;
  for (int b = 0; b < buckets; b++) {
    bounds[b] = sum;
    for (int t = 0; t < threads; t++) {
      dest[t * buckets + b] = sum;
      sum += hist[t * buckets + b];
    }
  }
  bounds[buckets] = sum;
#pragma omp parallel for schedule(static)
  for (int t = 0; t < threads; t++) {
    auto d = dest.data() + t * buckets;
    for (size_t j = a.size() * t / threads; j < a.size() * (t + 1) / threads;
         j++)
      tmp[d[(a[j].key >> shift) & 255]++] = a[j];
  }
  // Saturating hash counts upper-bound each key's true frequency.
  // Collisions keep extra keys but cannot discard an eligible key.
  std::vector<uint64_t> kept(buckets), out(buckets + 1);
#pragma omp parallel for schedule(dynamic, 1)
  for (int b = 0; b < buckets; b++) {
    uint64_t begin = bounds[b], end = bounds[b + 1], write = begin;
    if (end - begin >= 4096 && minfreq <= 255) {
      size_t capacity = 1;
      while (capacity < end - begin)
        capacity <<= 1;
      std::vector<uint8_t> count(capacity, 0);
      for (uint64_t j = begin; j < end; j++) {
        auto &value = count[mix(tmp[j].key) & (capacity - 1)];
        if (value < minfreq)
          value++;
      }
      for (uint64_t j = begin; j < end; j++)
        if (count[mix(tmp[j].key) & (capacity - 1)] >= minfreq)
          tmp[write++] = tmp[j];
    } else
      write = end;
    kept[b] = write - begin;
    std::sort(tmp.begin() + begin, tmp.begin() + write, less);
  }
  for (int b = 0; b < buckets; b++)
    out[b + 1] = out[b] + kept[b];
#pragma omp parallel for schedule(static)
  for (int b = 0; b < buckets; b++)
    std::copy(tmp.begin() + bounds[b], tmp.begin() + bounds[b] + kept[b],
              a.begin() + out[b]);
  // Retain constructed storage; the next round overwrites every input record.
  return out.back();
}
Result compress_cpu(const Csr &input, const Options &options) {
  validate(input, options);
  int threads = options.threads, minfreq = options.min_frequency,
      maxrounds = options.rounds;
  int64_t n = input.rowptr.size() - 1;
  std::vector<int64_t> ptr(input.rowptr.begin(), input.rowptr.end());
  std::vector<int32_t> text(input.col), rules;
  std::vector<std::vector<double>> stats;
  std::vector<int32_t> next;
  std::vector<Occ> occ, scratch;
  struct RestoreThreads {
    int saved = omp_get_max_threads();
    ~RestoreThreads() { omp_set_num_threads(saved); }
  } restore;
  omp_set_num_threads(threads);
  for (int round = 0; round < maxrounds && !text.empty(); round++) {
    auto start = std::chrono::steady_clock::now();
    std::vector<uint8_t> side(n + rules.size() / 2);
#pragma omp parallel for schedule(static)
    for (int64_t j = 0; j < (int64_t)side.size(); j++)
      side[j] = mix(uint64_t(j) ^ mix(round + 17)) & 1;
    std::vector<int64_t> offsets(n + 1, 0);
#pragma omp parallel for schedule(static)
    for (int64_t row = 0; row < n; row++) {
      int64_t count = 0;
      for (int64_t j = ptr[row]; j + 1 < ptr[row + 1]; j++)
        count += !side[text[j]] && side[text[j + 1]];
      offsets[row + 1] = count;
    }
    for (int64_t row = 0; row < n; row++)
      offsets[row + 1] += offsets[row];
    occ.resize(offsets.back());
#pragma omp parallel for schedule(static)
    for (int64_t row = 0; row < n; row++) {
      int64_t out = offsets[row];
      for (int64_t j = ptr[row]; j + 1 < ptr[row + 1]; j++)
        if (!side[text[j]] && side[text[j + 1]])
          occ[out++] = {
              uint64_t(uint32_t(text[j])) << 32 | uint32_t(text[j + 1]), j};
    }
    auto counted = std::chrono::steady_clock::now();
    int64_t before_filter = occ.size();
    int64_t active = radix_sort(occ, scratch, threads, side.size(), minfreq);
    auto sorted = std::chrono::steady_clock::now();
    // Partition sorted keys, never splitting a run. Prefixes preserve rule IDs.
    std::vector<int64_t> cuts(threads + 1, 0), bases(threads + 1, 0);
    cuts[threads] = active;
#pragma omp parallel for schedule(static)
    for (int t = 1; t < threads; t++) {
      int64_t mid = active * t / threads;
      if (mid < active)
        cuts[t] = std::lower_bound(
                      occ.begin(), occ.begin() + active, occ[mid].key,
                      [](const Occ &a, uint64_t key) { return a.key < key; }) -
                  occ.begin();
    }
    std::vector<std::vector<Group>> local(threads);
    int64_t removed = 0;
#pragma omp parallel for reduction(+ : removed) schedule(static)
    for (int t = 0; t < threads; t++) {
      for (int64_t begin = cuts[t]; begin < cuts[t + 1];) {
        int64_t end = begin + 1;
        while (end < cuts[t + 1] && occ[end].key == occ[begin].key)
          end++;
        if (end - begin >= minfreq) {
          local[t].push_back({begin, end, 0});
          removed += end - begin;
        }
        begin = end;
      }
      bases[t + 1] = local[t].size();
    }
    for (int t = 0; t < threads; t++)
      bases[t + 1] += bases[t];
    int64_t first = n + rules.size() / 2, ng = bases.back();
    if (first + ng > int64_t(std::numeric_limits<int32_t>::max()) + 1)
      throw std::overflow_error("Too many rules");
    rules.resize(rules.size() + 2 * ng);
    auto grouped = std::chrono::steady_clock::now();
// Eligible pairs do not overlap. Update the working sequence directly;
// negative symbols mark deletion, eliminating a full-size mark buffer.
#pragma omp parallel for schedule(static)
    for (int t = 0; t < threads; t++) {
      for (int64_t g = 0; g < (int64_t)local[t].size(); g++) {
        auto range = local[t][g];
        int32_t id = first + bases[t] + g;
        auto key = occ[range.begin].key;
        rules[2 * (int64_t(id) - n)] = key >> 32;
        rules[2 * (int64_t(id) - n) + 1] = uint32_t(key);
        for (int64_t j = range.begin; j < range.end; j++) {
          auto pos = occ[j].pos;
          text[pos] = id;
          text[pos + 1] = -1;
        }
      }
    }
    auto marked = std::chrono::steady_clock::now();
    auto sized = marked;
    if (removed) {
      offsets.assign(n + 1, 0);
#pragma omp parallel for schedule(static)
      for (int64_t row = 0; row < n; row++) {
        int64_t size = 0;
        for (int64_t j = ptr[row]; j < ptr[row + 1]; j++)
          size += text[j] >= 0;
        offsets[row + 1] = size;
      }
      for (int64_t row = 0; row < n; row++)
        offsets[row + 1] += offsets[row];
      next.resize(offsets.back());
      sized = std::chrono::steady_clock::now();
#pragma omp parallel for schedule(static)
      for (int64_t row = 0; row < n; row++) {
        int64_t out = offsets[row];
        for (int64_t j = ptr[row]; j < ptr[row + 1]; j++)
          if (text[j] >= 0)
            next[out++] = text[j];
      }
      ptr.swap(offsets);
      text.swap(next);
    }
    auto end = std::chrono::steady_clock::now();
    auto secs = [](auto a, auto b) {
      return std::chrono::duration<double>(b - a).count();
    };
    stats.push_back({double(round), double(active), double(ng), double(removed),
                     secs(start, counted), secs(counted, sorted),
                     secs(sorted, end), secs(start, end), secs(sorted, grouped),
                     secs(grouped, marked), secs(marked, sized),
                     secs(sized, end), double(before_filter)});
    // Empty rounds need not imply convergence: the next partition exposes other
    // pairs.
  }

  int64_t nr = rules.size() / 2;
  Result result;
  result.vertices = n;
  result.rules = nr;
  result.stats = std::move(stats);
  result.graph.rowptr.assign(ptr.begin(), ptr.end());
  for (int64_t r = 0; r < nr; r++)
    result.graph.rowptr.push_back(text.size() + 2 * (r + 1));
  result.graph.col = std::move(text);
  result.graph.col.insert(result.graph.col.end(), rules.begin(), rules.end());
  return result;
}
} // namespace compressgraph
