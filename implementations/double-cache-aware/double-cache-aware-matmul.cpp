#include <iostream>
#include <sys/sysctl.h>
#include <cstdint>


double **double_cache_aware_matmul(double **m1, double **m2, int row_dim, int col_dim, int common_dim, int block_size) {
    int cache_line_size = 128; // bytes
    int l1_data_cache_size = 128 * 1024; // bytes, total is 128 KB
    int l2_cache_size = 16 * 1024 * 1024; // bytes, total is 16 MB

    // For this project we want to target 75% of L1 cache usage.
    int bytes_per_element = sizeof(double); // 8 bytes
    int elements_per_cache_line = cache_line_size / bytes_per_element; // 16 elements

    // Safety/Headroom Threshold Factor (Target ~75% capacity to prevent eviction thrashing)
    double safety_factor = 0.75;
    double target_l1_capacity = l1_data_cache_size * safety_factor;
    double target_l2_capacity = l2_cache_size * safety_factor;

    // Compute L1 Block Size (B_L1)
    // Formula: 3 * (B_L1 * B_L1) * bytes_per_element <= target_l1_capacity
    double raw_b_l1 = std::sqrt(target_l1_capacity / (3.0 * bytes_per_element));
    
    // Round down to the nearest multiple of cache line elements
    int B_L1 = (static_cast<int>(raw_b_l1) / elements_per_cache_line) * elements_per_cache_line;
    // Guard against zero if cache size is somehow configured abnormally small
    B_L1 = std::max(B_L1, elements_per_cache_line); 

    // Compute L2 Block Size (B_L2)
    // Formula: 3 * (B_L2 * B_L2) * bytes_per_element <= target_l2_capacity
    double raw_b_l2 = std::sqrt(target_l2_capacity / (3.0 * bytes_per_element));
    
    // Round down to the nearest multiple of B_L1 to ensure even tile nesting
    int B_L2 = (static_cast<int>(raw_b_l2) / B_L1) * B_L1;
    B_L2 = std::max(B_L2, B_L1);

    double **result = new double *[row_dim];
    for (int i = 0; i < row_dim; ++i) {
        result[i] = new double[col_dim]{};
    }


    // Here's where the magic happens!
    
}
