#include <cassert>
#include <iostream>

namespace Exec {
#include "common.hpp"
}

namespace Check {
#include "common.hpp"
}

int main() {
  assert(Exec::prt(10) == Check::prt(10));
  assert(Exec::prt(1) == Check::prt(1));
  assert(Exec::prt(2) == Check::prt(2));

  return 0;
}
