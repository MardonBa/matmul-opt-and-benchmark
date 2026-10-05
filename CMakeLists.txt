cmake_minimum_required(VERSION 3.16)

project(matmul_benchmark)

set(CMAKE_CXX_STANDARD 17)

find_package(benchmark REQUIRED)

set(MATMUL_IMPLEMENTATION_SOURCES
    implementations/naive/naive_matmul.cpp
    implementations/ikj/ikj_matmul.cpp
    implementations/jik/jik_matmul.cpp
    implementations/jki/jki_matmul.cpp
    implementations/kij/kij_matmul.cpp
    implementations/kji/kji_matmul.cpp
    implementations/cache_aware/cache_aware_matmul.cpp
    implementations/double_cache_aware/double_cache_aware_matmul.cpp
)

add_executable(matmul_bench
    app/benchmark.cpp
    app/matmul_implementations.cpp
    ${MATMUL_IMPLEMENTATION_SOURCES}
)

target_include_directories(matmul_bench PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(matmul_bench
    benchmark::benchmark
)

include(CTest)
if(BUILD_TESTING)
    add_executable(matmul_correctness
        tests/matmul_correctness.cpp
        ${MATMUL_IMPLEMENTATION_SOURCES}
    )
    target_include_directories(matmul_correctness PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(NAME matmul_correctness COMMAND matmul_correctness)
endif()
