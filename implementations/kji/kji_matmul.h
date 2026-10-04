#ifndef KJI_MATMUL_H
#define KJI_MATMUL_H

#include "implementations/matrix.h"

matmul::Matrix kji_multiply(const matmul::Matrix &m1, const matmul::Matrix &m2,
                            int row_dim, int col_dim, int common_dim);

#endif
