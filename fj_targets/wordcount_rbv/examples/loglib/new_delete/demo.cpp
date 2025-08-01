#include <cassert>
#include <cstdio>

#include "log.hpp"
#include "new_delete.hpp"
#include "scee.hpp"

namespace app {
#include "test_fn.hpp"
}  // namespace app
namespace validator {
#include "test_fn.hpp"
}  // namespace validator

void run() {
    // app run
    auto x = scee::run2(app::test, validator::test, 1, 2);
    // validation
    scee::LogHead *log =
        static_cast<scee::LogHead *>(scee::log_dequeue(&scee::log_queue));
    assert(log != nullptr);
    scee::validate_one(log);
}

int main() {
    run();
    return 0;
}
