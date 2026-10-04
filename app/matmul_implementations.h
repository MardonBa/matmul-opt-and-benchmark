#ifndef MATMUL_IMPLEMENTATIONS_H
#define MATMUL_IMPLEMENTATIONS_H

#include <string>
#include <vector>

// All implementations use the same row-pointer matrix representation. Add a
// new implementation here so the benchmark runner can discover it by name.
using MatmulFunction = double **(*)(double **left, double **right, int rows,
                                    int columns, int shared_dimension);

struct MatmulImplementation {
    std::string name;
    MatmulFunction multiply;
};

const std::vector<MatmulImplementation> &matmul_implementations();

#endif
