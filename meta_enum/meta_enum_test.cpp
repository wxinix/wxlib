/*!
  MPL 1.1/GPL 2.0/LGPL 2.1 tri-license
  Copyright (C) 2022  Wuping Xin
*/

/*!
  MIT License
  Copyright (c) 2018 Tobias Widlund
*/

#include <iostream>
#include <meta_enum/meta_enum.hpp>

// helpers
constexpr int sum(int a, int b, int c)
{
  return a + b + c;
}

// Can be used for global or nested enums or enum classes.
//  - First arg is the name of the enum. 
//  - Second is the underlying type. 
//  - The rest are enum members.

meta_enum(
    Global,
    int,
    GlobalA,
    GlobalB = 3,
    GlobalC,
    GlobalD = 100);

meta_enum_class(
    GlobalClass,
    uint8_t,
    GlobalClassA,
    GlobalClassB = 3,
    GlobalClassC);

struct Nester
{
  meta_enum(
      Nested,
      int,
      NestedA,
      NestedB = 3);

  meta_enum_class(
      NestedClass,
      uint8_t,
      NestedClassA = 3,
      NestedClassB = 1 >> 3);
};

// Also works with very complex defined enum entries
meta_enum(
    Complex,
    int,
    First,
    Second = sum(1, {(2, ")h(),,\"ej", 1)}, 4 >> 2),
    Third = 4,
    Fourth
);

// Static assertions to verify that the complexly defined enum is parsed correctly
static_assert(Complex_meta.members.size() == 4);
static_assert(Complex_meta.members[0].string == "First");
static_assert(Complex_meta.members[1].string == R"( Second = sum(1, {(2, ")h(),,\"ej", 1)}, 4 >> 2))");
static_assert(Complex_meta.members[2].string == " Third = 4");
static_assert(Complex_meta.members[3].string == " Fourth");

static_assert(Complex_meta.members[0].name == "First");
static_assert(Complex_meta.members[1].name == "Second");
static_assert(Complex_meta.members[2].name == "Third");
static_assert(Complex_meta.members[3].name == "Fourth");

static_assert(Complex_meta.members[0].value == 0);
static_assert(Complex_meta.members[1].value == 3);
static_assert(Complex_meta.members[2].value == 4);
static_assert(Complex_meta.members[3].value == 5);

static_assert(Complex_meta.members[0].index == 0);
static_assert(Complex_meta.members[1].index == 1);
static_assert(Complex_meta.members[2].index == 2);
static_assert(Complex_meta.members[3].index == 3);

int main()
{
  // enum meta-objects are accessible with the _meta object. metaobject
  // has 'string' for full enum declaration string and 'members' for an
  // array of all members each member is an object of type MetaEnumMember
  std::cout << "global string: "
            << Global_meta.string
            << "\n";

  std::cout << "global count: "
            << Global_meta.members.size()
            << "\n";

  std::cout << "global class string: "
            << GlobalClass_meta.string
            << "\n";

  std::cout << "global class count: "
            << GlobalClass_meta.members.size()
            << "\n";

  std::cout << "nested string: "
            << Nester::Nested_meta.string
            << "\n";

  std::cout << "nested count: "
            << Nester::Nested_meta.members.size()
            << "\n";

  std::cout << "nested class string: "
            << Nester::NestedClass_meta.string
            << "\n";

  std::cout << "nested class count: "
            << Nester::NestedClass_meta.members.size()
            << "\n\n";

  // enum member objects have the following members:
  std::cout << "global members:\n";

  for (const auto &enumMember : Global_meta.members) {
    using UnderlyingType = decltype(Global_meta)::enumerator_t;
    std::cout << "index="
              << enumMember.index
              << ", name='"
              << enumMember.name
              << "', value: "
              << static_cast<UnderlyingType>(enumMember.value)
              << ", string='"
              << enumMember.string
              << "'\n";
  }
  std::cout << "\n";

  // helper functions are defined for convenience that operate on the _meta
  // data structure value_to_string gets the name of an enum value and
  // '__INVALID_ENUM_VAL__' on failure
  std::cout << "value_to_string: input="
            << Nester::NestedB
            << " output='"
            << Nester::Nested_value_to_string(Nester::NestedB)
            << "'\n";

  // These are different ways to access the meta enum member objects.
  // They return a std::optional which will be empty on bad input
  std::cout << "meta_from_name accesses through name. input="
            << "NestedClassA"
            << " found_index="
            << Nester::NestedClass_meta_from_name("NestedClassA")->index
            << "\n";

  std::cout << "meta_from_value accesses through enum value. input="
            << Nester::NestedB << " found_index="
            << Nester::Nested_meta_from_value(Nester::NestedB)->index
            << "\n";

  std::cout << "meta_from_index accesses through index. input="
            << 2
            << " found_name='" << GlobalClass_meta_from_index(2)->name
            << "'\n";
}
