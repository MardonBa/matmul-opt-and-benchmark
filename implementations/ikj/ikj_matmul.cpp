#include "ikj_matmul.h"

double **ikj_multiply(double **m1, double **m2, int row_dim, int col_dim, int common_dim) {
    double **result = new double *[row_dim];
    for (int i = 0; i < row_dim; ++i) {
        result[i] = new double[col_dim]{};
    }

    for (int i = 0; i < row_dim; ++i) {
        for (int k = 0; k < common_dim; ++k) {
            const double left = m1[i][k];
            for (int j = 0; j < col_dim; ++j) {
                result[i][j] += left * m2[k][j];
            }
        }
    }
    return result;
}
