#include <deps/ligra/ligra/ligra.h>
#include <core/io.hpp>

struct TOPO_F {
  int* in_degree;
  int* accu_degree;
  int* topo_order;
  intE vcnt;
  TOPO_F(int* _in_degree, int* _accu_degree, int* _topo_order, intE _vcnt) : 
    in_degree(_in_degree), accu_degree(_accu_degree), topo_order(_topo_order), vcnt(_vcnt) {}
  inline bool update (uintE s, uintE d) { //Update
    accu_degree[d]++;
    if(accu_degree[d] == in_degree[d]){
      if(s < vcnt){ topo_order[d] = std::min(topo_order[d], topo_order[s]+1); }
      else{ topo_order[d] = std::min(topo_order[d], topo_order[s]); }
      return 1;
    }else{
      return 0;
    }
  }
  inline bool updateAtomic (uintE s, uintE d){ //atomic version of Update
    writeAdd(&accu_degree[d], 1);
    if(accu_degree[d] == in_degree[d]){
      if(s < vcnt) {writeMin(&topo_order[d], topo_order[s]+1); }
      else{ writeMin(&topo_order[d], topo_order[s]); }
      return 1;
    }else{
      return 0;
    }
  }
  inline bool cond (uintE d) {  return topo_order[d] == UINT_E_MAX; } 
};

template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
    long start = 0;
    long n = GA.n;  // vertex number
    std::string info_path = P.getOptionValue("-i");
    std::vector<intE> info;
    intE cnt = read_binary2vector(info_path, info);
    long vcnt = info[0];

    int count = 0;
    int* in_degree = newA(int,n);
    int* accu_degree = newA(int,n);
    bool* frontier = newA(bool,n);
    int* topo_order = newA(int, n);
    parallel_for(long i=0; i<n; i++){ 
      topo_order[i] = UINT_E_MAX; 
      frontier[i] = 0;
      accu_degree[i] = 0;
      in_degree[i] = GA.V[i].getInDegree();
    }
    intE active_count = 0;
    parallel_for(long i=0; i<n; i++){
      if(in_degree[i] == 0){
        topo_order[i] = 0;
        frontier[i] = 1;
        writeAdd(&active_count, 1);
      }
    }
    vertexSubset Frontier(n, active_count, frontier); ////creates initial frontier
    while(!Frontier.isEmpty()){ //loop until frontier is empty
      vertexSubset output = edgeMap(GA, Frontier, TOPO_F(in_degree, accu_degree, topo_order, vcnt));
      Frontier.del();
      Frontier = output;
    }
    Frontier.del();
    free(in_degree);
    free(accu_degree);
    free(topo_order);
}