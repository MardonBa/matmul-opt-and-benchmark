#include <iostream>
#include <sys/sysctl.h>
#include <cstdint>


double **cache_aware_matmul(double **m1, double **m2, int row_dim, int col_dim, int common_dim, int block_size) {
    int cache_line_size = 128; // bytes
    int l1_data_cache_size = 128 * 1024; // bytes, total is 128 KB
    int l2_cache_size = 16 * 1024 * 1024; // bytes, total is 16 MB

    // For this project we want to target 75% of L1 cache usage.
    int bytes_per_element = sizeof(double); // 8 bytes, size of our data impacts block size
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

    double **result = new double *[row_dim];
    for (int i = 0; i < row_dim; ++i) {
        result[i] = new double[col_dim]{};
    }


    // Here's where the magic happens!

    // These outer loops iterate over the blocks
    for (int sj = 0; sj < col_dim; sj += B_L1) {
        for (int si = 0; si < row_dim; si += B_L1) {
            for (int sk = 0; sk < common_dim; sk += B_L1) {
                
                // Make sure we aren't iterating beyond the size of our marices!
                int i_max = std::min(si + B_L1, row_dim);
                int j_max = std::min(sj + B_L1, col_dim);
                int k_max = std::min(sk + B_L1, common_dim);

                // Iterate over the blocks, doing our standard matmul
                // Note that we're using the most efficient loop ordering we found, ikj
                for (int i = si; i < i_max; i++) {
                    for (int k = sk; k < k_max; k++) {
                        double r = m1[i][k];
                    }
                }
            }
        }
    }
}
