#include "kji_matmul.h"

double **kji_multiply(double **m1, double **m2, int row_dim, int col_dim, int common_dim) {
    double **result = new double *[row_dim];
    for (int i = 0; i < row_dim; ++i) {
        result[i] = new double[col_dim]{};
    }

    for (int k = 0; k < common_dim; ++k) {
        for (int j = 0; j < col_dim; ++j) {
            const double right = m2[k][j];
            for (int i = 0; i < row_dim; ++i) {
                result[i][j] += m1[i][k] * right;
            }
        }
    }
    return result;
}
