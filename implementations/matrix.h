#ifndef MATMUL_MATRIX_H
#define MATMUL_MATRIX_H

#include <cstddef>
#include <stdexcept>
#include <vector>

namespace matmul {

using Matrix = std::vector<double>;

// Matrices are row-major: element (row, column) is at row * columns + column.
inline std::size_t element_count(int rows, int columns) {
    if (rows < 0 || columns < 0) {
        throw std::invalid_argument("Matrix dimensions must be non-negative");
    }
    return static_cast<std::size_t>(rows) * static_cast<std::size_t>(columns);
}

inline Matrix make_result(const Matrix &left, const Matrix &right, int rows,
                          int columns, int shared_dimension) {
    if (shared_dimension < 0) {
        throw std::invalid_argument("Matrix dimensions must be non-negative");
    }
    if (left.size() != element_count(rows, shared_dimension) ||
        right.size() != element_count(shared_dimension, columns)) {
        throw std::invalid_argument("Matrix storage does not match its dimensions");
    }
    return Matrix(element_count(rows, columns), 0.0);
}

}  // namespace matmul

#endif
