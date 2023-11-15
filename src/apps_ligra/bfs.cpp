#include <deps/ligra/ligra/ligra.h>

struct BFS_F {
  uintE* Parents;
  BFS_F(uintE* _Parents) : Parents(_Parents) {}
  inline bool update (uintE s, uintE d) { //Update
    if(Parents[d] == UINT_E_MAX) { Parents[d] = s; return 1; }
    else return 0;
  }
  inline bool updateAtomic (uintE s, uintE d){ //atomic version of Update
    return (CAS(&Parents[d],UINT_E_MAX,s));
  }
  //cond function checks if vertex has been visited yet
  inline bool cond (uintE d) { return (Parents[d] == UINT_E_MAX); } 
};

template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
    long start = P.getOptionLongValue("-r",1);
    long n = GA.n;  // vertex number
    //creates Parents array, initialized to all -1, except for start
    uintE* Parents = newA(uintE,n);
    parallel_for(long i=0;i<n;i++) {Parents[i] = UINT_E_MAX;}
    Parents[start] = start;
    vertexSubset Frontier(n,start); //creates initial frontier
    while(!Frontier.isEmpty()){ //loop until frontier is empty
      vertexSubset output = edgeMap(GA, Frontier, BFS_F(Parents), n);    
      Frontier.del();
      Frontier = output; //set new frontier
    } 
    Frontier.del();
    free(Parents); 
}

