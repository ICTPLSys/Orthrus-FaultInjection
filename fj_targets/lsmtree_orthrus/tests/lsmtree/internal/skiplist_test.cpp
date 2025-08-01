#include <cstdlib>
#include <iostream>

#include "context.hpp"
#include "ctltypes.hpp"
#include "custom_stl.hpp"
#include "log.hpp"
#include "memtable.hpp"
#include "namespace.hpp"
#include "ptr.hpp"
#include "scee.hpp"
#include "thread.hpp"

namespace app {
using namespace raw::lsmtree;
auto main(MemTable* tbl, ValueT v) {
    ValueT value = 0;
    tbl->Set(v, v);
    value = tbl->Get(v);
    printf("app: %ld\n", value);
    return value;
}
}  // namespace app

namespace validator {
using namespace raw::lsmtree;
auto main(MemTable* tbl, ValueT v) {
    ValueT value = 0;
    tbl->Set(v, v);
    value = tbl->Get(v);
    printf("val: %ld\n", value);
    return value;
}
}  // namespace validator

int main_fn() {
    using namespace raw::lsmtree;
    MemTable tbl;
    for (int i = 1; i <= 100; i++) {
        scee::run2(app::main, validator::main, &tbl, (ValueT)i);
    }
    sleep(2);
    return 0;
}

int main() {
    scee::main_thread(main_fn);
    return 0;
}