#include "runtime.hpp"

namespace NAMESPACE::lsmtree::rt {
using namespace NAMESPACE;

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<double> dist(0.0, 1.0);

double rand() {
    double v = 0;
    if (!is_validator()) { // raw or app
        v = dist(gen);
    }
    return external_return(v);
    // return v;
}

}
