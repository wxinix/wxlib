#include "../../matchit/matchit.hpp"
#include <iostream>

constexpr bool check_and_log_large(double value)
{
  using namespace matchit;

  auto const square = [](auto &&v) { return v * v; };
  Id<double> s;

  return match(value)(
      pattern | app(square, and_(_ > 1000, s)) = [&] {
        std::cout << value << "^2 = " << *s << " > 1000!" << std::endl;
        return true;
      },
      pattern | _ = expr(false));
}

int32_t main()
{
  std::cout << check_and_log_large(100) << std::endl;
  return 0;
}
