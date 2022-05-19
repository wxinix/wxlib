/*!
  MPL 1.1/GPL 2.0/LGPL 2.1 tri-license
  Copyright (C) 2022  Wuping Xin
*/

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <semimap/semimap.hpp>

#define ID(x) \
    []() constexpr { return x; }

TEST_CASE("staticmap") // NOLINT(cert-err58-cpp)
{
  SUBCASE("test compile-time only load and store") {
    struct Tag
    {
    };

    using map = semi::static_map<std::string, std::string, Tag>;

    auto &food = map::get(ID("food"));
    CHECK(food.empty());

    food = "pizza";
    CHECK(map::get(ID("food")) == "pizza");

    auto &drink = map::get(ID("drink"));
    CHECK(drink.empty());

    drink = "beer";
    CHECK(map::get(ID("food")) == "pizza");
    CHECK(map::get(ID("drink")) == "beer");

    map::get(ID("food")) = "spaghetti";
    CHECK(map::get(ID("food")) == "spaghetti");
    CHECK(map::get(ID("drink")) == "beer");

    map::get(ID("drink")) = "soda";
    CHECK(map::get(ID("food")) == "spaghetti");
    CHECK(map::get(ID("drink")) == "soda");

    CHECK(map::get(ID("starter"), "soup") == "soup");
    CHECK(map::get(ID("starter"), "salad") == "soup");
  }

  SUBCASE("test run-time only load and store") {
    struct Tag
    {
    };

    using map = semi::static_map<std::string, std::string, Tag>;

    auto &food = map::get("food");
    CHECK(food.empty());

    food = "pizza";
    CHECK(map::get("food") == "pizza");

    auto &drink = map::get("drink");
    CHECK(drink.empty());

    drink = "beer";
    CHECK(map::get("food") == "pizza");
    CHECK(map::get("drink") == "beer");

    map::get("food") = "spaghetti";
    CHECK(map::get("food") == "spaghetti");
    CHECK(map::get("drink") == "beer");

    map::get("drink") = "soda";
    CHECK(map::get("food") == "spaghetti");
    CHECK(map::get("drink") == "soda");

    CHECK(map::get("starter", "soup") == "soup");
    CHECK(map::get("starter", "salad") == "soup");
  }

  SUBCASE("test mixed compile-time/run-time load and store") {
    struct Tag
    {
    };

    using map = semi::static_map<std::string, std::string, Tag>;

    // compile-time first, then run-time
    map::get(ID("food")) = "pizza";
    CHECK(map::get("food") == "pizza");

    // runtime-time first, then compile-time
    map::get("drink") = "beer";
    CHECK(map::get(ID("drink")) == "beer");

    CHECK(map::get(ID("food")) == "pizza");
    CHECK(map::get("drink") == "beer");

    CHECK(map::get(ID("starter"), "soup") == "soup");
    CHECK(map::get("starter", "salad") == "soup");

    CHECK(map::get("side", "rice") == "rice");
    CHECK(map::get(ID("side"), "peas") == "rice");
  }

  // test clear & contains
  SUBCASE("test clear and contains") {
    struct Tag
    {
    };

    using map = semi::static_map<std::string, std::string, Tag>;

    CHECK(!map::contains(ID("food")));
    CHECK(!map::contains("food"));

    // check again to see if contains by mistake add the element
    CHECK(!map::contains(ID("food")));
    CHECK(!map::contains("food"));

    map::get(ID("food")) = "pizza";
    CHECK(map::contains(ID("food")));
    CHECK(map::contains("food"));

    map::get("drink") = "beer";
    CHECK(map::contains("drink"));
    CHECK(map::contains(ID("drink")));

    map::get(ID("dessert")) = "ice cream";
    CHECK(map::contains("dessert"));
    CHECK(map::contains(ID("dessert")));

    map::get("starter") = "salad";
    CHECK(map::contains(ID("starter")));
    CHECK(map::contains("starter"));

    map::clear();

    CHECK(!map::contains(ID("food")));
    CHECK(!map::contains("food"));
    CHECK(!map::contains("drink"));
    CHECK(!map::contains(ID("drink")));
  }

  SUBCASE("test erase") {
    struct Tag
    {
    };

    using map = semi::static_map<std::string, std::string, Tag>;

    map::get(ID("food")) = "pizza";
    map::get(ID("drink")) = "beer";
    map::get(ID("dessert")) = "ice cream";
    map::get(ID("starter")) = "soup";
    map::get(ID("side")) = "salad";

    // erase first
    map::erase(ID("food"));
    CHECK(!map::contains(ID("food")));
    CHECK(map::contains(ID("drink")));
    CHECK(map::contains(ID("dessert")));
    CHECK(map::contains(ID("starter")));
    CHECK(map::contains(ID("side")));

    // erase last
    map::erase("side");
    CHECK(!map::contains(ID("food")));
    CHECK(map::contains(ID("drink")));
    CHECK(map::contains(ID("dessert")));
    CHECK(map::contains(ID("starter")));
    CHECK(!map::contains(ID("side")));

    // try adding something
    map::get("bill") = "too much";
    CHECK(!map::contains(ID("food")));
    CHECK(map::contains(ID("drink")));
    CHECK(map::contains(ID("dessert")));
    CHECK(map::contains(ID("starter")));
    CHECK(!map::contains(ID("side")));
    CHECK(map::contains(ID("bill")));

    // erase middle
    map::erase(ID("dessert"));
    CHECK(!map::contains(ID("food")));
    CHECK(map::contains(ID("drink")));
    CHECK(!map::contains(ID("dessert")));
    CHECK(map::contains(ID("starter")));
    CHECK(!map::contains(ID("side")));
    CHECK(map::contains(ID("bill")));
  }

  SUBCASE("test independent maps do not influence each other") {
    struct TagA
    {
    };

    struct TagB
    {
    };

    using mapA = semi::static_map<std::string, std::string, TagA>;
    using mapB = semi::static_map<std::string, std::string, TagB>;

    mapA::get(ID("food")) = "pizza";
    CHECK(mapA::get("food") == "pizza");
    CHECK(!mapB::contains("food"));

    mapB::get(ID("food")) = "spaghetti";
    CHECK(mapA::get("food") == "pizza");
    CHECK(mapB::get("food") == "spaghetti");

    mapB::get("drink") = "beer";
    CHECK(mapB::get(ID("drink")) == "beer");
    CHECK(!mapA::contains(ID("drink")));
    CHECK(mapA::contains("food"));

    mapA::get("drink") = "soda";
    CHECK(mapA::get(ID("drink")) == "soda");
    CHECK(mapB::get(ID("drink")) == "beer");

    mapA::get(ID("starter")) = "salad";
    mapB::get("starter") = "soup";

    mapB::erase("drink");
    CHECK(mapA::contains("drink"));
    CHECK(mapA::contains(ID("drink")));
    CHECK(!mapB::contains("drink"));
    CHECK(!mapB::contains(ID("drink")));

    mapB::clear();
    CHECK(mapA::get(ID("starter")) == "salad");
    CHECK(mapA::get("food") == "pizza");
    CHECK(mapA::get(ID("drink")) == "soda");
    CHECK(!mapB::contains("food"));
    CHECK(!mapB::contains(ID("drink")));
  }
}

