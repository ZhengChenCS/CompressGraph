#pragma once

#include <gunrock/algorithms/algorithms.hxx>
#include <thrust/sort.h>
#include <thrust/reduce.h>
#include <thrust/iterator/counting_iterator.h>
#include <thrust/iterator/constant_iterator.h>
#include <thrust/functional.h>

namespace gunrock {
namespace topo {

template <typename vertex_t>
struct param_t {
    vertex_t num_origin_vertices;
    param_t(vertex_t _num_origin_vertices) : num_origin_vertices(_num_origin_vertices) {}
};

template <typename vertex_t>
struct result_t{
    vertex_t *topo_order;
    result_t(vertex_t* _topo_order) : topo_order(_topo_order) {}
};

template <typename graph_t, typename param_type, typename result_type>
struct problem_t : gunrock::problem_t<graph_t> {
    param_type param;
    result_type result;

    problem_t(graph_t & G,
            param_type& _param,
            result_type& _result,
            std::shared_ptr<gcuda::multi_context_t> _context)
        : gunrock::problem_t<graph_t>(G, _context),
        param(_param), result(_result) {}
    
    using vertex_t = typename graph_t::vertex_type;
    using edge_t = typename graph_t::edge_type;
    using weight_t = typename graph_t::weight_type;

    thrust::device_vector<vertex_t> in_degree;
    thrust::device_vector<vertex_t> accu_degree;
    thrust::device_vector<vertex_t> edge_dst;

    void init() override {
        auto g = this->get_graph();
        auto n_vertices = g.get_number_of_vertices();
        auto n_edges = g.get_number_of_edges();
        in_degree.resize(n_vertices);
        accu_degree.resize(n_vertices);
        edge_dst.resize(n_edges);
    }

    void reset() override {
        auto g = this->get_graph();
        auto n_vertices = g.get_number_of_vertices();
        auto n_edges = g.get_number_of_edges();
        auto context = this->get_single_context();
        auto policy = context->execution_policy();

        thrust::fill(policy, accu_degree.begin(), accu_degree.end(), 0);
        auto d_topo_order = thrust::device_pointer_cast(this->result.topo_order);
        
        thrust::fill_n(policy, in_degree.begin(), n_vertices, 0);


        auto get_dst = [=] __device__(const int& e) -> vertex_t{
            return g.get_destination_vertex(e);
        };

        thrust::transform(policy, thrust::counting_iterator<vertex_t>(0),
                        thrust::counting_iterator<vertex_t>(n_edges),
                        edge_dst.begin(), get_dst);
        
        thrust::sort(edge_dst.begin(), edge_dst.end());

        thrust::device_vector<vertex_t> output_keys(n_vertices);

        auto new_end = thrust::reduce_by_key(policy, 
                                            edge_dst.begin(), 
                                            edge_dst.end(), 
                                            thrust::make_constant_iterator(1), 
                                            output_keys.begin(),
                                            in_degree.begin(),
                                            thrust::equal_to<int>(),
                                            thrust::plus<int>()
        );
        
        auto device_max = std::numeric_limits<vertex_t>::max();
        auto set_order = [=] __device__ __host__(const int& i){
            return in_degree[i] == 0 ? 0 : device_max;
        } ;
        thrust::transform(policy, thrust::counting_iterator<vertex_t>(0),
                        thrust::counting_iterator<vertex_t>(n_edges),
                        d_topo_order, set_order);
    }
};

template <typename problem_t>
struct enactor_t : gunrock::enactor_t<problem_t> {
    enactor_t(problem_t* _problem,
            std::shared_ptr<gcuda::multi_context_t> _context)
        : gunrock::enactor_t<problem_t>(_problem, _context) {}
    
    using vertex_t = typename problem_t::vertex_t;
    using edge_t = typename problem_t::edge_t;
    using weight_t = typename problem_t::weight_t;
    using frontier_t = typename enactor_t<problem_t>::frontier_t;

    void prepare_frontier(frontier_t* f,
                        gcuda::multi_context_t& context) override {
        auto P = this->get_problem();
        auto in_degree = P->in_degree;
        auto G = P->get_graph();
        auto n_vertices = G.get_number_of_vertices();
        for(auto vid = 0; vid < n_vertices; vid++){
            if(in_degree[vid] == 0){
                f->push_back(vid);
            }
        }
    }
    void loop(gcuda::multi_context_t& context) override {
        auto E = this->get_enactor();
        auto P = this->get_problem();
        auto G = P->get_graph();
        
        auto topo_order = P->result.topo_order;
        auto in_degree = P->in_degree.data().get();
        auto accu_degree = P->accu_degree.data().get();
        auto num_origin_vertices = P->param.num_origin_vertices;

        auto update = [topo_order, in_degree, accu_degree, num_origin_vertices] 
                        __host__ __device__(
                            vertex_t const& source,
                            vertex_t const& neighbor,
                            edge_t const& edge,
                            weight_t const& weight
                        ) -> bool {
            math::atomic::add(&accu_degree[neighbor], 1);
            if(accu_degree[neighbor] == in_degree[neighbor]){
                math::atomic::min(&topo_order[neighbor], 
                neighbor < num_origin_vertices ? topo_order[source]+1 : topo_order[source]
                );
                return true;
            }else{
                return false;
            }
        };

        operators::advance::execute<operators::load_balance_t::block_mapped>(
            G, E, update, context);
    }
};

template <typename graph_t>
float run(graph_t& G,
        typename graph_t::vertex_type& num_origin_vertices,
        typename graph_t::vertex_type* topo_order,
        std::shared_ptr<gcuda::multi_context_t> context =
              std::shared_ptr<gcuda::multi_context_t>(
                  new gcuda::multi_context_t(0))
)   {
    using vertex_t = typename graph_t::vertex_type;
    using param_type = param_t<vertex_t>;
    using result_type = result_t<vertex_t>;

    param_type param(num_origin_vertices);
    result_type result(topo_order);

    using problem_type = problem_t<graph_t, param_type, result_type>;
    using enactor_type = enactor_t<problem_type>;

    problem_type problem(G, param, result, context);
    problem.init();
    problem.reset();

    enactor_type enactor(&problem, context);
    return enactor.enact();
}

}

}