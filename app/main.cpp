#include "implementations/naive/naive_matmul.h"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void fill_matrix(matmul::Matrix &matrix, int rows, int columns, const std::string &name) {
    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            std::cout << "What is the value of " << name << " at index " << row << ", "
                      << column << "? ";
            std::cin >> matrix[row * columns + column];
        }
    }
}

void print_matrix(const matmul::Matrix &matrix, int rows, int columns) {
    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            std::cout << matrix[row * columns + column] << ' ';
        }
        std::cout << '\n';
    }
}

}  // namespace

int main() {
    int m1_rows, m1_columns, m2_rows, m2_columns;
    std::cout << "What are the dimensions of m1?\n";
    std::cout << "m1 rows: "; std::cin >> m1_rows;
    std::cout << "m1 columns: "; std::cin >> m1_columns;
    std::cout << "\nWhat are the dimensions of m2?\n";
    std::cout << "m2 rows: "; std::cin >> m2_rows;
    std::cout << "m2 columns: "; std::cin >> m2_columns;
    if (m1_columns != m2_rows) {
        throw std::runtime_error("m1 columns must equal m2 rows");
    }

    matmul::Matrix m1(matmul::element_count(m1_rows, m1_columns));
    matmul::Matrix m2(matmul::element_count(m2_rows, m2_columns));
    fill_matrix(m1, m1_rows, m1_columns, "m1");
    fill_matrix(m2, m2_rows, m2_columns, "m2");

    const matmul::Matrix result = naive_multiply(m1, m2, m1_rows, m2_columns, m1_columns);
    std::cout << "\nResult:\n";
    print_matrix(result, m1_rows, m2_columns);
}
