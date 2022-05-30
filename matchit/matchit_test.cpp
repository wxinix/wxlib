/*!
  MPL 1.1/GPL 2.0/LGPL 2.1 tri-license
  Copyright (C) 2022  Wuping Xin
*/

/*!
  Apache-2.0 License
  Copyright (c) 2021 Bowen Fu
*/

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>
#include "matchit_test_utility.hpp"
#include <matchit/matchit.hpp>

using namespace matchit;

TEST_CASE("scenario: pattern app")
{
  SUBCASE("test some") {
    constexpr auto deref = [](auto &&x) -> decltype(*x) { return *x; };

    static_assert(std::is_same_v<
        impl::PatternTraits<impl::App<decltype(deref), impl::Wildcard>>::template AppResultTuple<Base *>,
        std::tuple<>>);

    static_assert(std::is_same_v<
        impl::PatternTraits<impl::App<decltype(deref), impl::Wildcard>>::template AppResult<Base *>,
        Base &>);

    auto const x = std::unique_ptr<Base>{new Derived};
    CHECK(matched(x, some(as<Derived>(_))));
  }
}

TEST_CASE("scenario: pattern id")
{
  SUBCASE("test match_value") {
    Id<int32_t> x;
    x.match_value(1);
    CHECK_EQ(*x, 1);
  }

  SUBCASE("test reset id") {
    Id<int32_t> x;

    x.match_value(1);
    CHECK_EQ(*x, 1);

    x.reset(1);
    CHECK_EQ(*x, 1);

    x.reset(0);
    CHECK_FALSE(x.has_value());
  }

  SUBCASE("test reset after failure") {
    Id<int32_t> x;

    match(10)(
        pattern | x = [&] { CHECK_EQ(*x, 10); }
    );

    auto const matched = match(10)(
        pattern | not_(x) = expr(true),
        pattern | _ = expr(false)
    );

    CHECK_FALSE(matched);
  }

  SUBCASE("test reset after failure 2") {
    Id<int32_t> x;

    match(10)(
        pattern | x = [&] { CHECK_EQ(*x, 10); }
    );

    auto const matched = match(10)(
        pattern | and_(x, not_(x)) = expr(true),
        pattern | _ = expr(false)
    );

    CHECK_FALSE(matched);
  }

  SUBCASE("test reset after failure 3") {
    Id<int32_t> x;

    auto result = match(10)(
        pattern | and_(x, app(_ / 2, x)) = expr(true),
        pattern | _ = expr(false)
    );

    CHECK_FALSE(result);

    result = match(10)(
        pattern | and_(x, app(_ / 2, not_(x))) = [&] {
          CHECK_EQ(*x, 10);
          return true;
        },
        pattern | _ = expr(false));

    CHECK(result);
  }

  SUBCASE("test reset after failure 3-3") {
    Id<int32_t> x;

    auto result = match(10)(
        pattern | or_(and_(not_(x), not_(x)), app(_ / 2, x)) = [&] {
          CHECK_EQ(*x, 5);
          return true;
        },
        pattern | _ = expr(false)
    );

    CHECK(result);

    result = match(10)(
        pattern | or_(and_(x, not_(x)), app(_ / 2, x)) =
            [&] {
              CHECK_EQ(*x, 5);
              return true;
            },
        pattern | _ = expr(false)
    );

    CHECK(result);

    result = match(10)(
        pattern | or_(and_(not_(x), x), app(_ / 2, x)) =
            [&] {
              CHECK_EQ(*x, 5);
              return true;
            },
        pattern | _ = expr(false)
    );

    CHECK(result);
  }

  SUBCASE("test reset after failure 4") {
    Id<int32_t> x;

    auto const matched = match(std::make_tuple(10, 20))(
        pattern | or_(
            // first / 5 == second / 2 + 1
            ds(app(_ / 5, x), app(_ / 2 + 1, x)),
            // first / 2 == second / 5 + 1
            ds(app(_ / 2, x), app(_ / 5 + 1, x))) =
            [&] {
              CHECK_EQ(*x, 5);
              return true;
            },
        pattern | _ = expr(false));

    CHECK(matched);
  }

  SUBCASE("test reset after failure 5") {
    Id<int32_t> x;

    auto result = match(10)(
        pattern | and_(and_(or_(x)), and_(10)) =
            [&] {
              CHECK_EQ(*x, 10);
              return true;
            },
        pattern | _ = expr(false)
    );

    CHECK(result);

    result = match(10)(
        pattern | and_(and_(or_(x)), and_(1)) =
            [&] {
              CHECK_EQ(*x, 10);
              return true;
            },
        pattern | _ = expr(false)
    );

    CHECK_FALSE(result);
  }

  SUBCASE("test match multiple times 1") {
    Id<int32_t> z;

    match(10)(
        pattern | and_(z, z) = [&] { CHECK_EQ(*z, 10); }
    );
  }

  SUBCASE("test match multiple times 2") {
    Id<std::unique_ptr<int32_t>> x;

    auto result = match(std::make_unique<int32_t>(10))(
        pattern | and_(x) = [&] { return **x; }
    );

    CHECK_EQ(result, 10);
  }

  SUBCASE("test match multiple times 3") {
    Id<std::unique_ptr<int32_t>> x1;
    Id<std::unique_ptr<int32_t>> x2;

    auto result = match(std::make_unique<int32_t>(10))(
        pattern | and_(x1, x2) = [&] { return **x2; }
    );

    CHECK_EQ(result, 10);
  }

  SUBCASE("test app to id") {
    Id<int32_t> ii;

    auto const result = match(11)(
        pattern | app(_ * _, ii) = expr(ii)
    );

    CHECK_EQ(result, 121);
  }

  SUBCASE("test app to id 2") {
    Id<std::unique_ptr<int32_t>> ii;

    auto const result = match(11)(
        pattern | app([](auto &&x) { return std::make_unique<int32_t>(x); }, ii) = [&] { return **ii; }
    );

    CHECK_EQ(result, 11);
  }

  SUBCASE("test app to id 3") {
    Id<std::shared_ptr<int32_t>> ii;

    auto const result = match(std::make_shared<int32_t>(11))(
        pattern | ii = [&] { return ii.move(); }
    );

    CHECK_EQ(*result, 11);
  }

  SUBCASE("test app to id 4") {
    Id<std::shared_ptr<int32_t>> ii;

    auto const result = match(11)(
        pattern | app([](auto &&x) { return std::make_shared<int32_t>(x); }, ii) = [&] { return ii.move(); }
    );

    CHECK_EQ(*result, 11);
  }

  SUBCASE("test app to id 5") {
    Id<std::unique_ptr<int32_t>> ii;

    auto const result = match(std::make_unique<int32_t>(11))(
        pattern | ii = [&] { return ii.move(); }
    );

    CHECK_EQ(*result, 11);
  }

  SUBCASE("test app to id 5-1") {
    Id<std::unique_ptr<int32_t>> ii, jj;

    auto const result = match(std::make_unique<int32_t>(11))(
        pattern | and_(ii, jj) = [&] { return **ii; });

    CHECK_EQ(result, 11);
  }

  SUBCASE("test app to id 5-2") {
    Id<std::unique_ptr<int32_t>> ii, jj;

    auto const result = match(std::make_unique<int32_t>(11))(
        pattern | and_(ii, jj) = [&] { return **jj; });

    CHECK_EQ(result, 11);
  }

  SUBCASE("test app to id 5-pro") {
    Id<std::unique_ptr<int32_t>> jj;

    auto const result = match(std::make_unique<int32_t>(11))(
        pattern | and_(_, jj) = [&] { return jj.move(); }
    );

    CHECK_EQ(*result, 11);
  }

  SUBCASE("test app to id 5-plus pro negative") {
    auto const invalid_move = [] {
      Id<std::unique_ptr<int32_t>> ii, jj;

      match(std::make_unique<int32_t>(11))(
          pattern | and_(ii, jj) = [&] { return jj.move(); }
      );
    };

    CHECK_THROWS(invalid_move());
  }

  SUBCASE("test app to id 6") {
    Id<std::unique_ptr<int32_t>> ii;

    auto const result = match(11)(
        pattern | app([](auto &&x) { return std::make_unique<int32_t>(x); }, ii) = [&] { return ii.move(); }
    );

    CHECK_EQ(*result, 11);
  }

  SUBCASE("test app to id 7") {
    Id<std::optional<int32_t>> ii;

    auto const result = match(std::make_optional(11))(
        pattern | ii = [&] { return ii.move(); }
    );

    CHECK_EQ(*result, 11);
  }

  SUBCASE("test app to id 8") {
    Id<std::optional<int32_t>> ii;

    auto const result = match(11)(
        pattern | app([](auto &&x) { return std::make_optional(x); }, ii) = [&] { return ii.move(); }
    );

    CHECK_EQ(*result, 11);
  }

  SUBCASE("test id at int") {
    Id<int32_t> ii;

    auto const result = match(11)(
        pattern | app(_ * _, ii.at(121)) = expr(ii)
    );

    CHECK_EQ(result, 121);
  }

  SUBCASE("test id at unique") {
    Id<std::unique_ptr<int32_t>> ii;

    auto const result = match(11)(
        pattern | app([](auto &&x) { return std::make_unique<int32_t>(x * x); }, ii.at(some(_))) = [&] { return ii.move(); }
    );

    CHECK_EQ(*result, 121);
  }

  SUBCASE("test invalid value") {
    Id<int> x;
    CHECK_THROWS(*x);
  }

  SUBCASE("test invalid move()") {
    Id<std::string> x;
    CHECK_THROWS(x.move());
    std::string str = "12345";
    x.match_value(str);
    CHECK_THROWS(x.move());
  }

}

