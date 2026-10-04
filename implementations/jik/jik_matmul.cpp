#include "jik_matmul.h"

double **jik_multiply(double **m1, double **m2, int row_dim, int col_dim, int common_dim) {
    double **result = new double *[row_dim];
    for (int i = 0; i < row_dim; ++i) {
        result[i] = new double[col_dim];
    }

    for (int j = 0; j < col_dim; ++j) {
        for (int i = 0; i < row_dim; ++i) {
            double sum = 0.0;
            for (int k = 0; k < common_dim; ++k) {
                sum += m1[i][k] * m2[k][j];
            }
            result[i][j] = sum;
        }
    }
    return result;
}
