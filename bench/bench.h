#pragma once
// C-side benchmark entry points. These are compiled as C (bench_c.c, C23) and
// called from the C++ harness (main.cpp). The C data-structure headers cannot be
// compiled as C++ (they use _Generic, __typeof__ and compound literals), so the
// only thing that crosses the language boundary is this plain-C interface.
//
// Every "build/run" function returns a data-dependent checksum. The C++ harness
// (a) feeds it to a volatile sink so nothing is optimised away, and (b) compares
// it against the checksum of the equivalent STL run, which doubles as a
// correctness test of the C structures.
//
// Handle-based APIs (make/.../destroy) let the harness build a structure once,
// untimed, and then time only the operation of interest (lookup, iterate, ...).

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- CVec vs std::vector ------------------------------------------------ */
long long cbench_cvec_build(const int* keys, size_t n);        /* push n, sum  */
void* cbench_cvec_make(const int* keys, size_t n);
long long cbench_cvec_sum(void* h);                            /* iterate      */
long long cbench_cvec_index(void* h, const size_t* idx, size_t n); /* random   */
void cbench_cvec_destroy(void* h);

/* ---- emplace copy investigation: struct payloads ----------------------- */
/* CVec_emplace emulates object literal emplacement by materializing a temp lvalue 
 * then memcpy's it in (one copy, == C++ push_back(T{...})); These pit that against
 * std::vector emplace_back / push_back for an 8-byte and a 64-byte payload. */
long long cbench_cvec_pair_emplace(const int* keys, size_t n);
long long cbench_cvec_pair_push(const int* keys, size_t n);
long long cbench_cvec_big_emplace(const int* keys, size_t n);
long long cbench_cvec_big_push(const int* keys, size_t n);

/* ---- CDeq vs std::deque ------------------------------------------------- */
long long cbench_cdeq_build(const int* keys, size_t n);        /* push_back n  */
long long cbench_cdeq_mixed(const int* keys, size_t n);        /* alt ends+drain*/

/* ---- CPrioQ vs std::priority_queue ------------------------------------- */
long long cbench_cpq_sort(const int* keys, size_t n);          /* push n, drain*/

/* ---- CHMap vs std::unordered_map<int,int> ------------------------------ */
long long cbench_chmap_insert(const int* keys, const int* vals, size_t n);
void* cbench_chmap_make(const int* keys, const int* vals, size_t n);
long long cbench_chmap_lookup(void* h, const int* keys, size_t n);
void cbench_chmap_destroy(void* h);

/* ---- CHSet vs std::unordered_set<int> ---------------------------------- */
long long cbench_chset_insert(const int* keys, size_t n);
void* cbench_chset_make(const int* keys, size_t n);
long long cbench_chset_lookup(void* h, const int* keys, size_t n);
void cbench_chset_destroy(void* h);

/* ---- CMat vs std::vector<std::vector<int>> ----------------------------- */
long long cbench_cmat_build(size_t rows, size_t cols);         /* push cols/row*/

#ifdef __cplusplus
}
#endif
