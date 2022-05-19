#include "../../matchit/matchit.hpp"
#include <iostream>

constexpr bool isValid(int32_t n)
{
  using namespace matchit;
  return match(n)(
      // clang-format off
        pattern | or_(1, 3, 5) = expr(true),
        pattern | _            = expr(false)
      // clang-format on
  );
}

static_assert(isValid(5));
static_assert(!isValid(6));

int32_t main()
{
  std::cout << isValid(3) << std::endl;
  return 0;
}
