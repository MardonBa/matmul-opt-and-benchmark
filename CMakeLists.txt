cmake_minimum_required(VERSION 3.16)

project(matmul_benchmark)

set(CMAKE_CXX_STANDARD 17)

find_package(benchmark REQUIRED)

add_executable(matmul_bench
    benchmark.cpp
    matmul_implementations.cpp
    loop_order_matmul.cpp
    naive_matmul.cpp
)

target_link_libraries(matmul_bench
    benchmark::benchmark
)
