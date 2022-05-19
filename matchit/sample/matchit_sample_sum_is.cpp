#include "../../matchit/matchit.hpp"
#include <array>
#include <iostream>

constexpr bool sumIs(std::array<int32_t, 2> const &arr, int32_t s)
{
  using namespace matchit;
  Id<int32_t> i, j;
  return match(arr)(
      // clang-format off
        pattern | ds(i, j) | when(i + j == s) = expr(true),
        pattern | _                       = expr(false)
      // clang-format on
  );
}

static_assert(sumIs(std::array<int32_t, 2>{5, 6}, 11));

int32_t main()
{
  std::cout << sumIs(std::array<int32_t, 2>{5, 6}, 11) << std::endl;
  return 0;
}
