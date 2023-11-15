#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <ctime>
#include <iostream>
#include <string>
#include <vector>

#include "core/compression/basics.h"
#include "core/compression/hash.h"
#include "core/compression/heap.h"
#include "core/compression/records.h"
#include "core/io.hpp"
#include "core/util.hpp"

float factor = 0.50; // 1/extra space overhead; set closer to 1 for smaller and
                     // slower execution
int minsize = 256; // to avoid many reallocs at small sizes, should be ok as is

int u; // |text| and later current |C| with gaps

int *C; // compressed text

int c; // real |C|

int alph; // max used terminal symbol

int n; // |R|

Tlist *L; // |L| = c;

Thash Hash; // hash table of pairs

Theap Heap; // special heap of pairs

Trarray Rec; // records

void prepare(std::vector<int> &inter) {
    int i, id;
    int len = inter.size();
    Tpair pair;
    c = u = len;
    /*alph = 0;
    for (i = 0; i < u; i++) {
        if (C[i] > alph) alph = C[i];
    }*/
    // n = vlist.size()-1;
    n = alph;
    Rec = createRecords(factor, minsize);
    Heap = createHeap(u, &Rec, factor, minsize);
    Hash = createHash(256 * 256, &Rec);
    L = (Tlist *)malloc(u * sizeof(Tlist));
    assocRecords(&Rec, &Hash, &Heap, L);
    for (i = 0; i < len - 1; i++) {
#ifdef DEBUG
        if (i % 1000000 == 0) {
            printf("process len: %d\n", i);
        }
#endif
        pair.left = inter[i];
        pair.right = inter[i + 1];
        id = searchHash(Hash, pair);
        if (id == -1) {
            id = insertRecord(&Rec, pair);
            L[i].next = -1;
        } else {
            L[i].next = Rec.records[id].cpos;
            L[L[i].next].prev = i;
            incFreq(&Heap, id);
        }
        L[i].prev = -id - 1;
        Rec.records[id].cpos = i;
    }
    purgeHeap(&Heap);
}

void repair(std::vector<int> &inter, std::vector<Tpair> &rule) {
    int oid, id, cpos;
    Trecord *rec, *orec;
    Tpair pair;
    while (n + 1 > 0) {
        oid = extractMax(&Heap);
        if (oid == -1)
            break; // the end!!
        orec = &Rec.records[oid];
        cpos = orec->cpos; // first position in C
        rule.push_back(orec->pair);
#ifdef DEBUG
        if (rule.size() % 100000 == 0) {
            fprintf(stderr, "Rule size: %u\n", (uint)rule.size());
        }
#endif
        while (cpos != -1) {

            int ant, sgte, ssgte;
            // replacing bc->e in abcd, b = cpos, c = sgte, d = ssgte
            if (inter[cpos + 1] < 0)
                sgte = -inter[cpos + 1] - 1;
            else
                sgte = cpos + 1;
            if ((sgte + 1 < u) && (inter[sgte + 1] < 0))
                ssgte = -inter[sgte + 1] - 1;
            else
                ssgte = sgte + 1;
            // remove bc from L
            if (L[cpos].next != -1)
                L[L[cpos].next].prev = -oid - 1;
            orec->cpos = L[cpos].next;
            if (ssgte != u) // there is ssgte
            {               // remove occ of cd
                pair.left = inter[sgte];
                pair.right = inter[ssgte];
                id = searchHash(Hash, pair);
                if (id != -1) // may not exist if purgeHeap'd
                {
                    if (id != oid)
                        decFreq(&Heap, id);       // not to my pair!
                    if (L[sgte].prev != NullFreq) // still exists(not removed)
                    {
                        rec = &Rec.records[id];
                        if (L[sgte].prev < 0) // this cd is head of its list
                            rec->cpos = L[sgte].next;
                        else
                            L[L[sgte].prev].next = L[sgte].next;
                        if (L[sgte].next != -1) // not tail of its list
                            L[L[sgte].next].prev = L[sgte].prev;
                    }
                }
                // create occ of ed
                pair.left = n;
                id = searchHash(Hash, pair);
                if (id == -1) // new pair, insert
                {
                    id = insertRecord(&Rec, pair);
                    rec = &Rec.records[id];
                    L[cpos].next = -1;
                } else {
                    incFreq(&Heap, id);
                    rec = &Rec.records[id];
                    L[cpos].next = rec->cpos;
                    L[L[cpos].next].prev = cpos;
                }
                L[cpos].prev = -id - 1;
                rec->cpos = cpos;
            }
            if (cpos != 0) // there is ant
            {              // remove occ of ab
                if (inter[cpos - 1] < 0) {
                    ant = -inter[cpos - 1] - 1;
                    if (ant == cpos) // sgte and ant clashed -> 1 hole
                        ant = cpos - 2;
                } else
                    ant = cpos - 1;
                pair.left = inter[ant];
                pair.right = inter[cpos];
                id = searchHash(Hash, pair);
                if (id != -1) // may not exist if purgeHeap'd
                {
                    if (id != oid)
                        decFreq(&Heap, id);      // not to my pair!
                    if (L[ant].prev != NullFreq) // still exists (not removed)
                    {
                        rec = &Rec.records[id];
                        if (L[ant].prev < 0) // this ab is head of its list
                            rec->cpos = L[ant].next;
                        else
                            L[L[ant].prev].next = L[ant].next;
                        if (L[ant].next != -1) // it is not tail of its list
                            L[L[ant].next].prev = L[ant].prev;
                    }
                }
                // create occ of ae
                pair.right = n;
                id = searchHash(Hash, pair);
                if (id == -1) // new pair, insert
                {
                    id = insertRecord(&Rec, pair);
                    rec = &Rec.records[id];
                    L[ant].next = -1;
                } else {
                    incFreq(&Heap, id);
                    rec = &Rec.records[id];
                    L[ant].next = rec->cpos;
                    L[L[ant].next].prev = ant;
                }
                L[ant].prev = -id - 1;
                rec->cpos = ant;
            }
            inter[cpos] = n;
            if (ssgte != u)
                inter[ssgte - 1] = -cpos - 1;
            inter[cpos + 1] = -ssgte - 1;
            c--;
            orec = &Rec.records[oid]; // just in case of Rec.records realloc'd
            cpos = orec->cpos;
        }
        removeRecord(&Rec, oid);
        n++;
        purgeHeap(&Heap);   // remove freq 1 from heap
        if (c < factor * u) // compact C
        {
            int i, ni;
            i = 0;
            for (ni = 0; ni < c - 1; ni++) {
                inter[ni] = inter[i];
                L[ni] = L[i];
                if (L[ni].prev < 0) {
                    if (L[ni].prev != NullFreq) // real ptr
                        Rec.records[-L[ni].prev - 1].cpos = ni;
                } else
                    L[L[ni].prev].next = ni;
                if (L[ni].next != -1)
                    L[L[ni].next].prev = ni;
                i++;
                if (inter[i] < 0)
                    i = -inter[i] - 1;
            }
            inter[ni] = inter[i];
            u = c;
            // C = (int *)realloc(C, c * sizeof(int));
            inter.resize(c);
            L = (Tlist *)realloc(L, c * sizeof(Tlist));
            assocRecords(&Rec, &Hash, &Heap, L);
        }
    }
}

