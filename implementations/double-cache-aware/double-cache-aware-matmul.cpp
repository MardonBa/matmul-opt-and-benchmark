#include "double-cache-aware-matmul.h"

#include <algorithm>

matmul::Matrix double_cache_aware_multiply(const matmul::Matrix &m1,
                                           const matmul::Matrix &m2, int row_dim,
                                           int col_dim, int common_dim) {
    constexpr int kOuterTileSize = 256;
    constexpr int kInnerTileSize = 64;
    matmul::Matrix result = matmul::make_result(m1, m2, row_dim, col_dim, common_dim);

    for (int si_outer = 0; si_outer < row_dim; si_outer += kOuterTileSize) {
        for (int sk_outer = 0; sk_outer < common_dim; sk_outer += kOuterTileSize) {
            for (int sj_outer = 0; sj_outer < col_dim; sj_outer += kOuterTileSize) {
                const int i_outer_max = std::min(si_outer + kOuterTileSize, row_dim);
                const int k_outer_max = std::min(sk_outer + kOuterTileSize, common_dim);
                const int j_outer_max = std::min(sj_outer + kOuterTileSize, col_dim);
                for (int si = si_outer; si < i_outer_max; si += kInnerTileSize) {
                    for (int sk = sk_outer; sk < k_outer_max; sk += kInnerTileSize) {
                        for (int sj = sj_outer; sj < j_outer_max; sj += kInnerTileSize) {
                            const int i_max = std::min(si + kInnerTileSize, i_outer_max);
                            const int k_max = std::min(sk + kInnerTileSize, k_outer_max);
                            const int j_max = std::min(sj + kInnerTileSize, j_outer_max);
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
            }
        }
    }
    return result;
}
