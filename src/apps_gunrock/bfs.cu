#include "bfs.hxx"
#include <cstdlib>

using namespace gunrock;
using namespace memory;

void test_bfs(int num_arguments, char** argument_array) {
  if (num_arguments != 3) {
    fprintf(stderr, "Usage: %s filename.mtx single_source\n", argument_array[0]);
    exit(1);
  }
  // --
  // Define types

  using vertex_t = int;
  using edge_t = int;
  using weight_t = float;

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

  thrust::device_vector<vertex_t> row_indices(csr.number_of_nonzeros);
  thrust::device_vector<vertex_t> column_indices(csr.number_of_nonzeros);
  thrust::device_vector<edge_t> column_offsets(csr.number_of_columns + 1);

  // --
  // Build graph + metadata

  auto G =
      graph::build::from_csr<memory_space_t::device,
                             graph::view_t::csr /* | graph::view_t::csc */>(
          csr.number_of_rows,               // rows
          csr.number_of_columns,            // columns
          csr.number_of_nonzeros,           // nonzeros
          csr.row_offsets.data().get(),     // row_offsets
          csr.column_indices.data().get(),  // column_indices
          csr.nonzero_values.data().get(),  // values
          row_indices.data().get(),         // row_indices
          column_offsets.data().get()       // column_offsets
      );

  // --
  // Params and memory allocation

  vertex_t single_source = atoi(argument_array[2]);
  std::cout << "Single Source = " << single_source << std::endl;

  vertex_t n_vertices = G.get_number_of_vertices();
  thrust::device_vector<vertex_t> parents(n_vertices);

  // --
  // Run problem
  float gpu_elapsed = 0.0f;
  int num_runs = 5;

  for (auto i = 0; i < num_runs; i++)
    gpu_elapsed += gunrock::bfs::run(G, single_source, parents.data().get());

  gpu_elapsed /= num_runs;

  std::cout << "GPU Elapsed Time : " << gpu_elapsed << " (ms)" << std::endl;
}

int main(int argc, char** argv) {
  test_bfs(argc, argv);
}
