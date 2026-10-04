#include "implementations/ikj/ikj_matmul.h"
#include "implementations/jik/jik_matmul.h"
#include "implementations/jki/jki_matmul.h"
#include "implementations/kij/kij_matmul.h"
#include "implementations/kji/kji_matmul.h"
#include "implementations/naive/naive_matmul.h"

#include <cmath>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

using MatmulFunction = double **(*)(double **left, double **right, int rows,
                                    int columns, int shared_dimension);

double **make_matrix(const std::vector<std::vector<double>> &values) {
    double **matrix = new double *[values.size()];
    for (std::size_t row = 0; row < values.size(); ++row) {
        matrix[row] = new double[values[row].size()];
        for (std::size_t column = 0; column < values[row].size(); ++column) {
            matrix[row][column] = values[row][column];
        }
    }
    return matrix;
}

void free_matrix(double **matrix, int rows) {
    for (int row = 0; row < rows; ++row) delete[] matrix[row];
    delete[] matrix;
}

bool verify_implementation(const std::string &name, MatmulFunction multiply,
                           double **left, double **right) {
    constexpr int kRows = 2;
    constexpr int kShared = 3;
    constexpr int kColumns = 2;
    const double expected[kRows][kColumns] = {{58.0, 64.0}, {139.0, 154.0}};
    double **result = multiply(left, right, kRows, kColumns, kShared);
    bool correct = true;
    for (int row = 0; row < kRows; ++row) {
        for (int column = 0; column < kColumns; ++column) {
            if (std::fabs(result[row][column] - expected[row][column]) > 1e-12) {
                std::cerr << name << " produced " << result[row][column] << " at ["
                          << row << "][" << column << "], expected "
                          << expected[row][column] << '\n';
                correct = false;
            }
        }
    }
    free_matrix(result, kRows);
    return correct;
}

int main() {
    double **left = make_matrix({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
    double **right = make_matrix({{7.0, 8.0}, {9.0, 10.0}, {11.0, 12.0}});
    const std::vector<std::pair<std::string, MatmulFunction>> implementations = {
        {"naive", naive_multiply}, {"ikj", ikj_multiply}, {"jik", jik_multiply},
        {"jki", jki_multiply}, {"kij", kij_multiply}, {"kji", kji_multiply},
    };

    bool correct = true;
    for (const auto &[name, multiply] : implementations) {
        correct = verify_implementation(name, multiply, left, right) && correct;
    }
    free_matrix(left, 2);
    free_matrix(right, 3);
    return correct ? 0 : 1;
}
