# matmul-opt-and-benchmark

very cool project!

## Checklist

- [x] Implement naive matmul
- [ ] Implement time, operation, memory fetch?? benchmarking
- [ ] Implement matmul with various loop orders
- [ ] Writeup on the CPU operations, why loop order matters
- [ ] next steps..

## Setup

Install Google Benchmark, configure, and build:

```sh
brew install google-benchmark        # macOS
cmake -S . -B build
cmake --build build
```

## Running benchmarks

Run every registered implementation and matrix size:

```sh
./build/matmul_bench
```

Use normal Google Benchmark flags to narrow the run or control repetitions:

```sh
./build/matmul_bench --benchmark_filter='matmul/naive.*' --benchmark_repetitions=5
```

List the names available for selection, then run just the implementations you
want (comma-separated). The default is all implementations; use
`--all-implementations` when you want that choice to be explicit in a script:

```sh
./build/matmul_bench --list-implementations
./build/matmul_bench --implementations=naive --benchmark_filter='.*N:256.*'
# A quick all-implementation comparison; omit the filter for the full suite.
./build/matmul_bench --all-implementations --all-metrics --benchmark_filter='.*N:(32|64|128|256).*'
```

The full suite includes `N=1024` and `N=2048` for every implementation. Since
matrix multiplication is cubic, those cases can take a long time (especially
in a non-Release build), and Google Benchmark may not print a row until that
case has completed. Start with the filtered command above when comparing new
implementations.

Metrics are opt-in so their collection does not affect ordinary timing runs.
Use `--list-metrics` for the complete current list, or request a
comma-separated subset:

```sh
./build/matmul_bench --metrics=flops,allocations,bytes-allocated,effective-bandwidth
./build/matmul_bench --all-metrics
```

Each selected metric is printed in its own named column by default. To return
to Google Benchmark's single `UserCounters` column, pass
`--benchmark_counters_tabular=false`.

Write the same benchmark run to CSV while retaining the console table:

```sh
./build/matmul_bench --all-implementations --metrics=flops,allocations --csv=benchmark-results.csv
```

`--csv=PATH` writes Google Benchmark's complete CSV report to `PATH`; use a
different filename for each comparison run so results are not overwritten.

## Visualizing growth

Generate an SVG report with CPU-time growth, throughput by matrix size, and
speedup relative to `naive`. The plotter uses only Python's standard library.

```sh
python3 plot_benchmarks.py benchmark-results.csv --output benchmark-growth.svg
```

The CPU-time chart uses log axes and adds a fitted `N^p` exponent to each
implementation's legend. For ordinary matrix multiplication, `p` should be
near 3; a materially smaller value can indicate that the implementation is not
doing equivalent work.

`flop/s` is calculated as `2 * N^3`; effective bandwidth is the logical
traffic for reading both inputs and writing the output (`3 * N^2 * sizeof(double)`),
so it is a useful comparison metric rather than a measurement of DRAM traffic.
Allocation counters cover C++ `new`/`new[]` calls made by the multiplication
itself.

Cycles, instructions, cache-miss counters, IPC, and branch-miss rate are
collected with Linux `perf_event_open`. They require Linux permissions to use
performance counters (and may be restricted by `perf_event_paranoid`). On
macOS they are listed as unavailable; the portable metrics above still work.
Vectorization percentage is likewise listed but intentionally unavailable: it
needs architecture- and compiler-specific instruction classification, not a
portable runtime counter. `--all-metrics` runs every metric supported on the
current platform and reports the unavailable ones.

## Adding an implementation

Keep the function signature used by `naive_multiply`, add the implementation's
source file to `matmul_bench` in `CMakeLists.txt`, and add one entry to
`matmul_implementations.cpp`:

```cpp
{"blocked", blocked_multiply},
```

It will then appear in `--list-implementations`, run by default, and can be
selected with `--implementations=blocked` or compared directly with
`--implementations=naive,blocked`.
