#include <benchmark/benchmark.h>

#include "matmul_implementations.h"

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <new>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(__linux__)
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>
#elif defined(__APPLE__)
#include <sys/syscall.h>
#include <unistd.h>
#endif

namespace {

enum class Metric { kFlops, kAllocations, kBytesAllocated, kEffectiveBandwidth,
                    kCycles, kInstructions, kCacheMisses, kL1CacheMisses,
                    kLastLevelCacheMisses, kBranchMisses, kInstructionsPerCycle,
                    kBranchMissRate, kVectorization };

struct MetricDefinition { const char *name; Metric metric; const char *description; };

constexpr MetricDefinition kMetricDefinitions[] = {
    {"flops", Metric::kFlops, "floating-point operations per second"},
    {"allocations", Metric::kAllocations, "C++ allocations per multiplication"},
    {"bytes-allocated", Metric::kBytesAllocated, "C++ bytes allocated per multiplication"},
    {"effective-bandwidth", Metric::kEffectiveBandwidth, "logical matrix bytes processed per second"},
    {"cycles", Metric::kCycles, "CPU cycles per multiplication (Linux perf or macOS Recount)"},
    {"instructions", Metric::kInstructions, "retired instructions per multiplication (Linux perf or macOS Recount)"},
    {"cache-misses", Metric::kCacheMisses, "hardware cache misses per multiplication (Linux perf)"},
    {"l1-cache-misses", Metric::kL1CacheMisses, "L1 data-cache read misses per multiplication (Linux perf)"},
    {"llc-cache-misses", Metric::kLastLevelCacheMisses, "last-level-cache read misses per multiplication (Linux perf)"},
    {"branch-misses", Metric::kBranchMisses, "branch mispredictions per multiplication (Linux perf)"},
    {"ipc", Metric::kInstructionsPerCycle, "instructions per cycle (Linux perf or macOS Recount)"},
    {"branch-miss-rate", Metric::kBranchMissRate, "branch mispredictions divided by branch instructions (Linux perf)"},
    {"vectorization", Metric::kVectorization, "not available: no portable SIMD-instruction percentage exists"},
};

struct BenchmarkConfig {
    std::set<Metric> metrics;
    std::set<std::string> implementations;
    bool list_implementations = false;
    bool all_implementations = false;
    bool list_metrics = false;
    bool all_metrics = false;
    std::string csv_output;
};
BenchmarkConfig config;

bool is_hardware_metric(Metric metric) {
    return metric >= Metric::kCycles && metric <= Metric::kBranchMissRate;
}

bool metric_is_available(Metric metric) {
    if (metric == Metric::kVectorization) return false;
#if defined(__linux__)
    return true;
#elif defined(__APPLE__)
    return metric == Metric::kCycles || metric == Metric::kInstructions ||
           metric == Metric::kInstructionsPerCycle || !is_hardware_metric(metric);
#else
    return !is_hardware_metric(metric);
#endif
}

const MetricDefinition *find_metric(const std::string &name) {
    for (const auto &definition : kMetricDefinitions)
        if (name == definition.name) return &definition;
    return nullptr;
}

void enable_metric(const std::string &name) {
    const auto *definition = find_metric(name);
    if (definition == nullptr) throw std::runtime_error("Unknown metric: " + name);
    if (!metric_is_available(definition->metric))
        throw std::runtime_error("Metric '" + name + "' is unavailable on this platform");
    config.metrics.insert(definition->metric);
}

void enable_implementation(const std::string &name) { config.implementations.insert(name); }

void parse_comma_separated(const std::string &value, void (*add)(const std::string &)) {
    std::stringstream stream(value);
    std::string item;
    while (std::getline(stream, item, ',')) {
        if (item.empty()) throw std::runtime_error("Empty value in comma-separated option");
        add(item);
    }
}

void parse_custom_args(int *argc, char **argv) {
    int write_index = 1;
    for (int read_index = 1; read_index < *argc; ++read_index) {
        const std::string argument = argv[read_index];
        if (argument.rfind("--metrics=", 0) == 0)
            parse_comma_separated(argument.substr(10), enable_metric);
        else if (argument.rfind("--implementations=", 0) == 0)
            parse_comma_separated(argument.substr(18), enable_implementation);
        else if (argument == "--all-implementations")
            config.all_implementations = true;
        else if (argument == "--all-metrics") {
            config.all_metrics = true;
            for (const auto &definition : kMetricDefinitions)
                if (metric_is_available(definition.metric)) config.metrics.insert(definition.metric);
        } else if (argument == "--list-metrics") config.list_metrics = true;
        else if (argument == "--list-implementations") config.list_implementations = true;
        else if (argument.rfind("--csv=", 0) == 0) {
            config.csv_output = argument.substr(6);
            if (config.csv_output.empty()) throw std::runtime_error("CSV output path cannot be empty");
        }
        else argv[write_index++] = argv[read_index];
    }
    *argc = write_index;
}

void print_metric_list() {
    std::cout << "Metrics:\n";
    for (const auto &definition : kMetricDefinitions)
        std::cout << "  " << definition.name << " - " << definition.description
                  << (metric_is_available(definition.metric) ? "" : " [unavailable]") << '\n';
}

void print_implementation_list() {
    std::cout << "Implementations:\n";
    for (const auto &implementation : matmul_implementations())
        std::cout << "  " << implementation.name << '\n';
}

double **make_matrix(int rows, int columns) {
    double **matrix = new double *[rows];
    for (int row = 0; row < rows; ++row) {
        matrix[row] = new double[columns];
        for (int column = 0; column < columns; ++column)
            matrix[row][column] = static_cast<double>(std::rand()) / RAND_MAX;
    }
    return matrix;
}

void free_matrix(double **matrix, int rows) {
    for (int row = 0; row < rows; ++row) delete[] matrix[row];
    delete[] matrix;
}

struct AllocationStats { std::uint64_t count = 0; std::uint64_t bytes = 0; };
thread_local AllocationStats *active_allocation_stats = nullptr;
void record_allocation(std::size_t bytes) {
    if (active_allocation_stats != nullptr) {
        ++active_allocation_stats->count;
        active_allocation_stats->bytes += bytes;
    }
}
class AllocationScope {
  public:
    explicit AllocationScope(AllocationStats *stats) : previous_(active_allocation_stats) { active_allocation_stats = stats; }
    ~AllocationScope() { active_allocation_stats = previous_; }
  private:
    AllocationStats *previous_;
};

#if defined(__linux__) || defined(__APPLE__)
struct PerfReadings {
    std::uint64_t cycles = 0, instructions = 0, cache_misses = 0, l1_cache_misses = 0,
                  llc_cache_misses = 0, branch_misses = 0, branch_instructions = 0;
};
#endif

#if defined(__linux__)
class PerfCounters {
  public:
    explicit PerfCounters(const std::set<Metric> &metrics) : metrics_(metrics) {}
    ~PerfCounters() { for (int fd : fds_) if (fd >= 0) close(fd); }
    bool start(std::string *error) {
        add_requested_counters();
        for (int fd : fds_) {
            if (fd < 0) { *error = "perf_event_open failed: " + std::string(std::strerror(errno)) + ". Check perf_event_paranoid permissions."; return false; }
            ioctl(fd, PERF_EVENT_IOC_RESET, 0);
            ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
        }
        return true;
    }
    PerfReadings stop() {
        PerfReadings result;
        for (const auto &counter : counters_) {
            ioctl(counter.first, PERF_EVENT_IOC_DISABLE, 0);
            std::uint64_t value = 0;
            read(counter.first, &value, sizeof(value));
            result.*counter.second = value;
        }
        return result;
    }
  private:
    using Member = std::uint64_t PerfReadings::*;
    bool wants(Metric metric) const { return metrics_.count(metric) != 0; }
    void add_counter(std::uint32_t type, std::uint64_t config, Member destination) {
        perf_event_attr attributes{};
        attributes.type = type; attributes.size = sizeof(attributes); attributes.config = config;
        attributes.disabled = 1; attributes.exclude_kernel = 1; attributes.exclude_hv = 1;
        const int fd = static_cast<int>(syscall(__NR_perf_event_open, &attributes, 0, -1, -1, 0));
        fds_.push_back(fd); counters_.push_back({fd, destination});
    }
    void add_requested_counters() {
        if (wants(Metric::kCycles) || wants(Metric::kInstructionsPerCycle)) add_counter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES, &PerfReadings::cycles);
        if (wants(Metric::kInstructions) || wants(Metric::kInstructionsPerCycle)) add_counter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS, &PerfReadings::instructions);
        if (wants(Metric::kCacheMisses)) add_counter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES, &PerfReadings::cache_misses);
        if (wants(Metric::kL1CacheMisses)) add_counter(PERF_TYPE_HW_CACHE, PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16), &PerfReadings::l1_cache_misses);
        if (wants(Metric::kLastLevelCacheMisses)) add_counter(PERF_TYPE_HW_CACHE, PERF_COUNT_HW_CACHE_LL | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16), &PerfReadings::llc_cache_misses);
        if (wants(Metric::kBranchMisses) || wants(Metric::kBranchMissRate)) add_counter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_MISSES, &PerfReadings::branch_misses);
        if (wants(Metric::kBranchMissRate)) add_counter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_INSTRUCTIONS, &PerfReadings::branch_instructions);
    }
    const std::set<Metric> &metrics_;
    std::vector<int> fds_;
    std::vector<std::pair<int, Member>> counters_;
};
#endif