constexpr int32_t fib(int32_t n)
{
  assert(n >= 1);
  return match(n)(
      //@formatter:off
      pattern | 1 = expr(1),
      pattern | 2 = expr(1),
      pattern | _ = [n] { return fib(n - 1) + fib(n - 2); }
      //@formatter:on
  );
}

template<typename T>
constexpr auto eval(T &&input)
{
  return match(input)(
      pattern | ds('/', 1, 1) = expr(1),
      pattern | ds('/', 0, _) = expr(0),
      pattern | _ = expr(-1)
  );
}

TEST_CASE("scenario: constexpr")
{
  CHECK(fib(1) == 1);
  CHECK(fib(2) == 1);
  CHECK(fib(3) == 2);
  CHECK(fib(4) == 3);
  CHECK(fib(5) == 5);
  CHECK(eval(std::make_tuple('/', 0, 5)) == 0);

}

TEST_CASE("scenario: no return")
{
  SUBCASE("test match statement") {
    std::string output;

    match(4)(
        //@formatter:off
        pattern | or_(_ < 0, 2) = [&] { output = "mismatch!"; },
        pattern | _             = [&] { output = "match all!"; }
        //@formatter:on
    );

    CHECK(output == "match all!");
  }

  SUBCASE("test match expression no match throws exception") {
    CHECK_THROWS(match(4)(pattern | 1 = expr(true)));
  }
}

