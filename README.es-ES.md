

# CompressGraph

## 1. Descripción
Esta es la implementación de código abierto para CompressGraph:

CompressGraph: Efficient Parallel Graph Analytics with Rule-Based Compression”, Zheng Chen, Feng Zhang, Jiawei Guan, Jidong Zhai, Xipeng Shen, Huanchen Zhang, Wentong Shu, 
Xiaoyong Du. SIGMOD/PODS '23: Proceedings of the 2023 International Conference on Management of Data.
https://dl.acm.org/doi/10.1145/3588684

## 2. Cómo empezar

### 2.1 Dependencias del sistema
 - [CMake](https://gitlab.kitware.com/cmake/cmake)
 - OpenMP y C++17
 - CUDA
 - Opcional(CPU): [Ligra](https://github.com/jshun/ligra.git)
 - Opcional(GPU): [Gunrock](https://github.com/gunrock/gunrock.git)

### 2.2 Compilación

```shell
git clone https://github.com/ZhengChenCS/CompressGraph.git --recursive
cd CompressGraph
mkdir -p build
cd build
cmake .. -DLIGRA=ON -DGUNROCK=ON
make -j
```

Si solo desea ejecutar la aplicación en CPU, puede establecer `-DGUNROCK=OFF`.


### 2.3 Formato de entrada del grafo

El formato de entrada inicial del grafo debe estar en el [formato de grafo de adyacencia](https://www.cs.cmu.edu/~pbbs/benchmarks/graphIO.html). A continuación se muestran, como ejemplo, el formato SNAP (lista de aristas) y el formato de grafo de adyacencia para un grafo de muestra.

Formato SNAP:

```
src dst
0 1
0 2
2 0
2 1
```

Formato de grafo de adyacencia:

```
AdjacencyGraph
3 <El número de vértices>
4 <El número de aristas>
0 <o0>
2 <o1>
2 <o2>
1 <e0>
2 <e1>
0 <e2>
1 <e3>
```

## 3. Compresión de CompressGraph

### 3.1 Preparación de datos

El Módulo de Compresión acepta datos de grafos binarios CSR (Compressed Sparse Row) como entrada, los cuales contienen una matriz `vlist` y una matriz `elist`. 
Proporcionamos dos programas para convertir archivos de grafos desde el formato de lista de aristas y el formato de grafo de adyacencia al formato CSR.
El usuario puede invocarlos de la siguiente manera:

* De lista de aristas a CSR 
```shell
edgelist2csr < <edgelist.txt>
```

* De grafo de adyacencia a CSR
```shell
adj2csr < <adjgraph.txt>
```

Los dos programas generarán dos archivos de salida en formato CSR: `csr_vlist.bin` y `csr_elist.bin` en el directorio actual.

### 3.2 Compresión de grafos 

La carpeta `dataset` proporciona un ejemplo.

Para comprimir un grafo de entrada, ejecute:
```shell
compress <csr_vlist.bin> <csr_elist.bin>
```

Para filtrar las reglas por el umbral (16 por defecto), ejecute:
```shell
filter <csr_vlist.bin> <csr_elist.bin> <info.bin> 16
```

El programa `filter` filtrará las reglas que cumplan `(freq - 1) * (len - 1) - 1 <= threshold;`, donde `freq` es la frecuencia de las reglas y `len` es la longitud de las reglas.

También proporcionamos un programa de filtrado `filter_decmp` que puede filtrar las reglas que no cumplen con los requisitos de frecuencia y longitud por separado.

```shell
filter_decomp <csr_vlist.bin> <csr_elist.bin> <info.bin> <freq_threshold> <len_threshold>
```

El `filter_decomp` filtrará las reglas que cumplan `freq < freq_threshold || len < len_threshold`.


## 4. Análisis de CompressGraph

Hemos implementado el motor de análisis de CompressGraph basado en [Ligra](https://github.com/jshun/ligra.git) para CPU y [Gunrock](https://github.com/gunrock/gunrock.git) para GPU.

### 4.1 Preparación de datos

Antes de ejecutar el programa de análisis de grafos, debemos convertir el formato CSR al formato requerido por Ligra.
Adicionalmente, generamos algunas estructuras auxiliares, incluyendo `order.bin` y `degree.bin` (usadas en `pagerank` y `hits`).

```shell
$convert2ligra $csr_vlist $csr_elist > $output
$save_degree $csr_vlist
$gene_rule_order $csr_vlist $csr_elist $info
```

Proporcionamos los scripts `data_prepare.sh` y `data_prepare_gpu.sh` para ejecutar el proceso de preparación de datos en el directorio `script`.

### 4.2 Ejecutar aplicaciones

Los usuarios pueden ejecutar las aplicaciones de grafos utilizando el siguiente enfoque:

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


Proporcionamos scripts en los directorios `script/cpu` y `script/gpu` para ejecutar estos programas.

### 4.3 Ejecutar aplicaciones con scripts

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

## 5. Referencias

Si utiliza nuestro código, cite nuestro artículo:

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