#if defined(__APPLE__)
// `thread_selfcounts` is a macOS 12.4+ SPI.  It is intentionally invoked via
// syscall because its private SDK header is not installed with Xcode's public
// headers. THSC_CPI (kind 1) returns these two cumulative thread counters.
struct DarwinThreadCounts {
    std::uint64_t instructions;
    std::uint64_t cycles;
};

bool read_darwin_thread_counts(PerfReadings *readings, std::string *error) {
    constexpr std::uint32_t kThreadSelfCountsCpi = 1;
    DarwinThreadCounts counters{};
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    const long result = syscall(SYS_thread_selfcounts, kThreadSelfCountsCpi, &counters, sizeof(counters));
#pragma clang diagnostic pop
    if (result != 0) {
        *error = "macOS thread_selfcounts failed: " + std::string(std::strerror(errno)) +
                 ". Cycles and instructions require macOS 12.4+ and supported hardware.";
        return false;
    }
    readings->cycles = counters.cycles;
    readings->instructions = counters.instructions;
    return true;
}

bool wants_darwin_recount_metrics() {
    return config.metrics.count(Metric::kCycles) || config.metrics.count(Metric::kInstructions) ||
           config.metrics.count(Metric::kInstructionsPerCycle);
}
#endif

void add_metric_counters(benchmark::State &state, int n, const AllocationStats &allocations
#if defined(__linux__) || defined(__APPLE__)
                         , const PerfReadings &perf
#endif
) {
    const double iterations = static_cast<double>(state.iterations());
    const double operations = 2.0 * n * n * n;
    const double logical_bytes = 3.0 * n * n * sizeof(double);
    if (config.metrics.count(Metric::kFlops)) state.counters["flop/s"] = benchmark::Counter(operations, benchmark::Counter::kIsIterationInvariantRate);
    if (config.metrics.count(Metric::kEffectiveBandwidth)) state.counters["effective_bytes/s"] = benchmark::Counter(logical_bytes, benchmark::Counter::kIsIterationInvariantRate);
    if (config.metrics.count(Metric::kAllocations)) state.counters["allocations"] = allocations.count / iterations;
    if (config.metrics.count(Metric::kBytesAllocated)) state.counters["bytes_allocated"] = allocations.bytes / iterations;
#if defined(__linux__) || defined(__APPLE__)
    const auto per_iteration = [iterations](std::uint64_t value) { return value / iterations; };
    if (config.metrics.count(Metric::kCycles)) state.counters["cycles"] = per_iteration(perf.cycles);
    if (config.metrics.count(Metric::kInstructions)) state.counters["instructions"] = per_iteration(perf.instructions);
    if (config.metrics.count(Metric::kInstructionsPerCycle)) state.counters["ipc"] = perf.cycles == 0 ? 0.0 : static_cast<double>(perf.instructions) / perf.cycles;
#if defined(__linux__)
    if (config.metrics.count(Metric::kCacheMisses)) state.counters["cache_misses"] = per_iteration(perf.cache_misses);
    if (config.metrics.count(Metric::kL1CacheMisses)) state.counters["l1_cache_misses"] = per_iteration(perf.l1_cache_misses);
    if (config.metrics.count(Metric::kLastLevelCacheMisses)) state.counters["llc_cache_misses"] = per_iteration(perf.llc_cache_misses);
    if (config.metrics.count(Metric::kBranchMisses)) state.counters["branch_misses"] = per_iteration(perf.branch_misses);
    if (config.metrics.count(Metric::kBranchMissRate)) state.counters["branch_miss_rate"] = perf.branch_instructions == 0 ? 0.0 : static_cast<double>(perf.branch_misses) / perf.branch_instructions;
#endif
#endif
}

