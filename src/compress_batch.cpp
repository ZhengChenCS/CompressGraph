#include "core/batch/compress.hpp"
#include <algorithm>
#include <chrono>
#include <climits>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;
static double elapsed(Clock::time_point start) {
  return std::chrono::duration<double>(Clock::now() - start).count();
}
static int number(const char *text) {
  size_t used = 0;
  const std::string arg(text);
  auto value = std::stoll(arg, &used);
  if (used != arg.size() || value < 1 || value > INT_MAX)
    throw std::invalid_argument("Expected a positive integer: " + arg);
  return static_cast<int>(value);
}
static std::vector<int32_t> read(const fs::path &path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file)
    throw std::runtime_error("Cannot open input: " + path.string());
  auto bytes = file.tellg();
  if (bytes < 0 || bytes % 4 != 0 || bytes / 4 > INT_MAX)
    throw std::runtime_error("Invalid int32 binary size: " + path.string());
  std::vector<int32_t> data(static_cast<size_t>(bytes / 4));
  file.seekg(0);
  if (bytes && !file.read(reinterpret_cast<char *>(data.data()), bytes))
    throw std::runtime_error("Cannot read input: " + path.string());
  return data;
}
static void write(const fs::path &path, const std::vector<int32_t> &data) {
  std::ofstream file(path, std::ios::binary);
  if (!file)
    throw std::runtime_error("Cannot create output: " + path.string());
  file.write(reinterpret_cast<const char *>(data.data()),
             data.size() * sizeof(int32_t));
  file.close();
  if (!file)
    throw std::runtime_error("Cannot write output: " + path.string());
}
int main(int argc, char **argv) {
  try {
    if (argc == 2 && std::string(argv[1]) == "--help") {
      std::cout << "Usage: " << argv[0]
                << " vlist.bin elist.bin output_dir [--threads N] [--rounds N] "
                   "[--min-frequency N] [--repeat N] [--verify]\n";
      return 0;
    }
    if (argc < 4)
      throw std::invalid_argument(
          "Expected vlist, elist and output directory; use --help");
    compressgraph::Options options;
    int repeats = 1;
    bool check = false;
    for (int i = 4; i < argc; ++i) {
      std::string arg(argv[i]);
      if (arg == "--verify") {
        check = true;
        continue;
      }
      if (i + 1 == argc)
        throw std::invalid_argument("Missing option value: " + arg);
      int value = number(argv[++i]);
      if (arg == "--threads")
        options.threads = value;
      else if (arg == "--rounds")
        options.rounds = value;
      else if (arg == "--min-frequency")
        options.min_frequency = value;
      else if (arg == "--repeat")
        repeats = value;
      else
        throw std::invalid_argument("Unknown option: " + arg);
    }
    fs::path output(argv[3]);
    for (const auto *name : {"csr_vlist.bin", "csr_elist.bin", "info.bin"})
      if (fs::exists(output / name))
        throw std::runtime_error("Refusing to overwrite: " +
                                 (output / name).string());
    compressgraph::Csr input{read(argv[1]), read(argv[2])};
    compressgraph::validate(input, options);
    double initialization = 0;
#ifdef COMPRESSGRAPH_BATCH_GPU_CLI
    auto start = Clock::now();
    compressgraph::warmup_cuda();
    initialization = elapsed(start);
    auto compress = compressgraph::compress_cuda;
    const char *backend = "cuda";
#else
    auto compress = compressgraph::compress_cpu;
    const char *backend = "cpu";
#endif
    std::vector<double> times;
    compressgraph::Result result;
    for (int rep = 0; rep < repeats; ++rep) {
      auto start = Clock::now();
      auto current = compress(input, options);
      times.push_back(elapsed(start));
      result = std::move(current);
    }
    if (check)
      compressgraph::verify(input, result);
    fs::create_directories(output);
    write(output / "csr_vlist.bin", result.graph.rowptr);
    write(output / "csr_elist.bin", result.graph.col);
    write(output / "info.bin", {result.vertices, result.rules});
    auto sorted = times;
    std::sort(sorted.begin(), sorted.end());
    double median =
        (sorted[(sorted.size() - 1) / 2] + sorted[sorted.size() / 2]) / 2;
    std::cout << "{\"backend\":\"" << backend
              << "\",\"vertices\":" << result.vertices
              << ",\"input_edges\":" << input.col.size()
              << ",\"output_edges\":" << result.graph.col.size()
              << ",\"rules\":" << result.rules
              << ",\"rounds\":" << options.rounds
              << ",\"min_frequency\":" << options.min_frequency
              << ",\"compression_seconds\":" << median
              << ",\"initialization_seconds\":" << initialization
              << ",\"verified\":" << (check ? "true" : "false")
              << ",\"times\":[";
    for (size_t i = 0; i < times.size(); ++i)
      std::cout << (i ? "," : "") << times[i];
    std::cout << "]}\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Compression failed: " << e.what() << '\n';
    return 1;
  }
}
