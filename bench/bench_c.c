// C-side benchmark bodies for the CStyleDS containers. Compiled as C23 and
// linked against the C++ harness in main.cpp. See bench.h for contract.
#include "bench.h"

#include "CVec.h"
#include "CDeq.h"
#include "CPrioQ.h"
#include "CHMap.h"
#include "CHSet.h"
#include "CMat.h"

/* ============================ CVec ====================================== */

// add n items from a given array
long long cbench_cvec_build(const int* keys, size_t n) {
  CVec v; CVec_init(&v, int, 4);
  for (size_t i = 0; i < n; i++) { int k = keys[i]; CVec_push(&v, k); }
  long long s = 0;
  for (size_t i = 0; i < v.size; i++) s += CVec_at(&v, int, i);
  CVec_free(&v);
  return s;
}

// same with untimed heap alloc: reserve for use in other ops
void* cbench_cvec_make(const int* keys, size_t n) {
  CVec* v = (CVec*)malloc(sizeof(CVec));
  CVec_init(v, int, n ? n : 4);
  for (size_t i = 0; i < n; i++) { int k = keys[i]; CVec_push(v, k); }
  return v;
}

// access and sum elements in a filled vec
long long cbench_cvec_sum(void* h) {
  CVec* v = (CVec*)h;
  long long s = 0;
  for (size_t i = 0; i < v->size; i++) s += CVec_at(v, int, i);
  return s;
}

// same with random order access
long long cbench_cvec_index(void* h, const size_t* idx, size_t n) {
  CVec* v = (CVec*)h;
  long long s = 0;
  for (size_t i = 0; i < n; i++) s += CVec_at(v, int, idx[i]);
  return s;
}

void cbench_cvec_destroy(void* h) {
  CVec* v = (CVec*)h;
  CVec_free(v);
  free(v);
}

/* =================== emplace copy investigation ========================= */

typedef struct { int a, b; } BenchPair;   /*  8 bytes: copy likely elided     */
typedef struct { int v[16]; } BenchBig;   /* 64 bytes: copy likely visible */

// CVec_emplace(&v, T, a, b) == _CVec_push(&v, &(T){a, b}): compound literal + memcpy.
long long cbench_cvec_pair_emplace(const int* keys, size_t n) {
  CVec v; CVec_init(&v, BenchPair, 4);
  for (size_t i = 0; i < n; i++) CVec_emplace(&v, BenchPair, keys[i], keys[i] + 1);
  long long s = 0;
  for (size_t i = 0; i < v.size; i++) { BenchPair p = CVec_at(&v, BenchPair, i); s += p.a + p.b; }
  CVec_free(&v);
  return s;
}

// Named temp + CVec_push: same one-copy shape spelled out explicitly.
long long cbench_cvec_pair_push(const int* keys, size_t n) {
  CVec v; CVec_init(&v, BenchPair, 4);
  for (size_t i = 0; i < n; i++) { BenchPair p = { keys[i], keys[i] + 1 }; CVec_push(&v, p); }
  long long s = 0;
  for (size_t i = 0; i < v.size; i++) { BenchPair p = CVec_at(&v, BenchPair, i); s += p.a + p.b; }
  CVec_free(&v);
  return s;
}

long long cbench_cvec_big_emplace(const int* keys, size_t n) {
  CVec v; CVec_init(&v, BenchBig, 4);
  for (size_t i = 0; i < n; i++) CVec_emplAs(&v, BenchBig, .v[0] = keys[i]);
  long long s = 0;
  for (size_t i = 0; i < v.size; i++) s += CVec_at(&v, BenchBig, i).v[0];
  CVec_free(&v);
  return s;
}

long long cbench_cvec_big_push(const int* keys, size_t n) {
  CVec v; CVec_init(&v, BenchBig, 4);
  for (size_t i = 0; i < n; i++) { BenchBig b = { .v[0] = keys[i] }; CVec_push(&v, b); }
  long long s = 0;
  for (size_t i = 0; i < v.size; i++) s += CVec_at(&v, BenchBig, i).v[0];
  CVec_free(&v);
  return s;
}

/* ============================ CDeq ====================================== */

// add n items to back
long long cbench_cdeq_build(const int* keys, size_t n) {
  CDeq q; CDeq_init(&q, int, 4);
  for (size_t i = 0; i < n; i++) { int k = keys[i]; CDeq_pushBack(&q, k); }
  long long s = (long long)q.size;
  CDeq_free(&q);
  return s;
}

