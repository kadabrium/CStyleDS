# CStyleDS benchmarks

Timed comparison of the CStyleDS C containers against their closest C++ standard
library equivalents:

| CStyleDS | C++ baseline |
|----------|--------------|
| `CVec`   | `std::vector<int>` |
| `CDeq`   | `std::deque<int>` |
| `CPrioQ` | `std::priority_queue<int>` |
| `CHMap`  | `std::unordered_map<int,int>` |
| `CHSet`  | `std::unordered_set<int>` |
| `CMat`   | `std::vector<std::vector<int>>` |

## No external dependencies

Only the C++ standard library. Timing is a hand-rolled best-of-N harness over
`std::chrono::steady_clock` (see `main.cpp`). If you later want statistical rigor
(variance, auto-tuned iteration counts, isolation), swap the harness for
[Google Benchmark] — that's the one thing that would add a dependency.

[Google Benchmark]: https://github.com/google/benchmark

## Layout / why two languages

The container headers use `_Generic`, `__typeof__` and compound literals, so they
**cannot compile as C++**. The benchmark is therefore split:

- `bench_c.c`  — compiled as **C23**, includes the headers, does the C-side work.
- `main.cpp`   — compiled as **C++20**, runs the STL baselines and times *both*
  sides through one clock so the numbers are comparable.
- `bench.h`    — the plain-C interface that crosses the language boundary.

Handle-based entry points (`*_make` / `*_lookup` / `*_destroy`) let the harness
build a structure once, untimed, and measure only the operation of interest.

Each pair's checksums are compared: a matching checksum doubles as a correctness
test of the C container (`[ok]` / `[MISMATCH]` in the output). The program exits
non-zero if any pair mismatches.

## Build & run

Uses the repo's normal presets (see `../../CMakePresets.json`). Build **optimised**
— a debug build prints a warning and the numbers are meaningless.

```sh
# from the repo root
cmake --preset msvc
cmake --build --preset release --target CStyleDS_bench
./_build/msvc/bin/Release/CStyleDS_bench.exe
```

The MinGW/Clang preset works too (override the build type to Release):

```sh
cmake --preset mingw -DCMAKE_BUILD_TYPE=Release
cmake --build --preset mingw-debug --target CStyleDS_bench
```

## Note: the emulated `emplace` "extra copy"

`CVec_emplace(&v, T, ...)` expands to `_CVec_push(&v, &(T){...})` — it builds a
compound literal and `memcpy`s it in. That is **exactly one copy**, semantically
`push_back(T{...})`, not a true C++ `emplace_back(args...)` (which constructs in
place, zero copies). C has no placement-new / perfect forwarding, so it can't do
better. The `vec<pair8>` / `vec<big64>` rows measure whether that copy costs
anything:

- **C emplace ≈ C push, always** (both an 8-byte pair and a 64-byte struct). This
  confirms the emulated `emplace` really is a `push` — no better, but no worse.
- **8-byte payload:** emplace/push land within noise of C++ `emplace_back` — the
  copy is elided by the optimiser.
- **64-byte payload:** the copy is *not* elided, but it's dwarfed by growth cost.
  `CVec` (realloc, can grow in place) actually edges out `std::vector` (copies all
  live elements on every reallocation) for trivially-copyable payloads.

Takeaway: the extra copy is real in the codegen sense but not a measurable
penalty for POD — which is all C has. (C++ `emplace_back`'s in-place win only
matters for non-trivially-copyable types, which C can't express anyway.)

## Reading the output

Columns are **nanoseconds per logical operation** (build cost amortised over the
N inserts, etc.). The last column is `C / C++`: **below 1.0 means the CStyleDS
container was faster**. Tune `N` and the matrix size at the top of `main()`.