inline int convert(int ID, int vertex_cnt) {
    return ID > vertex_cnt ? ID - vertex_cnt : ID;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: compress csr_vlist csr_elist\n");
        return 0;
    }
    std::string pvlist = argv[1];
    std::string pelist = argv[2];
    std::vector<int> csr_vlist;
    fprintf(stderr, "Load graph...\n");
    int v_cnt = read_binary2vector(pvlist, csr_vlist);
    std::vector<int> csr_elist;
    int e_cnt = read_binary2vector(pelist, csr_elist);
    std::cout << v_cnt << std::endl;
    std::cout << e_cnt << std::endl;
    fprintf(stderr, "compress start...\n");
    double start = timestamp();
    v_cnt -= 1;
    std::vector<int> inter;
    insert_spliter(csr_vlist, csr_elist, inter, v_cnt);
    std::cout << inter.size() << std::endl;
    alph = v_cnt * 2;
    prepare(inter);
    std::vector<Tpair> rule;
    repair(inter, rule);
    std::vector<int> out_vlist;
    std::vector<int> out_elist;
    out_vlist.emplace_back(0);
    int i = 0;
    int e_size = 0;
    while (i < u) {
        if (inter[i] < v_cnt) {
            out_elist.emplace_back(inter[i]);
            e_size++;
        } else {
            if (inter[i] < alph) {
                out_vlist.emplace_back(e_size);
            } else {
                out_elist.emplace_back(inter[i] - v_cnt);
                e_size++;
            }
        }
        i++;
        if (i < u && inter[i] < 0) {
            i = -inter[i] - 1;
        }
    }
    int left, right;
    for (auto p : rule) {
        left = convert(p.left, v_cnt);
        right = convert(p.right, v_cnt);
        out_elist.emplace_back(left);
        out_elist.emplace_back(right);
        e_size += 2;
        out_vlist.emplace_back(e_size);
    }
    int rule_cnt = out_vlist.size() - csr_vlist.size();
    double end = timestamp();
    fprintf(stderr, "Compression complete.\n");
    fprintf(stderr, "Compression time: %.4lf(s)\n", end - start);
    FILE *fvlist;
    FILE *felist;
    FILE *info;
    fvlist = fopen("csr_vlist.bin", "w");
    felist = fopen("csr_elist.bin", "w");
    fwrite(&out_vlist[0], sizeof(int), out_vlist.size(), fvlist);
    fwrite(&out_elist[0], sizeof(int), out_elist.size(), felist);
    info = fopen("info.bin", "w");
    fwrite(&v_cnt, sizeof(int), 1, info);
    fwrite(&rule_cnt, sizeof(int), 1, info);
    double cr = (double)(csr_vlist.size() + csr_elist.size()) /
                (out_vlist.size() + out_elist.size());
    fprintf(stderr, "Compression ratio : %.4lf\n", cr);
    return 0;
}