// add then pop on both sides
long long cbench_cdeq_mixed(const int* keys, size_t n) {
  CDeq q; CDeq_init(&q, int, 4);
  for (size_t i = 0; i < n; i++) {
    int k = keys[i];
    if (i & 1) CDeq_pushFront(&q, k);
    else CDeq_pushBack(&q, k);
  }
  long long s = 0;
  size_t toggle = 0;
  while (q.size != 0) {
    if (toggle & 1) s += CDeq_popFront(&q, int);
    else s += CDeq_popBack(&q, int);
    toggle++;
  }
  CDeq_free(&q);
  return s;
}

/* ============================ CPrioQ ==================================== */

// add n items then pop
long long cbench_cpq_sort(const int* keys, size_t n) {
  CPrioQ pq; CPrioQ_initPrim(&pq, int, 4, MAX);
  for (size_t i = 0; i < n; i++) { int k = keys[i]; CPrioQ_push(&pq, k); }
  long long s = 0;
  while (pq.size != 0) s += CPrioQ_pop(&pq, int);   
  CPrioQ_free(&pq);
  return s;
}

/* ============================ CHMap ===================================== */

// add n pairs taken from 2 given arrays
long long cbench_chmap_insert(const int* keys, const int* vals, size_t n) {
  CHMap m; CHMap_initPrim(&m, int, int, 8);
  for (size_t i = 0; i < n; i++) { 
    int k = keys[i], v = vals[i]; CHMap_put(&m, k, v); 
  }
  long long s = (long long)m.size;
  CHMap_free(&m);
  return s;
}

void* cbench_chmap_make(const int* keys, const int* vals, size_t n) {
  CHMap* m = (CHMap*)malloc(sizeof(CHMap));
  CHMap_initPrim(m, int, int, 8);
  for (size_t i = 0; i < n; i++) { 
    int k = keys[i], v = vals[i]; CHMap_put(m, k, v); 
  }
  return m;
}

// lookup n items
long long cbench_chmap_lookup(void* h, const int* keys, size_t n) {
  CHMap* m = (CHMap*)h;
  long long s = 0;
  for (size_t i = 0; i < n; i++) {
    int* p = CHMap_getp(m, keys[i]);
    if (p) s += *p;
  }
  return s;
}

void cbench_chmap_destroy(void* h) {
  CHMap* m = (CHMap*)h;
  CHMap_free(m);
  free(m);
}

/* ============================ CHSet ===================================== */

long long cbench_chset_insert(const int* keys, size_t n) {
  CHSet s; CHSet_initPrim(&s, int, 8);
  for (size_t i = 0; i < n; i++) { int k = keys[i]; CHSet_put(&s, k); }
  long long r = (long long)s.size;
  CHSet_free(&s);
  return r;
}

void* cbench_chset_make(const int* keys, size_t n) {
  CHSet* s = (CHSet*)malloc(sizeof(CHSet));
  CHSet_initPrim(s, int, 8);
  for (size_t i = 0; i < n; i++) { int k = keys[i]; CHSet_put(s, k); }
  return s;
}

long long cbench_chset_lookup(void* h, const int* keys, size_t n) {
  CHSet* s = (CHSet*)h;
  long long hits = 0;
  for (size_t i = 0; i < n; i++) if (CHSet_has(s, keys[i])) hits++;
  return hits;
}

void cbench_chset_destroy(void* h) {
  CHSet* s = (CHSet*)h;
  CHSet_free(s);
  free(s);
}

/* ============================ CMat ====================================== */

// jagged build 
// each row with same init cap 
// The C++ side mirrors this with reserve(cols) per row
long long cbench_cmat_build(size_t rows, size_t cols) {
  
  CMat m; CMat_init(&m, int, 1, (int)cols);
  m.rowLens[0] = 0;            
  for (size_t r = 1; r < rows; r++) { CMat_addRow(&m, (int)cols); }
  long long s = 0;
  for (size_t r = 0; r < rows; r++) {
    for (size_t c = 0; c < cols; c++) {
      int v = (int)(r + c);
      CMat_push(&m, (int)r, v);
      s += v;
    }
  }
  CMat_free(&m);
  return s;
}
