# Meta Enum - Static reflection on enums in C++20

## Brief

Single-header facility for compile time reflection of enums in C++20.

### Features
 * Automatic string conversion of enum entries
 * Tracking of enum size via member count
 * Look up enum entries by index
 * Helper functions for converting between all the above
 * Supports both enum and enum class
 * Supports enums nested in types/namespaces/functions
 * Keeps track of the C++ code used to declare the enums
 * Single header.

### Compiled Example
See compiled code at: https://godbolt.org/z/TaPqPa

## Installation

Either make sure the `meta_enum.hpp` file is made available in your include paths or just copy it to your project.

## Usage

```cpp
#include <meta_enum.hpp>
```

### Declaring Enums

Use the macro `meta_enum` to declare your enums.

```cpp
meta_enum(Days, int, Monday, Tuesday, Wednesday, Thursday, Friday, Saturday, Sunday);
//equivalent to:
//enum Days : int { Monday, ...
```

Or use `meta_enum_class` for enum class.

The `meta_enum` macro supports assigning custom values to entries as per usual.
```cpp
meta_enum(MyEnum, uint8_t, First = 0, Second = 1 << 5, Third = myConstexprFunction<int>());
```

### Meta Enum Objects

By using `meta_enum` to declare `MyEnum`, you get the global constexpr object `MyEnum_meta` which stores a representation of your enum and is of type `MetaEnum`.

#### MetaEnum type - contains data for the whole enum declaration and holds all members

Typedefs:
 * enumerator_t contains the underlying integer type for the enum.

Members:
 * string  - contains the full enum declaration as a `string_view`
 * members - contains representations of all enum members represented as a std::array with MetaEnumMember objects

#### MetaEnumMember type - contains data for one particular enum member

Members:
 * value  - The enum value, for example `MyEnum::Second`
 * name   - String representation of the member. For example `"Second"`
 * string - The whole declaration string. For example `" Second = 1 << 5"`
 * index  - The numerical index of this member. For example `1`

### Convenience functions

A few functions are provided per `meta_enum` to ease usage.  For an enum with the name `MyEnum` you will get the following:
 * `std::string_view MyEnum_value_to_string(MyEnum)` converts an enum value to a textual representation. Will use the `.name` member of the member, or `"__INVALID_ENUM_VAL__"` on invalid input.
 * `std::optional<MetaEnumMember> MyEnum_meta_from_name(std::string_view)` Accesses the metaobject for a member found by name. Returns nullopt on invalid input.
 * `std::optional<MetaEnumMember> MyEnum_meta_from_value(MyEnum)` Accesses the metaobject for a member found by enum value. Returns nullopt on invalid input.
 * `std::optional<MetaEnumMember> MyEnum_meta_from_index(std::string_view)` Accesses the metaobject for a member found by enum member index. Returns nullopt on invalid input.

## Examples

See the file in the repo `meta_enum_test.cpp`

Last edited by Wuping Xin (@wxinix)