#ifndef LOOP_ORDER_MATMUL
#define LOOP_ORDER_MATMUL

// ijk is just the naive multiply
double **ikj_multiply(double **m1, double **m2, int row_dim, int col_dim, int common_dim);
double **jik_multiply(double **m1, double **m2, int row_dim, int col_dim, int common_dim);
double **jki_multiply(double **m1, double **m2, int row_dim, int col_dim, int common_dim);
double **kji_multiply(double **m1, double **m2, int row_dim, int col_dim, int common_dim);
double **kij_multiply(double **m1, double **m2, int row_dim, int col_dim, int common_dim);

#endif