void run_benchmark(benchmark::State &state, MatmulImplementation implementation) {
    const int n = static_cast<int>(state.range(0));
    double **left = make_matrix(n, n);
    double **right = make_matrix(n, n);
    AllocationStats allocations;
#if defined(__linux__)
    PerfCounters perf_counters(config.metrics); PerfReadings perf; std::string perf_error;
    if (!perf_counters.start(&perf_error)) { free_matrix(left, n); free_matrix(right, n); state.SkipWithError(perf_error.c_str()); return; }
#elif defined(__APPLE__)
    PerfReadings perf_start, perf;
    std::string perf_error;
    const bool use_darwin_recount = wants_darwin_recount_metrics();
    if (use_darwin_recount && !read_darwin_thread_counts(&perf_start, &perf_error)) {
        free_matrix(left, n); free_matrix(right, n); state.SkipWithError(perf_error.c_str()); return;
    }
#endif
    for (auto _ : state) {
        double **result;
        { AllocationScope allocation_scope(&allocations); result = implementation.multiply(left, right, n, n, n); }
        benchmark::DoNotOptimize(result);
        free_matrix(result, n);
    }
#if defined(__linux__)
    perf = perf_counters.stop();
#elif defined(__APPLE__)
    if (use_darwin_recount) {
        PerfReadings perf_end;
        if (!read_darwin_thread_counts(&perf_end, &perf_error)) {
            free_matrix(left, n); free_matrix(right, n); state.SkipWithError(perf_error.c_str()); return;
        }
        perf.cycles = perf_end.cycles - perf_start.cycles;
        perf.instructions = perf_end.instructions - perf_start.instructions;
    }
#endif
    free_matrix(left, n); free_matrix(right, n);
    add_metric_counters(state, n, allocations
#if defined(__linux__) || defined(__APPLE__)
                        , perf
#endif
    );
}

