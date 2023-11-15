#include <deps/ligra/ligra/ligra.h>
#include "math.h"
#include <core/io.hpp>

template <class vertex>
struct PR_F {
  double* p_curr, *p_next;
  intE *origin_out_degree;
  PR_F(double* _p_curr, double* _p_next, intE *_origin_out_degree) : 
    p_curr(_p_curr), p_next(_p_next), origin_out_degree(_origin_out_degree) {}
  inline bool update(uintE s, uintE d){ //update function applies PageRank equation
    p_next[d] += p_curr[s]/origin_out_degree[s];
    return 1;
  }
  inline bool updateAtomic (uintE s, uintE d) { //atomic Update
    writeAdd(&p_next[d],p_curr[s]/origin_out_degree[s]);
    return 1;
  }
  inline bool cond (intT d) { return cond_true(d); }
};

template <class vertex>
struct PR_Rule_F {
  double* p_next;
  PR_Rule_F(double* _p_next) : p_next(_p_next) {}
  inline bool update(uintE s, uintE d) {
      p_next[d] += p_next[s];
      return 0;
  }
  inline bool updateAtomic (uintE s, uintE d) { //atomic Update
      writeAdd(&p_next[d],p_next[s]);
      return 0;
  }
  inline bool cond (intT d) { return cond_true(d); }
};

//vertex map function to update its p value according to PageRank equation
struct PR_Vertex_F {
  double damping;
  double addedConstant;
  double* p_curr;
  double* p_next;
  PR_Vertex_F(double* _p_curr, double* _p_next, double _damping, intE n) :
    p_curr(_p_curr), p_next(_p_next), 
    damping(_damping), addedConstant((1-_damping)*(1/(double)n)){}
  inline bool operator () (uintE i) {
    p_next[i] = damping*p_next[i] + addedConstant;
    return 1;
  }
};

//resets p
struct PR_Vertex_Reset {
  double* p_curr;
  PR_Vertex_Reset(double* _p_curr) :
    p_curr(_p_curr) {}
  inline bool operator () (uintE i) {
    p_curr[i] = 0.0;
    return 1;
  }
};

template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
  long maxIters = P.getOptionLongValue("-maxiters",100);
  std::string info_path = P.getOptionValue("-i");
  std::string degree_path = P.getOptionValue("-d");
  std::string order_path = P.getOptionValue("-o");
  std::vector<intE> info;
  std::vector<intE> degree;
  std::vector<intE> order;
  intE cnt = read_binary2vector(info_path, info);
  cnt = read_binary2vector(degree_path, degree);
  cnt = read_binary2vector(order_path, order);
  const intE n = GA.n;
  const intE vcnt = info[0];
  const intE rcnt = info[1];
  const double damping = 0.85, epsilon = 0.0000001;
  
  double one_over_n = 1/(double)vcnt;
  double* p_curr = newA(double,n);
  {parallel_for(long i=0;i<vcnt;i++) p_curr[i] = one_over_n;
   parallel_for(long i=vcnt;i<n;i++) p_curr[i] = 0;}
  double* p_next = newA(double,n);
  {parallel_for(long i=0;i<n;i++) p_next[i] = 0;} //0 if unchanged
  bool* frontier = newA(bool,n);
  bool* full_frontier = newA(bool, n);
  {parallel_for(long i=0;i<vcnt;i++) frontier[i] = 1;}
  {parallel_for(long i=vcnt;i<n;i++) frontier[i] = 0;}
  {parallel_for(long i=0;i<n;i++) full_frontier[i] = 1;}
  intE max_step = 0;
  for(long i = 0; i < rcnt; i++){ max_step = std::max(max_step, order[i]); }
  std::vector<bool*> rule_frontier(max_step+1);
  std::vector<intE> active_count(max_step+1, 0);
  parallel_for(intE i = 0; i <= max_step; i++) { rule_frontier[i] = newA(bool, n); }
  parallel_for(intE i = 0; i < vcnt; i++){
    parallel_for(intE j = 0; j <= max_step; j++) { rule_frontier[j][i] = 0; }
  }
  parallel_for(intE i = 0; i < rcnt; i++){
    for(intE j = 0; j <= max_step; j++){
      if(order[i] == j) { rule_frontier[j][i+vcnt] = 1; writeAdd(&active_count[j], 1); }
      else{ rule_frontier[j][i+vcnt] = 0; }
    }
  }
  long iter = 0;
  vertexSubset vertexFrontier(n, vcnt, frontier);
  vertexSubset fullFrontier(n, n, full_frontier);
  std::vector<vertexSubset*> ruleFrontier;
  for(intE i = 0; i <= max_step; i++){
    ruleFrontier.emplace_back(new vertexSubset(n, active_count[i], rule_frontier[i]));
  }

  while(iter++ < maxIters) {
    edgeMap(GA, vertexFrontier, PR_F<vertex>(p_curr, p_next, &degree[0]), 0, no_output);
    for(intE step = 0; step <= max_step; step++){
      edgeMap(GA, *ruleFrontier[step], PR_Rule_F<vertex>(p_next), 0, no_output);
    }
    vertexMap(vertexFrontier,PR_Vertex_F(p_curr,p_next,damping,n));
    //compute L1-norm between p_curr and p_next
    {parallel_for(long i=0;i<vcnt;i++) {
      p_curr[i] = fabs(p_curr[i]-p_next[i]);
      }
     parallel_for(long i=vcnt;i<n;i++) {
      p_curr[i] = 0;
      }}
    double L1_norm = sequence::plusReduce(p_curr,n);
    if(L1_norm < epsilon) break;
    //reset p_curr
    vertexMap(fullFrontier,PR_Vertex_Reset(p_curr));
    swap(p_curr,p_next);
  }
  
  fullFrontier.del();
  vertexFrontier.del();
  for(intE step = 0; step <= max_step; step++){ ruleFrontier[step]->del(); }
  free(p_curr); 
  free(p_next); 
}