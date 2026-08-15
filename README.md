# CompressGraph

## 1. Description
This is the open-source implementation for CompressGraph:

CompressGraph: Efficient Parallel Graph Analytics with Rule-Based Compression”, Zheng Chen, Feng Zhang, Jiawei Guan, Jidong Zhai, Xipeng Shen, Huanchen Zhang, Wentong Shu, 
Xiaoyong Du. SIGMOD/PODS '23: Proceedings of the 2023 International Conference on Management of Data.
https://dl.acm.org/doi/10.1145/3588684

## 2. Getting Started

### 2.1 System Dependency
 - [CMake](https://gitlab.kitware.com/cmake/cmake)
 - OpenMP and C++17
 - CUDA
 - Optional(CPU): [Ligra](https://github.com/jshun/ligra.git)
 - Optional(GPU): [Gunrock](https://github.com/gunrock/gunrock.git)

### 2.2 Compilation

```shell
git clone https://github.com/ZhengChenCS/CompressGraph.git --recursive
cd CompressGraph
mkdir -p build
cd build
cmake .. -DLIGRA=ON -DGUNROCK=ON
make -j
```

If you just want to run application on CPU, you can set `-DGUNROCK=OFF`.


### 2.3 Graph Input Format

The initial input graph format should be in the [adjacency graph format](https://www.cs.cmu.edu/~pbbs/benchmarks/graphIO.html). For example, the SNAP format(edgelist) and the adjacency graph format for a sample graph are shown below.

SNAP format:

```
src dst
0 1
0 2
2 0
2 1
```

Adjacency Graph format:

```
AdjacencyGraph
3 <The number of vertices>
4 <The number of edges>
0 <o0>
2 <o1>
2 <o2>
1 <e0>
2 <e1>
0 <e2>
1 <e3>
```

## 3. CompressGraph Compression

### 3.1 Data Preparation

The Compression Mudule accepts binary CSR(Compressed Sparse Row) graph data as input, which contains a `vlist` and a `elist` array. 
We provide two programs for converting graph files from edgelist and adjacency graph format to CSR format.
User can invoke them as follows:

* Edgelist to CSR 
```shell
edgelist2csr < <edgelist.txt>
```

* Adjacency graph to CSR
```shell
adj2csr < <adjgraph.txt>
```

The two programs will generate two output files in CSR format: `csr_vlist.bin` and `csr_elist.bin` in the current directory.

### 3.2 Graph Compression 

The `dataset` folder provides an example.

To compress a input graph, run:
```shell
compress <csr_vlist.bin> <csr_elist.bin>
```

To filter the rule by the threshold(16 by default), run:
```shell
filter <csr_vlist.bin> <csr_elist.bin> <info.bin> 16
```

The `filter` program will filter out rules that meet `(freq - 1) * (len - 1) - 1 <= threshold;`, where `freq` is the frequency of rules, `len` is the length of rules.

We also provide a filtering program `filter_decmp` that can filter out rules that do not meet the frequency and length requirements separately.

```shell
filter_decomp <csr_vlist.bin> <csr_elist.bin> <info.bin> <freq_threshold> <len_threshold>
```

The `filter_decomp` will filter out rules that meet `freq < freq_threshold || len < len_threshold`.


## 4. ComprassGraph Analytics

We have implemented the CompressGraph analytic engine based on [Ligra](https://github.com/jshun/ligra.git) on CPU and [Gunrock](https://github.com/gunrock/gunrock.git) on GPU.

### 4.1 Data Prepareation

Before running the graph analytic program, we need to convert the CSR format to the format required by Ligra.
Additionaly, we generate some auxiliary structures, including `order.bin` and `degree.bin`(used in `pagerank` and `hits`).

```shell
$convert2ligra $csr_vlist $csr_elist > $output
$save_degree $csr_vlist
$gene_rule_order $csr_vlist $csr_elist $info
```

We provide a script `data_prepare.sh` and `data_prepare_gpu.sh` to execute the data prepareation process in `script` directory.

### 4.2 Run Applications

Users can execute graph applications using the following approach:

```shell
./bfs_cpu -r 1 $file
./cc_cpu $file
./sssp_cpu -r 1 $file
./pagerank_cpu -maxiters 10 -i $info -d $degree -o $order $file
./topo_cpu -i $info $file
./hits_cpu -maxiters 10 -i $info -o $order $file
```

```shell
./bfs_gpu $file 0
./cc_gpu $file
./sssp_gpu $file $info 0
./pagerank_gpu $file
./hits_gpu $file
./topo_gpu $file $info
```


We provide scripts in `script/cpu` and `script/gpu` directory to execute these programs.

### 4.3 Run applications with script

```shell
cd script
bash data_prepare.sh
cd cpu
bash bfs.sh
bash sssp.sh
bash cc.sh
bash pagerank.sh
bash topo.sh
bash hits.sh
cd gpu
bash bfs.sh
bash sssp.sh
bash cc.sh
bash pagerank.sh
bash topo.sh
bash hits.sh
```

## 5. Citation

If you use our code, please cite our paper:

```
@article{chen2023compressgraph,
  title={CompressGraph: Efficient Parallel Graph Analytics with Rule-Based Compression},
  author={Chen, Zheng and Zhang, Feng and Guan, JiaWei and Zhai, Jidong and Shen, Xipeng and Zhang, Huanchen and Shu, Wentong and Du, Xiaoyong},
  journal={Proceedings of the ACM on Management of Data},
  volume={1},
  number={1},
  pages={1--31},
  year={2023},
  publisher={ACM New York, NY, USA}
}
```


## 6. Project Extensions (by hetvi3012)

This section details additional features and analyses added to the original CompressGraph framework. All scripts mentioned are located in the `script` directory or its subdirectories.

### 6.1 Triangle Counting (CPU)

* **Goal:** Implements a parallel triangle counting algorithm that operates directly on the CompressGraph representation using the Ligra framework.
* **Compilation:** The `triangle_cpu` executable is built automatically when compiling with `-DLIGRA=ON`. Ensure you have applied the `<cstdint>` header fix to the `deps/ligra` code.
* **How to Run:**
    ```bash
    cd script/cpu
    bash triangle.sh
    ```
* **Output:** Prints the total triangle count and the execution time to the console.

### 6.2 Compression Threshold Analysis

* **Goal:** Analyzes the trade-off between the compression rule filter threshold, the resulting graph size (compression ratio), and the performance of BFS.
* **How to Run:**
    1.  Ensure the baseline `.bin` files exist in `dataset/cnr-2000/compress/` by running `bash script/data_prepare.sh` once.
    2.  Run the analysis script:
        ```bash
        cd script
        ./analyze_threshold.sh
        ```
* **Output:** Prints a CSV table to the console showing `threshold`, `filtered_size_bytes`, `size_ratio`, and `bfs_time_seconds`. This data can be used to plot the trade-off curve.

### 6.3 Testing on Different Graph Types

* **Goal:** Evaluates the effectiveness of CompressGraph's rule-based compression and analytics performance on graph structures different from the default web graph. Tests on a collaboration network (`ca-GrQc`) and a road network (`roadNet-CA`).
* **Data Preparation:**
    1.  Download the datasets (run from the `dataset` directory):
        ```bash
        # Collaboration Network
        wget [https://snap.stanford.edu/data/ca-GrQc.txt.gz](https://snap.stanford.edu/data/ca-GrQc.txt.gz)
        gunzip ca-GrQc.txt.gz
        grep -v "^#" ca-GrQc.txt > ca-GrQc.edgelist

        # Road Network
        wget [https://snap.stanford.edu/data/roadNet-CA.txt.gz](https://snap.stanford.edu/data/roadNet-CA.txt.gz)
        gunzip roadNet-CA.txt.gz
        grep -v "^#" roadNet-CA.txt > roadNet-CA.edgelist
        ```
* **How to Run:**
    Use the `run_full_pipeline.sh` script, providing the base name of the graph edgelist file in the `dataset` directory. Run from the `script` directory:
    ```bash
    cd script
    ./run_full_pipeline.sh ca-GrQc
    ./run_full_pipeline.sh roadNet-CA
    ```
* **Output:** Creates a results directory for each graph (e.g., `ca-GrQc_results`, `roadNet-CA_results`) containing intermediate files, compression stats (`compress_stats.txt`), and prints analytics output to the console. Compare the compression ratios and run times against the `cnr-2000` graph. Note: PageRank may fail on these graphs.
