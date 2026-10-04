#ifndef NAIVE_MATMUL_H
#define NAIVE_MATMUL_H

#include "implementations/matrix.h"

matmul::Matrix naive_multiply(const matmul::Matrix &m1, const matmul::Matrix &m2,
                              int row_dim, int col_dim, int common_dim);

#endif
