#include "ikj_matmul.h"

matmul::Matrix ikj_multiply(const matmul::Matrix &m1, const matmul::Matrix &m2,
                            int row_dim, int col_dim, int common_dim) {
    matmul::Matrix result = matmul::make_result(m1, m2, row_dim, col_dim, common_dim);

    for (int i = 0; i < row_dim; ++i) {
        for (int k = 0; k < common_dim; ++k) {
            const double left = m1[i * common_dim + k];
            for (int j = 0; j < col_dim; ++j) {
                result[i * col_dim + j] += left * m2[k * col_dim + j];
            }
        }
    }
    return result;
}
