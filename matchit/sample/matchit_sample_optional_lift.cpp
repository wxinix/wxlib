#include "../../matchit/matchit.hpp"
#include <iostream>
#include <optional>

// lift a function from T -> U to std::optional<T> -> std::optional<U>
template <typename Func>
auto optionalLift(Func func)
{
  using namespace matchit;
  return [func](auto &&v)
  {
    Id<std::decay_t<decltype(*v)>> x;
    using RetType = decltype(std::make_optional(func(*x)));
    return match(v)(
        // clang-format off
            pattern | some(x) = [&] { return std::make_optional(func(*x)); },
            pattern | none    = expr(RetType{})
        // clang-format on
    );
  };
}

int32_t main()
{
  auto const func = [](auto &&e)
  { return e * e; };
  auto const result = optionalLift(func)(std::make_optional(2));
  std::cout << *result << std::endl;
  return 0;
}
