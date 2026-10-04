# matmul-opt-and-benchmark

very cool project!

## Checklist

- [x] Implement naive matmul
- [x] Implement time, operation, memory fetch?? benchmarking
- [x] Implement matmul with various loop orders
- [ ] Writeup on the CPU operations, why loop order matters
- [ ] Cache-aware blocking/tiling
- [ ] Compiler optimization
- [ ] SIMD/Vectorization
- [ ] Multithreading
- [ ] Comparison against optimized BLAS

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

On Linux, cycles, instructions, cache metrics, IPC, and branch metrics are
collected with `perf_event_open`; this may require permission to use hardware
counters (`perf_event_paranoid`). On macOS 12.4 or later, cycles, instructions,
and IPC are collected with the OS's per-thread Recount counters:

```sh
./build/matmul_bench --metrics=cycles,instructions,ipc --benchmark_filter='.*N:512.*'
```

The macOS counter API is runtime-checked because some configurations do not
expose instruction and cycle counts. Cache-miss and branch metrics remain
Linux-only. Vectorization percentage is intentionally unavailable because it
needs compiler- and architecture-specific instruction analysis, rather than a
portable runtime counter. `--all-metrics` runs every metric supported on the
current platform and reports the unavailable ones.

## Profiling with Instruments on macOS

Use the benchmark runner for repeatable timing and CSV comparisons; use
Instruments to explain *why* one implementation is faster. Instruments adds
profiling overhead, so do not treat its elapsed time as the benchmark result.

1. Install the full Xcode app (Command Line Tools alone do not include
   Instruments), then build a symbolized optimized binary:

   ```sh
   cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
   cmake --build build
   ```

2. Open **Instruments** from Xcode (**Xcode > Open Developer Tool >
   Instruments**) and select the **CPU Counters** template.
3. Choose the `matmul_bench` executable as the target. In its launch arguments,
   use one implementation and one useful size, for example:

   ```text
   --implementations=ikj --benchmark_filter=.*N:512.* --benchmark_min_time=0.5s
   ```

4. In the CPU Counters instrument, select **CPU Bottlenecks** mode, then click
   Record. Let the selected benchmark finish and stop the recording.
5. In the call tree/detail view, focus on `*_multiply` and its inner loops.
   Compare the loop-order runs separately, looking for stalls, cache-related
   bottlenecks, branch behavior, and vectorization guidance reported by the
   instrument.

Close other CPU-heavy applications and repeat the same focused trace when
comparing variants. Apple documents the CPU Counters workflow and uses it to
validate changes after rerunning performance tests.

## Adding an implementation

Keep the function signature used by `naive_multiply`, add the implementation's
files under its own `implementations/<name>/` directory, add its `.cpp` file to
`matmul_bench` in `CMakeLists.txt`, and add one entry to
`app/matmul_implementations.cpp`:

```cpp
{"blocked", blocked_multiply},
```

It will then appear in `--list-implementations`, run by default, and can be
selected with `--implementations=blocked` or compared directly with
`--implementations=naive,blocked`.

## Source layout

- `app/` contains the benchmark executable and implementation registry.
- `implementations/<name>/` contains one matrix multiplication implementation
  and its header. Current folders are `naive`, `ikj`, `jik`, `jki`, `kij`, and
  `kji`, plus `cache-aware` and `double-cache-aware`.
- `tests/` verifies every implementation against a rectangular reference case.

## Matrix representation

All implementations accept and return `matmul::Matrix`, an alias for a flat
`std::vector<double>` in row-major order. For `A(rows × shared)` and
`B(shared × columns)`, element `(row, column)` is stored at
`row * columns + column`; the result contains exactly `rows * columns`
elements. `implementations/matrix.h` centralizes this contract and validates
the supplied storage dimensions.

Run the correctness test with:

```sh
ctest --test-dir build --output-on-failure
```
