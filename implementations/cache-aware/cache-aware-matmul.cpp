#include "cache-aware-matmul.h"

#include <algorithm>

matmul::Matrix cache_aware_multiply(const matmul::Matrix &m1, const matmul::Matrix &m2,
                                    int row_dim, int col_dim, int common_dim) {
    constexpr int kTileSize = 64;
    matmul::Matrix result = matmul::make_result(m1, m2, row_dim, col_dim, common_dim);

    for (int si = 0; si < row_dim; si += kTileSize) {
        for (int sk = 0; sk < common_dim; sk += kTileSize) {
            for (int sj = 0; sj < col_dim; sj += kTileSize) {
                const int i_max = std::min(si + kTileSize, row_dim);
                const int k_max = std::min(sk + kTileSize, common_dim);
                const int j_max = std::min(sj + kTileSize, col_dim);
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