TEST_CASE("scenario: expression")
{
  SUBCASE("test nullary") {
    CHECK_EQ(expr(5)(), 5);
    CHECK_EQ((!expr(false))(), true);
    CHECK_EQ((expr(5) + 5)(), 10);
    CHECK_EQ((expr(5) % 5)(), 0);
    CHECK_EQ((expr(5) < 5)(), false);
    CHECK_EQ((expr(5) <= 5)(), true);
    CHECK_EQ((expr(5) != 5)(), false);
    CHECK_EQ((expr(5) >= 5)(), true);
    CHECK_EQ((expr(false) && true)(), false);
    CHECK_EQ((expr(false) || true)(), true);
  }

  SUBCASE("test unary") {
    CHECK_EQ((!_)(true), false);
    CHECK_EQ((-_)(1), -1);
    CHECK_EQ((1 - _)(1), 0);
    CHECK_EQ((_ % 3)(5), 2);
    CHECK_EQ((_ * 2)(5), 10);
    CHECK_EQ((_ == 2)(5), false);
    CHECK_EQ((_ != 2)(5), true);
    CHECK_EQ((_ || false)(true), true);
    CHECK_EQ((_ && false)(true), false);
  }
}

TEST_CASE("scenario: ds")
{
  using namespace impl;

  CHECK(impl::is_range_v<const std::vector<int32_t>>);

  SUBCASE("test match tuple") {
    CHECK(matched(std::make_tuple(), ds()));
    CHECK(matched(std::make_tuple("123", 123), ds("123", 123)));
    CHECK_FALSE(matched(std::make_tuple("123", 123), ds("123", 12)));
  }

  SUBCASE("test match array") {
    CHECK(matched(std::array<int32_t, 0>{}, ds()));
    CHECK(matched(std::array<int32_t, 2>{123, 456}, ds(123, 456)));
    CHECK_FALSE(matched(std::array<int32_t, 2>{123, 456}, ds(456, 123)));
  }

  SUBCASE("test match vec") {
    CHECK(matched(std::vector<int32_t>{}, ds()));
    CHECK(matched(std::vector<int32_t>{123, 456}, ds(123, 456)));
    CHECK_FALSE(matched(std::vector<int32_t>{123, 456}, ds(123, 456, 123)));
  }

  SUBCASE("test match list") {
    CHECK(matched(std::list<int32_t>{}, ds()));
    CHECK(matched(std::list<int32_t>{123, 456}, ds(123, 456)));
    CHECK_FALSE(matched(std::list<int32_t>{123, 456}, ds(123, 456, 123)));
  }

  SUBCASE("test tuple ooo") {
    CHECK(matched(std::tuple<>{}, ds(ooo)));
    CHECK(matched(std::make_tuple("123", 123), ds(ooo)));
    CHECK(matched(std::make_tuple("123", 123), ooo));
    CHECK(matched(std::make_tuple("123", 123), ds(ooo, 123)));
    CHECK(matched(std::make_tuple("123", 123), ds("123", ooo)));
    CHECK(matched(std::make_tuple("123", 123), ds("123", ooo, 123)));
  }

  SUBCASE("test array ooo") {
    CHECK(matched(std::array<int32_t, 2>{123, 456}, ds(ooo)));
    CHECK(matched(std::array<int32_t, 0>{}, ds(ooo)));
    CHECK(matched(std::array<int32_t, 2>{123, 456}, ds(123, ooo)));
    CHECK(matched(std::array<int32_t, 2>{123, 456}, ds(ooo, 456)));
    CHECK(matched(std::array<int32_t, 2>{123, 456}, ds(123, ooo, 456)));
  }

  SUBCASE("test vec ooo") {
    CHECK(matched(std::vector<int32_t>{123, 456}, ds(ooo)));
    CHECK(matched(std::vector<int32_t>{}, ds(ooo)));
    CHECK(matched(std::vector<int32_t>{123, 456}, ds(123, ooo)));
    CHECK(matched(std::vector<int32_t>{123, 456}, ds(ooo, 456)));
    CHECK(matched(std::vector<int32_t>{123, 456}, ds(123, ooo, 456)));
  }

  SUBCASE("test list ooo") {
    CHECK(matched(std::list<int32_t>{123, 456}, ds(ooo)));
    CHECK(matched(std::list<int32_t>{}, ds(ooo)));
    CHECK(matched(std::list<int32_t>{123, 456}, ds(123, ooo)));
    CHECK(matched(std::list<int32_t>{123, 456}, ds(ooo, 456)));
    CHECK(matched(std::list<int32_t>{123, 456}, ds(123, ooo, 456)));
  }

  SUBCASE("test vec ooo binder 1") {
    auto const vec = std::vector<int32_t>{123, 456};
    Id<SubrangeT<decltype(vec)>> subrange;
    auto matched = match(vec)(
        pattern | subrange.at(ooo) =
            [&] {
              auto const expected = {123, 456};
              expect_range(*subrange, expected);
              return true;
            },
        pattern | _ = expr(false));
    CHECK(matched);
  }

  SUBCASE("test vec ooo binder 2") {
    Id<SubrangeT<std::vector<int32_t>>> subrange;
    match(std::vector<int32_t>{123, 456})(
        pattern | subrange.at(ooo) =
            [&] {
              auto const expected = {123, 456};
              expect_range(*subrange, expected);
            },
        pattern | _ =
            [] {
              // make sure the above pattern is matched, otherwise the test case
              // fails.
              DOCTEST_FAIL("test vec ooo binder 2 no match");
            });
  }

  SUBCASE("test vec ooo binder 3") {
    Id<SubrangeT<std::vector<int32_t>>> subrange;
    match(std::vector<int32_t>{123, 456})(
        pattern | ds(123, subrange.at(ooo), 456) = [&] { CHECK_EQ((*subrange).size(), 0); });
  }

  SUBCASE("test vec ooo binder 4") {
    Id<SubrangeT<std::vector<int32_t>>> subrange;
    match(std::vector<int32_t>{123, 456, 789})(
        pattern | ds(123, subrange.at(ooo)) = [&] {
          auto const expected = {456, 789};
          expect_range(*subrange, expected);
        });
  }

  SUBCASE("test fail due to two few values") {
    CHECK_FALSE(matched(std::vector<int32_t>{123, 456, 789}, ds(123, ooo, 456, 456, 789)));
  }

  SUBCASE("test list ooo binder 4") {
    Id<SubrangeT<std::list<int32_t>>> subrange;
    match(std::list<int32_t>{123, 456, 789})(
        pattern | ds(123, subrange.at(ooo)) = [&] {
          auto const expected = {456, 789};
          expect_range(*subrange, expected);
        });
  }

  SUBCASE("test array ooo binder 1") {
    auto const array = std::array<int32_t, 2>{123, 456};
    Id<SubrangeT<decltype(array)>> subrange;
    auto matched = match(array)(
        pattern | subrange.at(ooo) =
            [&] {
              auto const expected = {123, 456};
              expect_range(*subrange, expected);
              return true;
            },
        pattern | _ = expr(false));
    CHECK(matched);
  }

  SUBCASE("test array ooo binder 2") {
    Id<SubrangeT<std::array<int32_t, 2>>> subrange;
    match(std::array<int32_t, 2>{123, 456})(
        pattern | subrange.at(ooo) = [&] {
          auto const expected = {123, 456};
          expect_range(*subrange, expected);
        });
  }

  SUBCASE("test array ooo binder 3") {
    Id<SubrangeT<std::array<int32_t, 2>>> subrange;
    match(std::array<int32_t, 2>{123, 456})(
        pattern | ds(123, subrange.at(ooo), 456) = [&] { CHECK_EQ((*subrange).size(), 0); });
  }

  SUBCASE("test array ooo binder 4") {
    Id<SubrangeT<std::array<int32_t, 2>>> subrange;
    match(std::forward_as_tuple(std::array<int32_t, 3>{123, 456, 789}))(
        pattern | ds(ds(123, subrange.at(ooo))) = [&] {
          auto const expected = {456, 789};
          expect_range(*subrange, expected);
        });
  }

  SUBCASE("test array ooo binder 5") {
    Id<SubrangeT<std::array<int32_t, 2>>> subrange;
    Id<int32_t> e;
    match(std::make_tuple(std::array<int32_t, 3>{123, 456, 789}, std::array<int32_t, 3>{456, 789, 123}))(
        // move head to end
        pattern | ds(ds(e, subrange.at(ooo)), ds(subrange.at(ooo), e)) = [&] {
          CHECK_EQ(*e, 123);
          auto const expected = {456, 789};
          expect_range(*subrange, expected);
        });
  }

  SUBCASE("test array ooo binder 6") {
    Id<SubrangeT<std::array<int32_t, 2>>> subrange;
    Id<int32_t> e;
    match(std::make_tuple(std::array<int32_t, 3>{123, 456, 789}))(
        // move head to end
        pattern | ds(ds(e, subrange.at(ooo))) = [&] {
          CHECK_EQ(*e, 123);
          auto const expected = {456, 789};
          expect_range(*subrange, expected);
        });
  }

  SUBCASE("test subrange ooo binder ") {
    CHECK(recursive_symmetric(std::array<int32_t, 5>{5, 0, 3, 0, 5}));
    CHECK_FALSE(recursive_symmetric(std::array<int32_t, 5>{5, 0, 3, 7, 10}));

    CHECK(recursive_symmetric(std::vector<int32_t>{5, 0, 3, 0, 5}));
    CHECK_FALSE(recursive_symmetric(std::vector<int32_t>{5, 0, 3, 7, 10}));

    CHECK(recursive_symmetric(std::list<int32_t>{5, 0, 3, 0, 5}));
    CHECK_FALSE(recursive_symmetric(std::list<int32_t>{5, 0, 3, 7, 10}));

    CHECK(recursive_symmetric(std::initializer_list<int32_t>{5, 0, 3, 0, 5}));
    CHECK_FALSE(recursive_symmetric(std::initializer_list<int32_t>{5, 0, 3, 7, 10}));
  }

  SUBCASE("test set ooo binder ") {
    Id<SubrangeT<std::set<int32_t>>> subrange;
    Id<int32_t> e;
    match(std::set<int32_t>{123, 456, 789})(pattern | ds(e, subrange.at(ooo)) = [&] {
      CHECK_EQ(*e, 123);
      auto const expected = {456, 789};
      expect_range(*subrange, expected);
    });
  }

  SUBCASE("test map id") {
    Id<std::pair<int32_t, char const *>> e, f, g;
    match(std::map<int32_t, char const *>{{123, "a"}, {456, "b"}, {789, "c"}})(
        pattern | ds(e, f, g) = [&] {
          bool eq = *e == std::make_pair(123, "a");
          CHECK(eq);
          eq = *f == std::make_pair(456, "b");
          CHECK(eq);
          eq = *g == std::make_pair(789, "c");
          CHECK(eq);
        });
  }

  SUBCASE("test map ooo") {
    Id<std::pair<int32_t, char const *>> e;
    match(std::map<int32_t, char const *>{{123, "a"}, {456, "b"}, {789, "c"}})(
        pattern | ds(e, ooo) = [&] {
          auto eq = std::make_pair(123, "a") == (*e);
          CHECK(eq);
        });
  }

  SUBCASE("test map ooo at binder") {
    Id<SubrangeT<std::map<int32_t, char const *>>> subrange;
    Id<std::pair<int32_t, char const *>> e;
    match(std::map<int32_t, char const *>{{123, "a"}, {456, "b"}, {789, "c"}})(
        pattern | ds(e, subrange.at(ooo)) = [&] {
          auto eq = ((*e).first == 123) && (std::strcmp((*e).second, "a") == 0);
          CHECK(eq);
          auto const expected = {std::make_pair(456, "b"), std::make_pair(789, "c")};
          expect_range(*subrange, expected);
        });
  }
}

