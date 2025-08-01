#include "new_delete.hpp"

namespace app_namespace {

bool test(int a, int b) {
    int *p_sum = new_ptr(a + b);
    bool ok = *p_sum == a + b;
    delete_ptr(p_sum);
    return ok;
}

}  // namespace app_namespace
