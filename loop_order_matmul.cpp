#include <cstdlib>
double **ikj_multiply(double **m1, double **m2, int row_dim, int col_dim, int common_dim) {
    double **res = new double*[row_dim];
    for (size_t i = 0; i < row_dim; i++) {
        res[i] = new double[col_dim];
        // Initialization needs to get done in here
        for (size_t j = 0; j < col_dim; j++) {
            res[i][j] = 0.0;
        }
    }

    for (int i = 0; i < row_dim; i++) {
        for (int k = 0; k < common_dim; k++) {
            // store this once in register, that way we don't have to look it up in each inner loop iteration
            // applies to all the rest down below (when we apply it)
            double r = m1[i][k];
            for (int j = 0; j < col_dim; j++) {
                res[i][j] += r * m2[k][j];
            }
        }
    }

    return res;
}

double **jik_multiply(double **m1, double **m2, int row_dim, int col_dim, int common_dim) {
    double **res = new double*[row_dim];
    for (size_t i = 0; i < row_dim; i++) {
        res[i] = new double[col_dim];
    }

    for (int j = 0; j < col_dim; j++) {
        for (int i = 0; i < row_dim; i++) {
            res[i][j] = 0.0;

            for (int k = 0; k < common_dim; k++) {
                res[i][j] += m1[i][k] * m2[k][j];
            }
        }
    }

    return res;
}

double **jki_multiply(double **m1, double **m2, int row_dim, int col_dim, int common_dim) {
    double **res = new double*[row_dim];
    for (size_t i = 0; i < row_dim; i++) {
        res[i] = new double[col_dim];
        
        for (size_t j = 0; j < col_dim; j++) {
            res[i][j] = 0;
        }
    }

    for (int j = 0; j < col_dim; j++) {
        for (int k = 0; k < common_dim; k++) {
            double r = m2[k][j];
            for (int i = 0; i < row_dim; i++) {
                res[i][j] += m1[i][k] * r;
            }
        }
    }

    return res;
}

double **kji_multiply(double **m1, double **m2, int row_dim, int col_dim, int common_dim) {
    double **res = new double*[row_dim];
    for (size_t i = 0; i < row_dim; i++) {
        res[i] = new double[col_dim];
        
        for (size_t j = 0; j < col_dim; j++) {
            res[i][j] = 0;
        }
    }

    for (int k = 0; k < common_dim; k++) {
        for (int j = 0; j < col_dim; j++) {
            double r = m2[k][j];

            for (int i = 0; i < row_dim; i++) {
                res[i][j] += m1[i][k] * r;
            }
        }
    }

    return res;
}

double **kij_multiply(double **m1, double **m2, int row_dim, int col_dim, int common_dim) {
    double **res = new double*[row_dim];
    for (size_t i = 0; i < row_dim; i++) {
        res[i] = new double[col_dim];
        
        for (size_t j = 0; j < col_dim; j++) {
            res[i][j] = 0;
        }
    }

    for (int k = 0; k < common_dim; k++) {
        for (int i = 0; i < row_dim; i++) {
            double r = m1[i][k];

            for (int j = 0; j < col_dim; j++) {
                res[i][j] += r * m2[k][j];
            }
        }
    }

    return res;
}