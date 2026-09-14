# Parallel batch compression

These backends compress CSR graphs without a GNN training dependency. They preserve each input adjacency sequence exactly, including order and duplicate edges. The generated grammar can be expanded back to the input or used by the existing CompressGraph tools.

## Build

CPU (CMake 3.9+, a C++17 compiler with `std::filesystem`, and OpenMP):

```sh
cmake -S . -B build -DBATCH_COMPRESSION=ON -DBATCH_CUDA=OFF
cmake --build build -j
ctest --test-dir build --output-on-failure
```

CUDA is optional and independent of the Gunrock analytics build. Tested with CMake 3.24, GCC 11 and CUDA 12.3 on an RTX A6000:

```sh
cmake -S . -B build -DBATCH_CUDA=ON \
  -DCMAKE_CUDA_COMPILER=/usr/local/cuda/bin/nvcc \
  -DCMAKE_CUDA_ARCHITECTURES=86
cmake --build build -j
ctest --test-dir build --output-on-failure
```

The CUDA build requires CMake 3.18+ and a CUDA toolkit with CUB and C++17 support. Set the architecture for your GPU. CUDA-enabled tests require a visible GPU. Neither backend requires Python, PyTorch, pybind11, Ligra or Gunrock to run; the optional CLI integration tests use Python 3's standard library.

## Run

Use the same **raw int32 binary CSR** inputs as the original `compress` command. These are not `.npy` files. `vlist` has `n+1` offsets, starts with zero and ends at the edge count; `elist` contains vertex IDs in `[0,n)`. Input rows need not be sorted; order affects compression opportunities and is preserved.

```sh
./bin/compress_batch_cpu input/csr_vlist.bin input/csr_elist.bin output/cpu \
  --threads 20 --rounds 12 --min-frequency 16 --repeat 3 --verify

./bin/compress_batch_gpu input/csr_vlist.bin input/csr_elist.bin output/gpu \
  --rounds 12 --min-frequency 16 --repeat 3 --verify
```

Each output directory contains `csr_vlist.bin`, `csr_elist.bin` and `info.bin`. The first `n` rows are original vertices; subsequent rows are binary rules. `info.bin` contains two int32 values: original vertex count and rule count. Each rule references only original vertices or earlier rules. Outputs work with the existing `filter` and rule-processing tools. Existing output files are rejected rather than overwritten.

The native library interface is in `core/batch/compress.hpp`. Link `compressgraph_batch` for `compress_cpu`; link `compressgraph_batch_cuda` for `compress_cuda`. `verify` checks full sequence expansion. Both backends accept the same options and produce identical CSR for the same input, rounds and threshold. CUDA ignores the CPU `threads` setting.

Options:

- `--threads`: CPU OpenMP threads; default 4. Tune for the host, not universally 20.
- `--rounds`: batch replacement rounds, default 12; must be positive. This is not a training epoch count.
- `--min-frequency`: minimum pair occurrence count to create a rule, default 16, minimum 3. It is different from the existing post-compression `filter` contribution threshold. Lower values may improve compression at greater cost.
- `--repeat`: independent compression calls, default 1; only the last output is saved.
- `--verify`: exact expansion of all rows after timing; can take longer than compression.

The implementation uses int32 symbols and serialized offsets. It rejects oversized inputs and conservative rule-capacity overflow. It is an in-memory compressor, not an out-of-core implementation. GPU allocation failure is reported as an error. CPU and CUDA batch selection differ from the original greedy Re-Pair order and do not promise the same compression ratio as that algorithm.

## Timing

The CLI prints JSON with `compression_seconds` (median), `times`, `initialization_seconds`, graph counts and `verified`. Compression timing includes input validation and output construction. GPU timing also includes per-call device allocations/frees and both host-to-device and device-to-host copies. The CUDA context is initialized once before these calls and its initialization time is reported separately. Disk reads/writes and `--verify` are outside the compression timer; CLI wall time is therefore longer.

The native commands were validated on Reddit (232,965 vertices, 114,615,892 input edges), with 12 rounds and frequency 16. In one three-repeat test on an i7-12700K / RTX A6000, CPU compression took a median 2.635 s with 20 threads, and CUDA compression took 0.418 s including allocation and bidirectional transfer. CUDA context initialization was reported separately (0.210 s in that run). Both backends produced 68,668,533 stored edges, including rule edges (40.09% reduction), byte-equivalent to the previously validated prototypes.

These are hardware-specific measurements, not a timing guarantee. Earlier Python prototypes measured about 0.382 s on the GPU; differences in wrappers, allocations and run conditions mean those values should not be substituted for this native CLI's measured result. Use `--repeat` to measure your build. Disk loading and cold CUDA startup are not included in the compression median.

## Algorithm

Each round deterministically assigns every current symbol to an L/R partition and considers adjacent L-to-R pairs within each row. Their occurrences cannot overlap. High-frequency pairs receive rule IDs in sorted key order, and all their occurrences are replaced in parallel. Later rounds may compress pairs containing earlier rules; all rounds preserve exact expansion.

The CPU backend uses OpenMP, high-key buckets, dynamic bucket scheduling, and a saturating hash prefilter. Hash counts only overestimate a key's frequency, so the prefilter may retain extra low-frequency keys but never discards an eligible pair. Exact sorted grouping decides replacements. The implementation reuses buffers and tracks the selected prefix without shrinking candidate storage after prefiltering. It restores the caller's OpenMP thread setting on return or exception.

The CUDA backend keeps graph and work buffers on the device throughout the rounds. Warp-per-row scans use ballots and prefix sums; CUB radix sorting and run-length encoding find repeated pairs. Device kernels assign deterministic rules, replace nonoverlapping pairs and compact the graph. Only small size counters return to the CPU between rounds. The initial CUDA backend sorts all candidates, without the CPU hash prefilter.

L/R selection does not select identical-symbol pairs. More rounds expose more pairs but do not guarantee greedy optimality; an empty round is not a proof of convergence. CPU prefiltering and CUDA sorting are implementation differences, not approximation of the input graph.

## Original serial compressor

The original command remains available, with an optional frequency cutoff:

```sh
compress csr_vlist.bin csr_elist.bin       # original frequency-2 behavior
compress csr_vlist.bin csr_elist.bin 16    # skip low-frequency rules
```

A cutoff leaves uncompressed symbols explicit, so expansion remains exact. It changes compression ratio and is independent of subsequent rule filtering. The batch backends supplement rather than silently replace the original algorithm.