bool selected_implementations_exist() {
    if (config.all_implementations) return true;
    for (const auto &name : config.implementations) {
        bool found = false;
        for (const auto &implementation : matmul_implementations()) if (implementation.name == name) found = true;
        if (!found) { std::cerr << "Unknown implementation: " << name << '\n'; return false; }
    }
    return true;
}

void register_benchmarks() {
    for (const auto &implementation : matmul_implementations()) {
        if (!config.all_implementations && !config.implementations.empty() &&
            !config.implementations.count(implementation.name)) continue;
        benchmark::RegisterBenchmark(("matmul/" + implementation.name).c_str(), [implementation](benchmark::State &state) { run_benchmark(state, implementation); })
            ->Arg(32)->Arg(64)->Arg(128)->Arg(256)->Arg(512)->Arg(1024)->Arg(2048)->ArgName("N");
    }
}

}  // namespace

void *operator new(std::size_t size) { if (void *pointer = std::malloc(size)) { record_allocation(size); return pointer; } throw std::bad_alloc(); }
void *operator new[](std::size_t size) { if (void *pointer = std::malloc(size)) { record_allocation(size); return pointer; } throw std::bad_alloc(); }
void operator delete(void *pointer) noexcept { std::free(pointer); }
void operator delete[](void *pointer) noexcept { std::free(pointer); }
void operator delete(void *pointer, std::size_t) noexcept { std::free(pointer); }
void operator delete[](void *pointer, std::size_t) noexcept { std::free(pointer); }

int main(int argc, char **argv) {
    try {
        parse_custom_args(&argc, argv);
        if (config.list_metrics) print_metric_list();
        if (config.list_implementations) print_implementation_list();
        if (config.list_metrics || config.list_implementations) return 0;
        if (!selected_implementations_exist()) return 1;
        if (config.all_metrics) for (const auto &definition : kMetricDefinitions)
            if (!metric_is_available(definition.metric)) std::cerr << "Skipping unavailable metric in --all-metrics: " << definition.name << '\n';
        // Google Benchmark puts custom counters in one long "UserCounters"
        // field unless this option is set. Make separate named columns the
        // default, while preserving an explicit user-supplied setting.
        bool tabular_counters_requested = false;
        bool benchmark_output_requested = false;
        for (int index = 1; index < argc; ++index)
            if (std::string(argv[index]).rfind("--benchmark_counters_tabular=", 0) == 0)
                tabular_counters_requested = true;
            else if (std::string(argv[index]).rfind("--benchmark_out=", 0) == 0 ||
                     std::string(argv[index]).rfind("--benchmark_out_format=", 0) == 0)
                benchmark_output_requested = true;
        if (!config.csv_output.empty() && benchmark_output_requested)
            throw std::runtime_error("Use either --csv=PATH or Google Benchmark's --benchmark_out options, not both");
        std::string default_tabular_counters = "--benchmark_counters_tabular=true";
        std::string csv_output_argument = "--benchmark_out=" + config.csv_output;
        std::string csv_format_argument = "--benchmark_out_format=csv";
        std::vector<char *> benchmark_arguments(argv, argv + argc);
        if (!tabular_counters_requested)
            benchmark_arguments.push_back(default_tabular_counters.data());
        if (!config.csv_output.empty()) {
            benchmark_arguments.push_back(csv_output_argument.data());
            benchmark_arguments.push_back(csv_format_argument.data());
        }
        benchmark_arguments.push_back(nullptr);
        int benchmark_argument_count = static_cast<int>(benchmark_arguments.size()) - 1;
        benchmark::Initialize(&benchmark_argument_count, benchmark_arguments.data());
        register_benchmarks();
        benchmark::RunSpecifiedBenchmarks();
        benchmark::Shutdown();
    } catch (const std::exception &error) {
        std::cerr << "Benchmark configuration error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
