// Timed benchmarks: CStyleDS C containers vs C++ standard library.
//
// The C containers are exercised through bench_c.c (compiled as C23); the STL
// baselines run here. Both sides are timed by the same std::chrono clock and fed
// identical input, so the numbers are directly comparable. Each pair also has its
// checksums compared, which validates that the C container returns correct
// results (printed as [ok] / [MISMATCH]).
//
// Build in an OPTIMISED config (Release / RelWithDebInfo) or the numbers are
// meaningless; the harness prints a warning if it detects a debug build.

#include "bench.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <functional>
#include <numeric>
#include <queue>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

// Keep results observable so the optimiser can't delete the measured work.
volatile long long g_sink = 0;

// Run a given number of times, return best (minimum) wall time in nanoseconds.
// Best-of-N rejects scheduler/allocator noise better than the mean for
// microbenchmarks. `fn` returns a checksum that is routed to a volatile sink.
template <class F>
double best_ns(F&& fn, int iters) {
  double best = 1e300;
  for (int i = 0; i < iters; i++) {
    auto t0 = Clock::now();
    long long chk = fn();
    auto t1 = Clock::now();
    g_sink += chk;
    double ns = std::chrono::duration<double, std::nano>(t1 - t0).count();
    best = std::min(best, ns);
  }
  return best;
}

int g_pass = 0, g_fail = 0;

// Time a pair of corresponding operations passed as function wrappers
// ops == number of logical operations (for the per-op columns).
template <class CFn, class CppFn>
void getResRow(const char* name, double ops, int iters, CFn&& cfn, CppFn&& cppfn) {
  // Correctness: one untimed run of each, checksums must match.
  long long c_chk = cfn();
  long long cpp_chk = cppfn();
  const bool ok = (c_chk == cpp_chk);
  ok ? g_pass++ : g_fail++;

  double c = best_ns(cfn, iters);
  double cpp = best_ns(cppfn, iters);

  double c_per = c / ops;
  double cpp_per = cpp / ops;
  double ratio = cpp_per > 0 ? c_per / cpp_per : 0.0;

  std::printf("%-26s %13.2f %13.2f %9.2fx   %s\n",
    name, c_per, cpp_per, ratio, ok ? "[ok]" : "[MISMATCH]");
}

}  // namespace

