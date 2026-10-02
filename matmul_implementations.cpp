#include "matmul_implementations.h"
#include "naive_matmul.h"
#include "loop_order_matmul.h"

const std::vector<MatmulImplementation> &matmul_implementations() {
    static const std::vector<MatmulImplementation> implementations = {
        {"naive", naive_multiply},
        {"ikj", ikj_multiply},
        {"jik", jik_multiply},
        {"jki", jki_multiply},
        {"kji", kji_multiply},
        {"kij", kij_multiply}
    };
    return implementations;
}
