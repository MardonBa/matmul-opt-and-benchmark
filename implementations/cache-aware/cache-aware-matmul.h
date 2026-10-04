#ifndef CACHE_AWARE_MATMUL_H
#define CACHE_AWARE_MATMUL_H

double **cache_aware_matmul(double **m1, double **m2, int row_dim, int col_dim, int common_dim, int block_size);

#endif