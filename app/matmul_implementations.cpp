#include "matmul_implementations.h"
#include "implementations/ikj/ikj_matmul.h"
#include "implementations/jik/jik_matmul.h"
#include "implementations/jki/jki_matmul.h"
#include "implementations/kij/kij_matmul.h"
#include "implementations/kji/kji_matmul.h"
#include "implementations/naive/naive_matmul.h"
#include "implementations/cache_aware/cache_aware_matmul.h"
#include "implementations/double_cache_aware/double_cache_aware_matmul.h"

const std::vector<MatmulImplementation> &matmul_implementations() {
    static const std::vector<MatmulImplementation> implementations{
        MatmulImplementation{"naive", naive_multiply},
        MatmulImplementation{"ikj", ikj_multiply},
        MatmulImplementation{"jik", jik_multiply},
        MatmulImplementation{"jki", jki_multiply},
        MatmulImplementation{"kji", kji_multiply},
        MatmulImplementation{"kij", kij_multiply},
        MatmulImplementation{"cache aware", cache_aware_matmul},
        MatmulImplementation{"double cache aware", double_cache_aware_matmul}
    };
    return implementations;
}
