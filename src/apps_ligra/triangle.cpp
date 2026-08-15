#include "deps/ligra/ligra/ligra.h"
#include <atomic>

template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
  long n = GA.n;
  std::atomic<long> triangle_count(0);

  parallel_for (long u = 0; u < n; u++) {
    if (GA.V[u].getOutDegree() == 0) continue;

    bool* u_neighbors_set = newA(bool, n);
    parallel_for(long i=0; i<n; i++) u_neighbors_set[i] = 0;

    struct FillSet_F {
      bool* set;
      FillSet_F(bool* _set) : set(_set) {}
      inline bool update (uintE s, uintE d) { set[d] = 1; return true; }
      inline bool updateAtomic (uintE s, uintE d) { set[d] = 1; return true; }
      inline bool cond(uintE d) { return true; }
    };

    vertexSubset u_frontier(n, u);
    edgeMap(GA, u_frontier, FillSet_F(u_neighbors_set), n, false);

    struct Intersect_F {
      long u;
      long n;
      bool* u_set;
      std::atomic<long>& count;
      graph<vertex>& GA; 

      Intersect_F(long _u, long _n, bool* _set, std::atomic<long>& _c, graph<vertex>& _GA)
        : u(_u), n(_n), u_set(_set), count(_c), GA(_GA) {} 

      inline bool update(uintE s, uintE v) {
        if (v > u) { 

          struct Check_Ngh_F {
            long v;
            bool* u_set;
            std::atomic<long>& count;

            Check_Ngh_F(long _v, bool* _set, std::atomic<long>& _c)
              : v(_v), u_set(_set), count(_c) {}

            inline bool update(uintE s, uintE w) {
              if (u_set[w]) { 
                count.fetch_add(1);
              }
              return true;
            }
            inline bool updateAtomic(uintE s, uintE w) { return update(s,w); }
            inline bool cond(uintE w) { return (w > v); }
          };

          vertexSubset v_frontier(n, v); 
          edgeMap(GA, v_frontier, Check_Ngh_F(v, u_set, count), n, false);
        }
        return true;
      }
      inline bool updateAtomic(uintE s, uintE v) { return update(s,v); }
      inline bool cond(uintE v) { return true; } 
    };

    edgeMap(GA, u_frontier, Intersect_F(u, n, u_neighbors_set, triangle_count, GA), n, false);

    u_frontier.del();
    free(u_neighbors_set);
  }

  // We don't need the buggy timer, Ligra prints its own.
  cout << "Triangle Count: " << triangle_count.load() << endl;
}
