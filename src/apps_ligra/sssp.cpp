#include <deps/ligra/ligra/ligra.h>
#include <core/io.hpp>

struct BF_Relax_F {
  intE* ShortestPathLen;
  int* Visited;
  intE vcnt;
  BF_Relax_F(intE* _ShortestPathLen, int* _Visited, intE _vcnt) : 
    ShortestPathLen(_ShortestPathLen), Visited(_Visited), vcnt(_vcnt) {}
  inline bool update (uintE s, uintE d) { //Update ShortestPathLen if found a shorter path
    if (d >= vcnt) {
      intE newDist = ShortestPathLen[s];
      if (ShortestPathLen[d] > newDist) {
        ShortestPathLen[d] = newDist;
        if(Visited[d] == 0) { Visited[d] = 1 ; return 1;}
      }
    }
    else {
      intE newDist = ShortestPathLen[s] + 1;
      if(ShortestPathLen[d] > newDist) {
        ShortestPathLen[d] = newDist;
        if(Visited[d] == 0) { Visited[d] = 1 ; return 1;}
      }
    }
    return 0;
  }

  inline bool updateAtomic (uintE s, uintE d){ //atomic Update
    if (d >= vcnt) {
      intE newDist = ShortestPathLen[s];
      return (writeMin(&ShortestPathLen[d],newDist) &&
        CAS(&Visited[d],0,1));
    }
    else {
      intE newDist = ShortestPathLen[s] + 1;
      return (writeMin(&ShortestPathLen[d],newDist) &&
        CAS(&Visited[d],0,1));
    }
  }
  inline bool cond (uintE d) { return cond_true(d); }
};

//reset visited elements
struct BF_Reset_F {
  int* Visited;
  BF_Reset_F(int* _Visited) : Visited(_Visited) {}
  inline bool operator() (uintE i){
    Visited[i] = 0;
    return 1;
  }
};

template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
    long start = P.getOptionLongValue("-r",1);
    std::string info_path = P.getOptionValue("-i");
    std::vector<intE> info;
    intE cnt = read_binary2vector(info_path, info);
    intE vcnt = info[0];
    long n = GA.n;
    //initialize ShortestPathLen to "infinity"
    intE* ShortestPathLen = newA(intE,n);
    {parallel_for(long i=0;i<n;i++) ShortestPathLen[i] = INT_MAX/2;}
    ShortestPathLen[start] = 0;

    int* Visited = newA(int, n);
    {parallel_for(long i=0;i<n;i++) Visited[i] = 0;}

    vertexSubset Frontier(n,start); //initial frontier

    long round = 0;
    while(!Frontier.isEmpty()){
        if(round == n-1) {
        //negative weight cycle
        {parallel_for(long i=0;i<n;i++) ShortestPathLen[i] = -(INT_E_MAX/2);}
        break;
        }
        vertexSubset output = edgeMap(GA, Frontier, BF_Relax_F(ShortestPathLen,Visited,vcnt), n);
        vertexMap(output,BF_Reset_F(Visited));
        Frontier.del();
        Frontier = output;
        round++;
    }
    Frontier.del(); free(Visited);
    free(ShortestPathLen);
}