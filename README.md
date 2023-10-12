# CompressGraph

## Description
This is the open-source implementation for CompressGraph:

CompressGraph: Efficient Parallel Graph Analytics with Rule-Based Compression”, Zheng Chen, Feng Zhang, Jiawei Guan, Jidong Zhai, Xipeng Shen, Huanchen Zhang, Wentong Shu, 
Xiaoyong Du. SIGMOD/PODS '23: Proceedings of the 2023 International Conference on Management of Data.
https://dl.acm.org/doi/10.1145/3588684


## System Dependency
 - [CMake](https://gitlab.kitware.com/cmake/cmake)
 - Optional(CPU): [Ligra](https://github.com/jshun/ligra.git)
 - Optional(GPU): [Gunrock](https://github.com/gunrock/gunrock.git)

 ## Compilation
```bash
git clone https://github.com/ZhengChenCS/CompressGraph.git --recursive
cd CompressGraph
mkdir -p build
cd build
cmake ..
make -j
```

## Graph Compression

The Compression Mudule accepts binary CSR(Compressed Sparse Row) graph data as input. The `dataset` folder provides an example.

To compress a input graph, run:
```shell
compress <csr_vlist.bin> <csr_elist.bin>
```

To filter the rule by the threshold(16 by default), run:
```shell
filter <csr_vlist.bin> <csr_elist.bin> <info.bin> 16
```

The `script` folder provides the example of invoke compression and filtering.

## Graph analytics on CPU

CompressGraph implements multi-core parallel graph analytics on CPU based on [Ligra](https://github.com/jshun/ligra.git).

## Graph analytics on GPU

CompressGraph implements multi-core parallel graph analytics on CPU based on [Gunrock](https://github.com/gunrock/gunrock.git).


## Citation

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



