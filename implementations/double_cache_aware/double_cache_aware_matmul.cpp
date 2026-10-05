#include "double_cache_aware_matmul.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <sys/sysctl.h>

std::vector<double> double_cache_aware_matmul(const std::vector<double> &m1,
                                              const std::vector<double> &m2,
                                              int row_dim, int col_dim,
                                              int common_dim) {
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

    std::vector<double> result = matmul::make_result(m1, m2, row_dim, col_dim, common_dim);

    for (int si2 = 0; si2 < row_dim; si2 += B_L2) {
        const int i2_max = std::min(si2 + B_L2, row_dim);
        for (int sk2 = 0; sk2 < common_dim; sk2 += B_L2) {
            const int k2_max = std::min(sk2 + B_L2, common_dim);
            for (int sj2 = 0; sj2 < col_dim; sj2 += B_L2) {
                const int j2_max = std::min(sj2 + B_L2, col_dim);

                // Every L1 tile is bounded by its containing L2 tile.
                for (int si1 = si2; si1 < i2_max; si1 += B_L1) {
                    const int i1_max = std::min(si1 + B_L1, i2_max);
                    for (int sk1 = sk2; sk1 < k2_max; sk1 += B_L1) {
                        const int k1_max = std::min(sk1 + B_L1, k2_max);
                        for (int sj1 = sj2; sj1 < j2_max; sj1 += B_L1) {
                            const int j1_max = std::min(sj1 + B_L1, j2_max);
                            for (int i = si1; i < i1_max; ++i) {
                                for (int k = sk1; k < k1_max; ++k) {
                                    const double left = m1[i * common_dim + k];
                                    for (int j = sj1; j < j1_max; ++j) {
                                        result[i * col_dim + j] += left * m2[k * col_dim + j];
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return result;
}
