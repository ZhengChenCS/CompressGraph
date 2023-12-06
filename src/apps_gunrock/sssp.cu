#include "sssp.hxx"
#include <core/io.hpp>

using namespace gunrock;
using namespace memory;

void test_sssp(int num_arguments, char** argument_array) {
  if (num_arguments != 4) {
    fprintf(stderr, "Usage: %s filename.mtx info.bin single_source\n", argument_array[0]);
    exit(1);
  }

  // --
  // Define types

  using vertex_t = int;
  using edge_t = int;
  using weight_t = int;

  using csr_t =
      format::csr_t<memory_space_t::device, vertex_t, edge_t, weight_t>;

  // --
  // IO

  csr_t csr;
  std::string filename = argument_array[1];

  if (util::is_market(filename)) {
    io::matrix_market_t<vertex_t, edge_t, weight_t> mm;
    csr.from_coo(mm.load(filename));
  } else if (util::is_binary_csr(filename)) {
    csr.read_binary(filename);
  } else {
    std::cerr << "Unknown file format: " << filename << std::endl;
    exit(1);
  }

  // --
  // Build graph

  auto G = graph::build::from_csr<memory_space_t::device, graph::view_t::csr>(
      csr.number_of_rows,               // rows
      csr.number_of_columns,            // columns
      csr.number_of_nonzeros,           // nonzeros
      csr.row_offsets.data().get(),     // row_offsets
      csr.column_indices.data().get(),  // column_indices
      csr.nonzero_values.data().get()   // values
  );  // supports row_indices and column_offsets (default = nullptr)

  // --
  // Params and memory allocation
  srand(time(NULL));

  vertex_t n_vertices = G.get_number_of_vertices();
  vertex_t single_source = atoi(argument_array[3]);
  std::cout << "Single Source = " << single_source << std::endl;

  std::string info_path = argument_array[2];
  std::vector<vertex_t> info;
  vertex_t cnt = read_binary2vector(info_path, info);


  thrust::device_vector<weight_t> distances(n_vertices);

  float gpu_elapsed = 0.0f;
  int num_runs = 5;

  for (auto i = 0; i < num_runs; i++)
    gpu_elapsed += gunrock::sssp::run(G, single_source, info[0], distances.data().get());

  gpu_elapsed /= num_runs;

  std::cout << "GPU Elapsed Time : " << gpu_elapsed << " (ms)" << std::endl;
}

int main(int argc, char** argv) {
  test_sssp(argc, argv);
}
