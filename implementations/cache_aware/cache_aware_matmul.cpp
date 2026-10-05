#include "cache_aware_matmul.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <sys/sysctl.h>

std::vector<double> cache_aware_matmul(const std::vector<double> &m1,
                                       const std::vector<double> &m2, int row_dim,
                                       int col_dim, int common_dim) {
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

    std::vector<double> result = matmul::make_result(m1, m2, row_dim, col_dim, common_dim);

    // i-k-j keeps the right-hand matrix and result accesses contiguous.
    for (int si = 0; si < row_dim; si += B_L1) {
        const int i_max = std::min(si + B_L1, row_dim);
        for (int sk = 0; sk < common_dim; sk += B_L1) {
            const int k_max = std::min(sk + B_L1, common_dim);
            for (int sj = 0; sj < col_dim; sj += B_L1) {
                const int j_max = std::min(sj + B_L1, col_dim);
                for (int i = si; i < i_max; ++i) {
                    for (int k = sk; k < k_max; ++k) {
                        const double left = m1[i * common_dim + k];
                        for (int j = sj; j < j_max; ++j) {
                            result[i * col_dim + j] += left * m2[k * col_dim + j];
                        }
                    }
                }
            }
        }
    }
    return result;
}
