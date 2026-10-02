#include <iostream>
#include <array>
#include "naive_matmul.h"

void fill_matrix(double **array, size_t rows, size_t cols, std::string m_name) {
    // Don't need to return anything here because we're passing through pointers, so the matrix is updated inplace
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            double val;
            std::cout << "What is the value of " + m_name + " at index " + std::to_string(i) + ", " + std::to_string(j) + "? ";
            std::cin >> val;
            array[i][j] = val;
        }
    }
}

void print_matrix(double **array, size_t rows, size_t cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            std::cout << std::to_string(array[i][j]) + " ";
        }
        std::cout << "\n";
    }
}


int main() {
  // get dimensions on m1
  std::cout << "what are the dimensions of m1?\n";
  int m1_dim1;
  int m1_dim2;
  std::cout << "m1 dim1: ";
  std::cin >> m1_dim1;
  std::cout << "m1 dim2: ";
  std::cin >> m1_dim2;
  // get dimensions on m2
  std::cout << "\nwhat are the dimensions of m2?\n";
  int m2_dim1;
  int m2_dim2;
  std::cout << "m2 dim1: ";
  std::cin >> m2_dim1;
  std::cout << "m2 dim2: ";
  std::cin >> m2_dim2;
  // throw error if they don't match
  if (m1_dim2 != m2_dim1) {
    throw std::runtime_error("\nMatrix dimensions (m1 dim2 and m2 dim1) didn't match");
  }

  // get values of m1
  double **m1 = new double*[m1_dim1];
  for (size_t i = 0; i < m1_dim1; i++) {
    m1[i] = new double[m1_dim2];
  }
  fill_matrix(m1, m1_dim1, m1_dim2, "m1");
  std::cout << "m1:\n";
  print_matrix(m1, m1_dim1, m1_dim2);

  // get values of m2
  double **m2 = new double*[m1_dim1];
  for (size_t i = 0; i < m2_dim1; i++) {
    m2[i] = new double[m2_dim2];
  }
  fill_matrix(m2, m2_dim1, m2_dim2, "m2");
  print_matrix(m2, m2_dim1, m2_dim2);

  // multiply
  std::cout << "\nmultiplying...\n";
  double **res = naive_multiply(m1, m2, m1_dim1, m2_dim2, m1_dim2);
  print_matrix(res, m1_dim1, m2_dim2);
}