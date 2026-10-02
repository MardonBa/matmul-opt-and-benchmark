#include <cstdlib>

double **naive_multiply(double **m1, double **m2, int row_dim, int col_dim, int common_dim) {
    double **res = new double*[row_dim];
    for (size_t i = 0; i < row_dim; i++) {
        res[i] = new double[col_dim];
    }

    for (size_t i = 0; i < row_dim; i++) {
        for (size_t j = 0; j < col_dim; j++) {
            res[i][j] = 0.0;

            for (size_t k = 0; k < common_dim; k++) {
                res[i][j] += m1[i][k] * m2[k][j];
            }
        }
    }
    return res;
}