#include "matmul_implementations.h"
#include "implementations/cache-aware/cache-aware-matmul.h"
#include "implementations/double-cache-aware/double-cache-aware-matmul.h"
#include "implementations/ikj/ikj_matmul.h"
#include "implementations/jik/jik_matmul.h"
#include "implementations/jki/jki_matmul.h"
#include "implementations/kij/kij_matmul.h"
#include "implementations/kji/kji_matmul.h"
#include "implementations/naive/naive_matmul.h"

const std::vector<MatmulImplementation> &matmul_implementations() {
    static const std::vector<MatmulImplementation> implementations = {
        {"naive", naive_multiply},
        {"ikj", ikj_multiply},
        {"jik", jik_multiply},
        {"jki", jki_multiply},
        {"kji", kji_multiply},
        {"kij", kij_multiply},
        {"cache-aware", cache_aware_multiply},
        {"double-cache-aware", double_cache_aware_multiply},
    };
    return implementations;
}