TEST_CASE("scenario: legacy")
{
  static_assert(impl::StorePointer<std::unique_ptr<int32_t> const, std::unique_ptr<int32_t> const &>::value);
  static_assert(!impl::StorePointer<std::unique_ptr<int32_t>, std::unique_ptr<int32_t> &&>::value);

  SUBCASE("match test 1") {
    auto const match_func = [](int32_t input) {
      Id<int32_t> ii;
      //@formatter:off
      return match(input)(
          pattern | 1                    = func1, 
          pattern | 2                    = func2,
          pattern | or_(56, 59)          = func2,
          pattern | (_ < 0)              = expr(-1), 
          pattern | (_ < 10)             = expr(-10),
          pattern | and_(_ < 17, _ > 15) = expr(16),
          pattern | app(_ * _, _ > 1000) = expr(1000),
          pattern | app(_ * _, ii)       = expr(ii), 
          pattern | ii                   = -ii,
          pattern | _                    = expr(111));
      //@formatter:on
    };

    CHECK_EQ(match_func(1), 1);
    CHECK_EQ(match_func(2), 12);
    CHECK_EQ(match_func(11), 121);   // id matched.
    CHECK_EQ(match_func(59), 12);    // or_ matched.
    CHECK_EQ(match_func(-5), -1);    // meet matched.
    CHECK_EQ(match_func(10), 100);   // app matched.
    CHECK_EQ(match_func(100), 1000); // app > meet matched.
    CHECK_EQ(match_func(5), -10);    // _ < 10 matched.
    CHECK_EQ(match_func(16), 16);    // and_ matched.
  }

  SUBCASE("match test 2") {
    auto const match_func = [](auto &&input) {
      Id<int32_t> i;
      Id<int32_t> j;
      //@formatter:off
      return match(input)(
          pattern | ds('/', 1, 1) = expr(1),
          pattern | ds('/', 0, _) = expr(0),
          pattern | ds('*', i, j) = i * j,
          pattern | ds('+', i, j) = i + j,
          pattern | _             = expr(-1)
      );
      //@formatter:on
    };

    CHECK_EQ(match_func(std::make_tuple('/', 1, 1)), 1);
    CHECK_EQ(match_func(std::make_tuple('+', 2, 1)), 3);
    CHECK_EQ(match_func(std::make_tuple('/', 0, 1)), 0);
    CHECK_EQ(match_func(std::make_tuple('*', 2, 1)), 2);
    CHECK_EQ(match_func(std::make_tuple('/', 2, 1)), -1);
    CHECK_EQ(match_func(std::make_tuple('/', 2, 3)), -1);
  }

  SUBCASE("match test 3") {
    auto const match_func = [](A const &input) {
      Id<int32_t> i;
      // compose patterns for destructuring struct A.
      auto const dsA = ds_via(&A::a, &A::b);
      return match(input)(pattern | dsA(i, 1) = expr(i), pattern | _ = expr(-1));
    };

    CHECK_EQ(match_func(A{3, 1}), 3);
    CHECK_EQ(match_func(A{2, 2}), -1);
  }

  SUBCASE("match test 4") {
    auto const match_func = [](Num const &input) {
      return match(input)(
          pattern | as<One>(_) = expr(1),
          pattern | kind<Kind::kTwo> = expr(2),
          pattern | _ = expr(3)
      );
    };

    matchit::impl::AsPointer<Two>()(std::variant<One, Two>{});
    CHECK_EQ(match_func(One{}), 1);
    CHECK_EQ(match_func(Two{}), 2);
  }

  SUBCASE("match test 5") {
    auto const match_func = [](std::pair<int32_t, int32_t> ij) {
      //@formatter:off
      return match(ij.first % 3, ij.second % 5)(
          pattern | ds(0, 0)     = expr(1),
          pattern | ds(0, _ > 2) = expr(2),
          pattern | ds(_, _ > 2) = expr(3),
          pattern | _            = expr(4)
      );
      //@formatter:on
    };

    CHECK_EQ(match_func(std::make_pair(3, 5)), 1);
    CHECK_EQ(match_func(std::make_pair(3, 4)), 2);
    CHECK_EQ(match_func(std::make_pair(4, 4)), 3);
    CHECK_EQ(match_func(std::make_pair(4, 1)), 4);
  }

  SUBCASE("match test 6") {
    CHECK_EQ(fib(1), 1);
    CHECK_EQ(fib(2), 1);
    CHECK_EQ(fib(3), 2);
    CHECK_EQ(fib(4), 3);
    CHECK_EQ(fib(5), 5);
  }

  SUBCASE("match test 7") {
    auto const match_func = [](std::pair<int32_t, int32_t> ij) {
      Id<std::tuple<int32_t, int32_t>> id;
      // delegate at to and_
      auto const at = [](auto &&id, auto &&pattern) { return and_(pattern, id); };
      return match(ij.first % 3, ij.second % 5)(
          pattern | ds(0, _ > 2) = expr(2),
          pattern | ds(1, _ > 2) = expr(3),
          pattern | at(id, ds(_, 2)) =
              [&id] {
                CHECK(std::get<1>(*id) == 2);
                static_cast<void>(id);
                return 4;
              },
          pattern | _ = expr(5)
      );
    };

    CHECK_EQ(match_func(std::make_pair(4, 2)), 4);
  }

  SUBCASE("match test 8") {
    auto const equal = [](std::pair<int32_t, std::pair<int32_t, int32_t>> ijk) {
      Id<int32_t> x;

      return match(ijk)(
          pattern | ds(x, ds(_, x)) = expr(true),
          pattern | _ = expr(false)
      );
    };

    CHECK(equal(std::make_pair(2, std::make_pair(1, 2))));
    CHECK_FALSE(equal(std::make_pair(2, std::make_pair(1, 3))));
  }

  SUBCASE("match test 9") {
    auto const optional = [](auto const &i) {
      Id<int32_t> x;
      return match(i)(
          pattern | some(x) = expr(true),
          pattern | none = expr(false)
      );
    };

    CHECK_EQ(optional(std::make_unique<int32_t>(2)), true);
    CHECK_EQ(optional(std::make_shared<int32_t>(2)), true);
    CHECK_EQ(optional(std::unique_ptr<int32_t>{}), false);
    CHECK_EQ(optional(std::make_optional<int32_t>(2)), true);
    CHECK_EQ(optional(std::optional<int32_t>{}), false);

    int32_t *p = nullptr;
    CHECK_EQ(optional(p), false);

    int32_t a = 3;
    CHECK_EQ(optional(&a), true);
  }

  SUBCASE("match test 10") {
    static_assert(matchit::impl::StorePointer<Shape, Shape &>::value);
    static_assert(matchit::impl::StorePointer<Shape const, Shape const &>::value);

    auto const dyn_cast = [](auto const &i) {
      return match(i)(
          pattern | some(as<Circle>(_)) = expr("Circle"),
          pattern | some(as<Square>(_)) = expr("Square"),
          pattern | none = expr("None")
      );
    };

    CHECK_EQ(dyn_cast(std::unique_ptr<Shape>(new Square{})), "Square");
    CHECK_EQ(dyn_cast(std::unique_ptr<Shape>(new Circle{})), "Circle");
    CHECK_EQ(dyn_cast(std::unique_ptr<Shape>()), "None");
  }

  SUBCASE("match test 10-1") {
    auto const dToBCast = [](auto const &i) {
      return match(i)(
          pattern | some(as<Shape>(_)) = expr("Shape"),
          pattern | none = expr("None")
      );
    };

    CHECK_EQ(dToBCast(std::make_unique<Circle>()), "Shape");
  }

  SUBCASE("match test 11") {
    auto const get_if = [](auto const &i) {
      return match(i)(
          pattern | as<Square>(_) = expr("Square"),
          pattern | as<Circle>(_) = expr("Circle")
      );
    };

    std::variant<Square, Circle> sc = Square{};
    CHECK_EQ(get_if(sc), "Square");
    sc = Circle{};
    CHECK_EQ(get_if(sc), "Circle");
  }

  SUBCASE("match test 12") {
    CHECK(matched(std::array<int32_t, 2>{1, 2}, ds(ooo, _)));
    CHECK(matched(std::array<int32_t, 3>{1, 2, 3}, ds(ooo, _)));
    CHECK(matched(std::array<int32_t, 2>{1, 2}, ds(ooo, _)));
  }

  SUBCASE("match test 13") {
    auto const ds_agg = [](auto const &v) {
      Id<int32_t> i;

      return match(v)(
          pattern | ds(1, i) = expr(i),
          pattern | ds(_, i) = expr(i)
      );
    };

    CHECK_EQ(ds_agg(A{1, 2}), 2);
    CHECK_EQ(ds_agg(A{3, 2}), 2);
    CHECK_EQ(ds_agg(A{5, 2}), 2);
    CHECK_EQ(ds_agg(A{2, 5}), 5);
  }


  SUBCASE("match test 14") {
    auto const anyCast = [](auto const &i) {
      return match(i)(
          pattern | as<Square>(_) = expr("Square"),
          pattern | as<Circle>(_) = expr("Circle")
      );
    };

    std::any sc;
    sc = Square{};
    CHECK_EQ(anyCast(sc), "Square");
    sc = Circle{};
    CHECK_EQ(anyCast(sc), "Circle");

    CHECK(matched(sc, as<Circle>(_)));
    CHECK_FALSE(matched(sc, as<Square>(_)));
  }

  SUBCASE("match test 15") {
    auto const optional = [](auto const &i) {
      Id<char> c;
      return match(i)(
          pattern | none = expr(1),
          pattern | some(none) = expr(2),
          pattern | some(some(c)) = expr(c)
      );
    };
    char const **x = nullptr;
    char const *y_ = nullptr;
    char const **y = &y_;
    char const *z_ = "x";
    char const **z = &z_;

    CHECK_EQ(optional(x), 1);
    CHECK_EQ(optional(y), 2);
    CHECK_EQ(optional(z), 'x');
  }

  SUBCASE("match test 16") {
    auto const notX = [](auto const &i) {
      return match(i)(
          pattern | not_(or_(1, 2)) = expr(3),
          pattern | 2 = expr(2),
          pattern | _ = expr(1)
      );
    };

    CHECK_EQ(notX(1), 1);
    CHECK_EQ(notX(2), 2);
    CHECK_EQ(notX(3), 3);
  }

// when
  SUBCASE("match test 17") {
    auto const whenX = [](auto const &x) {
      Id<int32_t> i, j;
      return match(x)(
          pattern | ds(i, j) | when(i + j == 10) = expr(3),
          pattern | ds(_ < 5, _) = expr(5),
          pattern | _ = expr(1)
      );
    };

    CHECK_EQ(whenX(std::make_pair(1, 9)), 3);
    CHECK_EQ(whenX(std::make_pair(1, 7)), 5);
    CHECK_EQ(whenX(std::make_pair(7, 7)), 1);
  }

  SUBCASE("match test 18") {
    auto const idNotOwn = [](auto const &x) {
      Id<int32_t> i;
      return match(x)(
          pattern | i | when(i == 5) = expr(1),
          pattern | _ = expr(2)
      );
    };

    CHECK_EQ(idNotOwn(1), 2);
    CHECK_EQ(idNotOwn(5), 1);
  }

  SUBCASE("match test 19") {
    auto const match_func = [](auto &&input) {
      Id<int32_t> j;
      return match(input)(
          // `... / 2 3`
          pattern | ds(ooo, '/', 2, 3) = expr(1),
          // `... 3`
          pattern | ds(ooo, 3) = expr(3),
          // `/ ...`
          pattern | ds('/', ooo) = expr(4),

          pattern | ooo = expr(222),
          // `3 3 3 3 ..` all 3
          pattern | ooo = expr(333),

          // `... int32_t 3`
          pattern | ds(ooo, j, 3) = expr(7),
          // `... int32_t 3`
          pattern | ds(ooo, or_(j), 3) = expr(8),

          // `...`
          pattern | ooo = expr(12),

          pattern | _ = expr(-1)
      );
    };

    CHECK_EQ(match_func(std::make_tuple('/', 2, 3)), 1);
    CHECK_EQ(match_func(std::make_tuple(3, 3, 3, 3, 3)), 3);
    CHECK(matched(std::make_tuple(3, 3, 3, 3, 3), ds(ooo)));
    CHECK(matched(std::make_tuple("123", 3, 3, 3, 2), ds("123", ooo, 2)));
    CHECK(matched(std::make_tuple("string", 3, 3, 3, 3), ds(ooo, 3)));
    CHECK(matched(std::make_tuple("string", 3, 3, 3, 3), ds(ooo)));
    CHECK(matched(std::make_tuple("string"), ds(ooo)));
  }

  SUBCASE("match test 20") {
    Id<std::string> strA;
    Id<const char *> strB;
    CHECK(matched("abc", strA));
    CHECK(matched("abc", strB));
  }

}

