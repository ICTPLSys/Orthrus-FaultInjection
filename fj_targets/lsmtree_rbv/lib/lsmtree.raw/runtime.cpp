#include <iostream>
#include <numeric>
#include "runtime.hpp"

// namespace NAMESPACE::lsmtree::rt {
// namespace mt {
// #include "mersenne-twister.h"
// struct gen {
//     gen() {
//         mt::seed(114514);
//     }

//     uint32_t rand() {
//         return mt::rand_u32();
//     }
// } g_rnd;

// }

// using namespace NAMESPACE;

// // std::random_device rd;
// // std::mt19937 gen(rd());
// // std::uniform_real_distribution<double> dist(0.0, 1.0);

// uint64_t rand() {
//     uint64_t v = 0;
//     if (!is_validator()) { // raw or app
//         v = mt::g_rnd.rand();
//     }
//     return external_return(v);
//     // return v;
// }

// }
