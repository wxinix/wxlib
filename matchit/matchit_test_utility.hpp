#ifndef WXLIB_MATCHIT_TEST_UTILITY_HPP
#define WXLIB_MATCHIT_TEST_UTILITY_HPP

struct A
{
  int32_t a;
  int32_t b;
};

namespace std {

template<size_t I>
constexpr auto &get(A const &a)
{
  if constexpr (I == 0) {
    return a.a;
  } else if constexpr (I == 1) {
    return a.b;
  }
}

}

#include <matchit/matchit.hpp>

using namespace matchit;

class Base
{
public:
  virtual int32_t index() const = 0;
  virtual ~Base() = default;
};

class Derived : public Base
{
public:
  int32_t index() const override
  { return 1; }
};

template<typename Range1, typename Range2>
auto expect_range(Range1 const &result, Range2 const &expected)
{
  CHECK_EQ(result.size(), expected.size());

  auto b1 = result.begin();
  auto b2 = expected.begin();

  for (; b1 != result.end() && b2 != expected.end(); ++b1, ++b2) {
    using impl::operator==;
    bool eq = (*b1) == (*b2);
    CHECK(eq);
  }
}

template<typename Range>
constexpr bool recursive_symmetric(Range const &range)
{
  Id<int32_t> i;
  Id<SubrangeT<Range const>> subrange;
  return match(range)(
      //@formatter:off
      pattern | ds(i, subrange.at(ooo), i) = [&] { return recursive_symmetric(*subrange); },
      pattern | ds(i, subrange.at(ooo), _) = expr(false),
      pattern | _                          = expr(true)
      //@formatter:on
  );
}

std::tuple<> xxx();

bool func1()
{
  return true;
}

int64_t func2()
{
  return 12;
}

bool operator==(A const lhs, A const rhs)
{
  return lhs.a == rhs.a && lhs.b == rhs.b;
}

enum class Kind
{
  kOne,
  kTwo
};

class Num
{
public:
  virtual ~Num() = default;
  virtual Kind kind() const = 0;
};

class One : public Num
{
public:
  constexpr static auto k = Kind::kOne;
  Kind kind() const override
  {
    return k;
  }
};

class Two : public Num
{
public:
  constexpr static auto k = Kind::kTwo;
  Kind kind() const override
  {
    return k;
  }
};

template<Kind k>
constexpr auto kind = app(&Num::kind, k);

template<typename T>
auto get_if(Num const *num)
{
  return static_cast<T const *>(num->kind() == T::k ? num : nullptr);
}

struct Shape
{
  virtual bool is() const = 0;
  virtual ~Shape() = default;
};

struct Circle : Shape
{
  bool is() const final
  {
    return false;
  }
};

struct Square : Shape
{
  bool is() const final
  {
    return false;
  }
};

bool operator==(Shape const &, Shape const &)
{
  return true;
}

namespace std {

template<>
class tuple_size<A> : public std::integral_constant<size_t, 2>
{
};

} // namespace std

#endif