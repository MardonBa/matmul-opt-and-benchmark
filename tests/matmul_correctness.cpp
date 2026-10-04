#include "implementations/cache-aware/cache-aware-matmul.h"
#include "implementations/double-cache-aware/double-cache-aware-matmul.h"
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

using MatmulFunction = matmul::Matrix (*)(const matmul::Matrix &left,
                                          const matmul::Matrix &right, int rows,
                                          int columns, int shared_dimension);

bool verify_implementation(const std::string &name, MatmulFunction multiply,
                           const matmul::Matrix &left, const matmul::Matrix &right) {
    constexpr int kRows = 2;
    constexpr int kShared = 3;
    constexpr int kColumns = 2;
    const matmul::Matrix expected = {58.0, 64.0, 139.0, 154.0};
    const matmul::Matrix result = multiply(left, right, kRows, kColumns, kShared);
    if (result.size() != expected.size()) {
        std::cerr << name << " returned " << result.size() << " elements, expected "
                  << expected.size() << '\n';
        return false;
    }

    bool correct = true;
    for (std::size_t index = 0; index < expected.size(); ++index) {
        if (std::fabs(result[index] - expected[index]) > 1e-12) {
            const int row = static_cast<int>(index) / kColumns;
            const int column = static_cast<int>(index) % kColumns;
            std::cerr << name << " produced " << result[index] << " at [" << row << "]["
                      << column << "], expected " << expected[index] << '\n';
            correct = false;
        }
    }
    return correct;
}

int main() {
    const matmul::Matrix left = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
    const matmul::Matrix right = {7.0, 8.0, 9.0, 10.0, 11.0, 12.0};
    const std::vector<std::pair<std::string, MatmulFunction>> implementations = {
        {"naive", naive_multiply}, {"ikj", ikj_multiply}, {"jik", jik_multiply},
        {"jki", jki_multiply}, {"kij", kij_multiply}, {"kji", kji_multiply},
        {"cache-aware", cache_aware_multiply},
        {"double-cache-aware", double_cache_aware_multiply},
    };

    bool correct = true;
    for (const auto &[name, multiply] : implementations) {
        correct = verify_implementation(name, multiply, left, right) && correct;
    }
    return correct ? 0 : 1;
}
