#include "../../matchit/matchit.hpp"
#include <iostream>

enum class Kind
{
  kOne,
  kTwo
};

#if __cplusplus > 201703L
#define CPP20_CONSTEXPR constexpr
#else
#define CPP20_CONSTEXPR
#endif

class Num
{
public:
  CPP20_CONSTEXPR virtual Kind kind() const = 0;

protected:
  ~Num() = default;
};

class One : public Num
{
public:
  constexpr static auto k = Kind::kOne;
  CPP20_CONSTEXPR Kind kind() const override
  {
    return k;
  }
};

class Two : public Num
{
public:
  constexpr static auto k = Kind::kTwo;
  CPP20_CONSTEXPR Kind kind() const override
  {
    return k;
  }
};

template<Kind k>
constexpr auto kind = matchit::app(&Num::kind, k);

namespace std {

template<typename T>
auto get_if(Num const *num)
{
  std::cout << "get_if" << std::endl;
  return static_cast<T const *>(num->kind() == T::k ? num : nullptr);
}

}

constexpr int32_t static_cast_as(Num const &input)
{
  using namespace matchit;

  return match(input)(
      pattern | as<One>(_) = expr(1),
      pattern | kind<Kind::kTwo> = expr(2),
      pattern | _ = expr(3)
  );
}

#if 0 // fail on gcc, fix me later.
#if __cplusplus > 201703L
static_assert(staticCastAs(One{}) == 1);
#endif
#endif

int32_t main()
{
  std::cout << static_cast_as(One{}) << std::endl;
  return 0;
}
