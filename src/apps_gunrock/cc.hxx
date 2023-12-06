#pragma once

#include <gunrock/algorithms/algorithms.hxx>

namespace gunrock{
namespace cc{

template <typename vertex_t>
struct param_t{
    vertex_t place;
    param_t(vertex_t _place) : place(_place){}
};


template <typename vertex_t>
struct result_t {
    vertex_t *component_ids;
    result_t(vertex_t* _component_ids) 
    : component_ids(_component_ids) {}
};

template <typename graph_t, typename param_type, typename result_type>
struct problem_t : gunrock::problem_t<graph_t>{
    param_type param;
    result_type result;

    problem_t(graph_t& G,
            param_type& _param,
            result_type& _result,
            std::shared_ptr<gcuda::multi_context_t> _context)
      : gunrock::problem_t<graph_t>(G, _context),
        param(_param),
        result(_result) {}
    
    
    using vertex_t = typename graph_t::vertex_type;
    using edge_t = typename graph_t::edge_type;
    using weight_t = typename graph_t::weight_type;

    thrust::device_vector<vertex_t> prevIDs;

    void init() override {
        auto g = this->get_graph();
        auto n_vertices = g.get_number_of_vertices();
        prevIDs.resize(n_vertices);
        auto policy = this->context->get_context(0)->execution_policy();
        thrust::sequence(policy, prevIDs.begin(), prevIDs.end());
        auto d_component_ids = thrust::device_pointer_cast(this->result.component_ids);
        thrust::sequence(policy, d_component_ids+0, d_component_ids+n_vertices);
    }

    void reset() override {
        auto g = this->get_graph();
        auto n_vertices = g.get_number_of_vertices();

        auto context = this->get_single_context();
        auto policy = context->execution_policy();
        thrust::sequence(policy, prevIDs.begin(), prevIDs.end());
        auto d_component_ids = thrust::device_pointer_cast(this->result.component_ids);
        thrust::sequence(policy, d_component_ids+0, d_component_ids+n_vertices);
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

    void loop(gcuda::multi_context_t& context) override {
        // Data slice
        auto E = this->get_enactor();
        auto P = this->get_problem();
        auto G = P->get_graph();

        auto n_vertices = G.get_number_of_vertices();
        auto prevIDs = P->prevIDs.data().get();
        auto IDs = P->result.component_ids;

        auto coo_op = [=] __device__(
            edge_t const& e
        ) -> void {
            auto src = G.get_source_vertex(e);
            auto dst = G.get_destination_vertex(e);
            math::atomic::min(&IDs[dst], prevIDs[src]);
        };

        operators::parallel_for::execute<operators::parallel_for_each_t::edge>(
            G, coo_op, context
        );

        auto policy = this->context->get_context(0)->execution_policy();
        thrust::copy_n(policy, IDs, n_vertices, prevIDs);
    }

    virtual bool is_converged(gcuda::multi_context_t &context){
        if(this->iteration == 0) return false;
        auto P = this->get_problem();
        auto G = P->get_graph();
        
        auto n_vertices = G.get_number_of_vertices();
        auto policy = this->context->get_context(0)->execution_policy();

        auto d_component_ids = thrust::device_pointer_cast(P->result.component_ids);

        bool is_equal = thrust::equal(P->prevIDs.begin(), P->prevIDs.end(), d_component_ids);
        return is_equal;
    }
};

template <typename graph_t>
float run(graph_t &G,
        typename graph_t::vertex_type* component_ids,
        std::shared_ptr<gcuda::multi_context_t> context =
              std::shared_ptr<gcuda::multi_context_t>(
                  new gcuda::multi_context_t(0))
) {
    using vertex_t = typename graph_t::vertex_type;
    using param_type = param_t<vertex_t>;
    using result_type = result_t<vertex_t>;

    vertex_t place = 0;
    param_type param(place);
    result_type result(component_ids);

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