TEST_CASE("map") // NOLINT(cert-err58-cpp)
{
  SUBCASE("test compile-time only load and store") {
    semi::map<std::string, std::string> map;

    auto &food = map.get(ID("food"));
    CHECK(food.empty());

    food = "pizza";
    CHECK(map.get(ID("food")) == "pizza");

    auto &drink = map.get(ID("drink"));
    CHECK(drink.empty());

    drink = "beer";
    CHECK(map.get(ID("food")) == "pizza");
    CHECK(map.get(ID("drink")) == "beer");

    map.get(ID("food")) = "spaghetti";
    CHECK(map.get(ID("food")) == "spaghetti");
    CHECK(map.get(ID("drink")) == "beer");

    map.get(ID("drink")) = "soda";
    CHECK(map.get(ID("food")) == "spaghetti");
    CHECK(map.get(ID("drink")) == "soda");

    CHECK(map.get(ID("starter"), "soup") == "soup");
    CHECK(map.get(ID("starter"), "salad") == "soup");
  }

  SUBCASE("test run-time only load and store") {
    semi::map<std::string, std::string> map;

    auto &food = map.get("food");
    CHECK(food.empty());

    food = "pizza";
    CHECK(map.get("food") == "pizza");

    auto &drink = map.get("drink");
    CHECK(drink.empty());

    drink = "beer";
    CHECK(map.get("food") == "pizza");
    CHECK(map.get("drink") == "beer");

    map.get("food") = "spaghetti";
    CHECK(map.get("food") == "spaghetti");
    CHECK(map.get("drink") == "beer");

    map.get("drink") = "soda";
    CHECK(map.get("food") == "spaghetti");
    CHECK(map.get("drink") == "soda");

    CHECK(map.get("starter", "soup") == "soup");
    CHECK(map.get("starter", "salad") == "soup");
  }

  SUBCASE("test mixed compile-time/run-time load and store") {
    semi::map<std::string, std::string> map;

    // compile-time first, then run-time
    map.get(ID("food")) = "pizza";
    CHECK(map.get("food") == "pizza");

    // runtime-time first, then compile-time
    map.get("drink") = "beer";
    CHECK(map.get(ID("drink")) == "beer");

    CHECK(map.get(ID("food")) == "pizza");
    CHECK(map.get("drink") == "beer");

    CHECK(map.get(ID("starter"), "soup") == "soup");
    CHECK(map.get("starter", "salad") == "soup");

    CHECK(map.get("side", "rice") == "rice");
    CHECK(map.get(ID("side"), "peas") == "rice");
  }

  SUBCASE("test clear and contains") {
    semi::map<std::string, std::string> map;

    CHECK(!map.contains(ID("food")));
    CHECK(!map.contains("food"));

    // check again to see if contains by mistake add the element
    CHECK(!map.contains(ID("food")));
    CHECK(!map.contains("food"));

    map.get(ID("food")) = "pizza";
    CHECK(map.contains(ID("food")));
    CHECK(map.contains("food"));

    map.get("drink") = "beer";
    CHECK(map.contains("drink"));
    CHECK(map.contains(ID("drink")));

    map.get(ID("dessert")) = "ice cream";
    CHECK(map.contains("dessert"));
    CHECK(map.contains(ID("dessert")));

    map.get("starter") = "salad";
    CHECK(map.contains(ID("starter")));
    CHECK(map.contains("starter"));

    map.clear();

    CHECK(!map.contains(ID("food")));
    CHECK(!map.contains("food"));
    CHECK(!map.contains("drink"));
    CHECK(!map.contains(ID("drink")));
  }

  SUBCASE("test erase") {
    semi::map<std::string, std::string> map;

    map.get(ID("food")) = "pizza";
    map.get(ID("drink")) = "beer";
    map.get(ID("dessert")) = "ice cream";
    map.get(ID("starter")) = "soup";
    map.get(ID("side")) = "salad";

    // erase first
    map.erase(ID("food"));
    CHECK(!map.contains(ID("food")));
    CHECK(map.contains(ID("drink")));
    CHECK(map.contains(ID("dessert")));
    CHECK(map.contains(ID("starter")));
    CHECK(map.contains(ID("side")));

    // erase last
    map.erase("side");
    CHECK(!map.contains(ID("food")));
    CHECK(map.contains(ID("drink")));
    CHECK(map.contains(ID("dessert")));
    CHECK(map.contains(ID("starter")));
    CHECK(!map.contains(ID("side")));

    // try adding something
    map.get("bill") = "too much";
    CHECK(!map.contains(ID("food")));
    CHECK(map.contains(ID("drink")));
    CHECK(map.contains(ID("dessert")));
    CHECK(map.contains(ID("starter")));
    CHECK(!map.contains(ID("side")));
    CHECK(map.contains(ID("bill")));

    // erase middle
    map.erase(ID("dessert"));
    CHECK(!map.contains(ID("food")));
    CHECK(map.contains(ID("drink")));
    CHECK(!map.contains(ID("dessert")));
    CHECK(map.contains(ID("starter")));
    CHECK(!map.contains(ID("side")));
    CHECK(map.contains(ID("bill")));
  }

  SUBCASE("test independent maps do not influence each other") {
    semi::map<std::string, std::string> mapA;
    semi::map<std::string, std::string> mapB;

    mapA.get(ID("food")) = "pizza";
    CHECK(mapA.get("food") == "pizza");
    CHECK(!mapB.contains("food"));

    mapB.get(ID("food")) = "spaghetti";
    CHECK(mapA.get("food") == "pizza");
    CHECK(mapB.get("food") == "spaghetti");

    mapB.get("drink") = "beer";
    CHECK(mapB.get(ID("drink")) == "beer");
    CHECK(!mapA.contains(ID("drink")));
    CHECK(mapA.contains("food"));

    mapA.get("drink") = "soda";
    CHECK(mapA.get(ID("drink")) == "soda");
    CHECK(mapB.get(ID("drink")) == "beer");

    mapA.get(ID("starter")) = "salad";
    mapB.get("starter") = "soup";

    mapB.erase("drink");
    CHECK(mapA.contains("drink"));
    CHECK(mapA.contains(ID("drink")));
    CHECK(!mapB.contains("drink"));
    CHECK(!mapB.contains(ID("drink")));

    mapB.clear();
    CHECK(mapA.get(ID("starter")) == "salad");
    CHECK(mapA.get("food") == "pizza");
    CHECK(mapA.get(ID("drink")) == "soda");
    CHECK(!mapB.contains("food"));
    CHECK(!mapB.contains(ID("drink")));
  }
}
