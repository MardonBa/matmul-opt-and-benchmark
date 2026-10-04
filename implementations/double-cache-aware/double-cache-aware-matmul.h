#ifndef DOUBLE_CACHE_AWARE_MATMUL_H
#define DOUBLE_CACHE_AWARE_MATMUL_H

#include "implementations/matrix.h"

matmul::Matrix double_cache_aware_multiply(const matmul::Matrix &m1,
                                           const matmul::Matrix &m2, int row_dim,
                                           int col_dim, int common_dim);

#endif
