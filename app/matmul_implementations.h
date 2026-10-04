#ifndef MATMUL_IMPLEMENTATIONS_H
#define MATMUL_IMPLEMENTATIONS_H

#include <string>
#include <vector>
#include "implementations/matrix.h"

// All implementations use contiguous row-major storage. Add a new
// implementation here so the benchmark runner can discover it by name.
using MatmulFunction = matmul::Matrix (*)(const matmul::Matrix &left,
                                          const matmul::Matrix &right, int rows,
                                          int columns, int shared_dimension);

struct MatmulImplementation {
    std::string name;
    MatmulFunction multiply;
};

const std::vector<MatmulImplementation> &matmul_implementations();

#endif