int main() {
  const size_t N = 1'000'000;
  const size_t MAT_ROWS = 1'000, MAT_COLS = 1'000;

  std::mt19937_64 rng(0xC0FFEE);

  // Random ints for order-sensitive containers (vec/deq/pq).
  std::vector<int> rnd(N);
  for (auto& x : rnd) x = static_cast<int>(rng());

  // N unique shuffled keys for map/set (clean, no dup noise); vals == key
  // so every checksum reduces to a known sum where both sides should agree.
  std::vector<int> ukeys(N), uvals(N);
  std::iota(ukeys.begin(), ukeys.end(), 0);
  std::shuffle(ukeys.begin(), ukeys.end(), rng);
  uvals = ukeys;

  // Random access pattern for the indexing benchmark.
  std::vector<size_t> idx(N);
  for (auto& x : idx) x = rng() % N;

  std::printf("CStyleDS vs C++ STL  |  N = %zu, mat = %zux%zu  |  best-of-N, ns per op\n",
              N, MAT_ROWS, MAT_COLS);
#ifndef NDEBUG
  std::printf("\n  *** WARNING: debug build (NDEBUG not defined). Rebuild in Release "
              "for meaningful numbers. ***\n");
#endif
  std::printf("\n%-26s %13s %13s %10s\n", "benchmark", "C (ns/op)", "C++ (ns/op)", "C / C++");
  std::printf("%s\n", "--------------------------------------------------------------------------");

  // ---- CVec vs std::vector<int> ----------------------------------------
  getResRow("vec build+sum", (double)N, 5,
      [&] { return cbench_cvec_build(rnd.data(), N); },
      [&] {
        std::vector<int> v;
        for (size_t i = 0; i < N; i++) v.push_back(rnd[i]);
        long long s = 0; for (int x : v) s += x; return s;
      });

  {
    void* ch = cbench_cvec_make(rnd.data(), N);
    std::vector<int> cppv(rnd.begin(), rnd.end());
    getResRow("vec iterate (sum)", (double)N, 10,
        [&] { return cbench_cvec_sum(ch); },
        [&] { long long s = 0; for (int x : cppv) s += x; return s; });
    getResRow("vec random index", (double)N, 10,
        [&] { return cbench_cvec_index(ch, idx.data(), N); },
        [&] { long long s = 0; for (size_t i = 0; i < N; i++) s += cppv[idx[i]]; return s; });
    cbench_cvec_destroy(ch);
  }

  // ---- emplace copy investigation: struct payloads ---------------------
  // Pair (8B): the compound-literal copy is small enough that the optimiser
  // should collapse emplace, push and C++ emplace_back/push_back together.
  getResRow("vec<pair8> emplace", (double)N, 5,
      [&] { return cbench_cvec_pair_emplace(rnd.data(), N); },
      [&] {
        std::vector<std::pair<int, int>> v;
        for (size_t i = 0; i < N; i++) v.emplace_back(rnd[i], rnd[i] + 1);
        long long s = 0; for (auto& p : v) s += p.first + p.second; return s;
      });
  getResRow("vec<pair8> push_back", (double)N, 5,
      [&] { return cbench_cvec_pair_push(rnd.data(), N); },
      [&] {
        std::vector<std::pair<int, int>> v;
        for (size_t i = 0; i < N; i++) v.push_back({rnd[i], rnd[i] + 1});
        long long s = 0; for (auto& p : v) s += p.first + p.second; return s;
      });

  // Big (64B): C's emplace still does compound-literal + memcpy (one full copy).
  // Row 1 pits it against C++ emplace_back, which constructs in place (zero
  // copy) -- this is where the "extra copy" the C form can't avoid shows up.
  // Row 2 pits the same C emplace against C++ push_back (also one copy) -- they
  // should match, confirming CVec_emplace is really a push, not a true emplace.
  struct Big { int v[16]; };
  getResRow("vec<big64> C-empl/C++-empl", (double)N, 5,
      [&] { return cbench_cvec_big_emplace(rnd.data(), N); },
      [&] {
        std::vector<Big> v;
        for (size_t i = 0; i < N; i++) { v.emplace_back(); v.back().v[0] = rnd[i]; }
        long long s = 0; for (auto& b : v) s += b.v[0]; return s;
      });
  getResRow("vec<big64> C-empl/C++-push", (double)N, 5,
      [&] { return cbench_cvec_big_emplace(rnd.data(), N); },
      [&] {
        std::vector<Big> v;
        for (size_t i = 0; i < N; i++) { Big b{}; b.v[0] = rnd[i]; v.push_back(b); }
        long long s = 0; for (auto& b : v) s += b.v[0]; return s;
      });
  getResRow("vec<big64> C-push/C++-push", (double)N, 5,
      [&] { return cbench_cvec_big_push(rnd.data(), N); },
      [&] {
        std::vector<Big> v;
        for (size_t i = 0; i < N; i++) { Big b{}; b.v[0] = rnd[i]; v.push_back(b); }
        long long s = 0; for (auto& b : v) s += b.v[0]; return s;
      });

  // ---- CDeq vs std::deque<int> -----------------------------------------
  getResRow("deq build (push_back)", (double)N, 5,
      [&] { return cbench_cdeq_build(rnd.data(), N); },
      [&] {
        std::deque<int> q;
        for (size_t i = 0; i < N; i++) q.push_back(rnd[i]);
        return (long long)q.size();
      });

  getResRow("deq mixed ends+drain", (double)N, 5,
      [&] { return cbench_cdeq_mixed(rnd.data(), N); },
      [&] {
        std::deque<int> q;
        for (size_t i = 0; i < N; i++) {
          if (i & 1) q.push_front(rnd[i]); else q.push_back(rnd[i]);
        }
        long long s = 0; size_t t = 0;
        while (!q.empty()) {
          if (t & 1) { s += q.front(); q.pop_front(); }
          else       { s += q.back();  q.pop_back();  }
          t++;
        }
        return s;
      });

  // ---- CPrioQ vs std::priority_queue<int> (max-heap) -------------------
  getResRow("prioq build+drain", (double)N, 5,
      [&] { return cbench_cpq_sort(rnd.data(), N); },
      [&] {
        std::priority_queue<int> pq;
        for (size_t i = 0; i < N; i++) pq.push(rnd[i]);
        long long s = 0; while (!pq.empty()) { s += pq.top(); pq.pop(); }
        return s;
      });

  // ---- CHMap vs std::unordered_map<int,int> ----------------------------
  getResRow("hmap insert", (double)N, 5,
      [&] { return cbench_chmap_insert(ukeys.data(), uvals.data(), N); },
      [&] {
        std::unordered_map<int, int> m;
        for (size_t i = 0; i < N; i++) m[ukeys[i]] = uvals[i];
        return (long long)m.size();
      });

  {
    void* ch = cbench_chmap_make(ukeys.data(), uvals.data(), N);
    std::unordered_map<int, int> cppm;
    cppm.reserve(N);
    for (size_t i = 0; i < N; i++) cppm[ukeys[i]] = uvals[i];
    getResRow("hmap lookup (hit)", (double)N, 10,
        [&] { return cbench_chmap_lookup(ch, ukeys.data(), N); },
        [&] {
          long long s = 0;
          for (size_t i = 0; i < N; i++) { auto it = cppm.find(ukeys[i]); if (it != cppm.end()) s += it->second; }
          return s;
        });
    cbench_chmap_destroy(ch);
  }

  // ---- CHSet vs std::unordered_set<int> --------------------------------
  getResRow("hset insert", (double)N, 5,
      [&] { return cbench_chset_insert(ukeys.data(), N); },
      [&] {
        std::unordered_set<int> s;
        for (size_t i = 0; i < N; i++) s.insert(ukeys[i]);
        return (long long)s.size();
      });

  {
    void* ch = cbench_chset_make(ukeys.data(), N);
    std::unordered_set<int> cpps;
    cpps.reserve(N);
    for (size_t i = 0; i < N; i++) cpps.insert(ukeys[i]);
    getResRow("hset lookup (hit)", (double)N, 10,
        [&] { return cbench_chset_lookup(ch, ukeys.data(), N); },
        [&] { long long h = 0; for (size_t i = 0; i < N; i++) if (cpps.count(ukeys[i])) h++; return h; });
    cbench_chset_destroy(ch);
  }

  // ---- CMat vs std::vector<std::vector<int>> ---------------------------
  getResRow("mat jagged build", (double)(MAT_ROWS * MAT_COLS), 5,
      [&] { return cbench_cmat_build(MAT_ROWS, MAT_COLS); },
      [&] {
        std::vector<std::vector<int>> m(MAT_ROWS);
        long long s = 0;
        for (size_t r = 0; r < MAT_ROWS; r++) {
          m[r].reserve(MAT_COLS);
          for (size_t c = 0; c < MAT_COLS; c++) { int v = (int)(r + c); m[r].push_back(v); s += v; }
        }
        return s;
      });

  std::printf("%s\n", "--------------------------------------------------------------------------");
  std::printf("correctness: %d ok, %d mismatched\n", g_pass, g_fail);
  std::printf("(ratio C/C++: <1.0 means the C container was faster)\n");
  return g_fail == 0 ? 0 : 1;
}
