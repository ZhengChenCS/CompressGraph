#include <deps/ligra/ligra/ligra.h>
#include "math.h"
#include <core/io.hpp>

template <class vertex>
struct AUTH_F{
    double* hub_curr;
    double* auth_next;
    intE vcnt;
    AUTH_F(double* _hub_curr, double *_auth_next):
    hub_curr(_hub_curr), auth_next(_auth_next){}
    inline bool update(uintE s, uintE d){
        auth_next[s] += hub_curr[d];
        return 1;
    }
    inline bool updateAtomic (uintE s, uintE d){
        writeAdd(&auth_next[s], hub_curr[d]);
        return 1;
    }
    inline bool cond (intT d) { return cond_true(d); }
};

template <class vertex>
struct HUB_F{
    double* auth_curr;
    double* hub_next;
    HUB_F(double* _auth_curr, double *_hub_next):
    auth_curr(_auth_curr), hub_next(_hub_next){}
    inline bool update(uintE s, uintE d){
        hub_next[d] += auth_curr[s];
        return 1;
    }
    inline bool updateAtomic(uintE s, uintE d){
        writeAdd(&hub_next[d], auth_curr[s]);
        return 1;
    }
    inline bool cond (intT d) { return cond_true(d); }
};


template <class vertex>
void Compute(graph<vertex> &GA, commandLine P) {
    long maxIters = P.getOptionLongValue("-maxiters", 100);
    std::string info_path = P.getOptionValue("-i");
    std::string order_path = P.getOptionValue("-o");
    std::vector<intE> info;
    std::vector<intE> order;
    int cnt = read_binary2vector(info_path, info);
    cnt = read_binary2vector(order_path, order);
    const intE n = GA.n;
    const intE vcnt = info[0];
    const intE rcnt = info[1];

    double* auth_curr = newA(double, n);
    double* hub_curr = newA(double, n);
    double* auth_next = newA(double, n);
    double* hub_next = newA(double, n);

    parallel_for(long i=0; i<vcnt; i++){ auth_curr[i]=1; hub_curr[i]=1; }
    parallel_for(long i=vcnt; i<n; i++){ auth_curr[i]=0; hub_curr[i]=0; }
    parallel_for(long i=0; i<n; i++) { auth_next[i]=0; hub_next[i]=0; }
    
    bool* v_frontier = newA(bool, n);
    parallel_for(long i=0;i<vcnt;i++){ v_frontier[i]=1; }
    parallel_for(long i=vcnt;i<n;i++){ v_frontier[i]=0; }
    intE max_step = 0;
    for(long i=0; i<rcnt; i++){ max_step = std::max(max_step, order[i]); }
    std::vector<bool*> rule_frontier(max_step+1);
    std::vector<intE> active_count(max_step+1, 0);
    parallel_for(intE i=0; i <= max_step; i++){ rule_frontier[i] = newA(bool, n); }
    parallel_for(intE i = 0; i < rcnt; i++){
        for(intE j = 0; j <= max_step; j++){
        if(order[i] == j) { rule_frontier[j][i+vcnt] = 1; writeAdd(&active_count[j], 1); }
        else{ rule_frontier[j][i+vcnt] = 0; }
        }
    }
    long iter = 0;
    vertexSubset vertexFrontier(n, vcnt, v_frontier);
    std::vector<vertexSubset*> ruleFrontier;
    for(intE i = 0; i <= max_step; i++){
        ruleFrontier.emplace_back(new vertexSubset(n, active_count[i], rule_frontier[i]));
    }

    while(iter++ < maxIters){
        edgeMap(GA, vertexFrontier, HUB_F<vertex>(auth_curr, hub_next), 0, no_output);
        for(intE step = 0; step <= max_step; step++){
            edgeMap(GA, *ruleFrontier[step], HUB_F<vertex>(hub_next, hub_next), 0, no_output);
        }
        for(intE step=max_step; step >= 0; step--){
            edgeMap(GA, *ruleFrontier[step], AUTH_F<vertex>(hub_curr, hub_curr), 0, no_output);
        }
        edgeMap(GA, vertexFrontier, AUTH_F<vertex>(hub_curr, auth_next), 0, no_output);
        double auth_norm = 0;
        double hub_norm = 0;
        parallel_for(intE i = 0; i < vcnt; i++){
            double accu_auth = auth_next[i]*auth_next[i];
            double accu_hub = hub_next[i]*hub_next[i];
            writeAdd(&auth_norm, accu_auth);
            writeAdd(&hub_norm, accu_hub);
        }
        parallel_for(intE i = 0; i < vcnt; i++){
            auth_curr[i] = auth_next[i] / auth_norm;
            auth_next[i] = 0.0;
            hub_curr[i] = hub_next[i] / hub_norm;
            hub_next[i] = 0.0;
        }
        parallel_for(intE i=vcnt; i<n; i++){
            auth_curr[i] = 0.0;
            auth_next[i] = 0.0;
            hub_curr[i] = 0.0;
            hub_next[i] = 0.0;
        }
    }
    vertexFrontier.del();
    for(intE step = 0; step <= max_step; step++){ ruleFrontier[step]->del(); }
    free(auth_curr);
    free(hub_curr);
    free(auth_next);
    free(hub_next);
}