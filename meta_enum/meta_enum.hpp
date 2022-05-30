/*!
  MPL 1.1/GPL 2.0/LGPL 2.1 tri-license
  Copyright (C) 2022  Wuping Xin
*/

/*!
  MIT License
  Copyright (c) 2018 Tobias Widlund
*/

#ifndef WXLIB_META_ENUM_HPP
#define WXLIB_META_ENUM_HPP

#include <array>
#include <concepts>
#include <optional>
#include <string_view>

template<typename EnumT>/* */
requires std::is_enum_v<EnumT>
struct MetaEnumMember
{
  EnumT value = {};
  std::string_view name;
  std::string_view string;
  size_t index = {};
};

template<typename EnumT, typename EnumeratorT, size_t size>/* */
requires std::is_enum_v<EnumT> && std::is_integral_v<EnumeratorT>
struct MetaEnum
{
  using enumerator_t = EnumeratorT;
  std::string_view string;
  std::array<MetaEnumMember<EnumT>, size> members = {};
};

namespace meta_enum_internal {

constexpr bool IsNested(size_t a_brackets, bool a_quote)
{
  return a_brackets != 0 || a_quote;
}

constexpr size_t NextEnumCommaOrEndPos(size_t a_start, std::string_view a_enum_str)
{
  size_t brackets = 0; //()[]{}
  bool quote = false;  //""
  char last_char = '\0';
  char next_char = '\0';

  auto feed_counters = [&](char c) constexpr {
    if (quote) {
      if (last_char != '\\' && c == '"') //ignore " if they are backslashes
        quote = false;
      return;
    }

    switch (c) {
      case '"':
        if (last_char != '\\') //ignore " if they are backslashes
          quote = true;
        break;
      case '(':
      case '<':
        if (last_char == '<' || next_char == '<')
          break;
        [[fallthrough]];
      case '{':++brackets;
        break;
      case ')':
      case '>':
        if (last_char == '>' || next_char == '>')
          break;
        [[fallthrough]];
      case '}':--brackets;
        break;
      default:break;
    }
  };

  size_t current = a_start;
  for (; current < a_enum_str.size() && (IsNested(brackets, quote) || (a_enum_str[current] != ',')); ++current) {
    feed_counters(a_enum_str[current]);
    last_char = a_enum_str[current];
    next_char = current + 2 < a_enum_str.size() ? a_enum_str[current + 2] : '\0';
  }

  return current;
}

constexpr bool IsAllowedIdChar(char c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

constexpr std::string_view ParseEnumMemberName(std::string_view a_member_str)
{
  size_t name_start = 0;
  while (!IsAllowedIdChar(a_member_str[name_start])) {
    ++name_start;
  }

  size_t name_size = 0;
  while ((name_start + name_size) < a_member_str.size() && IsAllowedIdChar(a_member_str[name_start + name_size])) {
    ++name_size;
  }

  return {&a_member_str[name_start], name_size};
}

template<typename EnumT, typename EnumeratorT, size_t size>
constexpr MetaEnum<EnumT, EnumeratorT, size> ParseMetaEnum(std::string_view in, const std::array<EnumT, size> &a_values)
{
  MetaEnum<EnumT, EnumeratorT, size> result;
  result.string = in;

  std::array<std::string_view, size> member_str;
  size_t amount_filled = 0;
  size_t startpos = 0;

  while (amount_filled < size) {
    const size_t endpos = NextEnumCommaOrEndPos(startpos + 1, in);
    size_t strlen = endpos - startpos;

    if (startpos != 0) {
      ++startpos;
      --strlen;
    }

    member_str[amount_filled] = {&in[startpos], strlen};
    ++amount_filled;
    startpos = endpos;
  }

  for (size_t i = 0; i < member_str.size(); ++i) {
    result.members[i].name = ParseEnumMemberName(member_str[i]);
    result.members[i].string = member_str[i];
    result.members[i].value = a_values[i];
    result.members[i].index = i;
  }

  return result;
}

template<typename EnumeratorT>
struct IntWrapper
{
  constexpr IntWrapper() : value{0}, empty{true}
  {
  }

  explicit(false) constexpr IntWrapper(EnumeratorT in) : value{in}, empty{false} // NOLINT(google-explicit-constructor)
  {
  }

  constexpr IntWrapper &operator=(EnumeratorT in)
  {
    value = in;
    empty = false;
    return *this;
  }

  EnumeratorT value;
  bool empty;
};

template<typename EnumT, typename EnumeratorT, size_t size>
constexpr std::array<EnumT, size> ResolveEnumValuesArray(const std::initializer_list<IntWrapper<EnumeratorT>> &in)
{
  std::array<EnumT, size> result{};

  EnumeratorT next_value = 0;
  for (size_t i = 0; i < size; ++i) {
    auto it = in.begin();
    auto wrapper = (std::advance(it, i), *it);
    EnumeratorT new_value = wrapper.empty ? next_value : wrapper.value;
    next_value = new_value + 1;
    result[i] = static_cast<EnumT>(new_value);
  }

  return result;
}
}

#define meta_enum(Type, EnumeratorT, ...)\
  enum Type: EnumeratorT { __VA_ARGS__};\
  constexpr static auto Type##_internal_size = []() constexpr {\
    using IntWrapperType = meta_enum_internal::IntWrapper<EnumeratorT>;\
    IntWrapperType __VA_ARGS__;\
    return std::initializer_list<IntWrapperType>{__VA_ARGS__}.size();\
  };\
  constexpr static auto Type##_meta = meta_enum_internal::ParseMetaEnum<\
    Type, EnumeratorT, Type##_internal_size()>(#__VA_ARGS__, []() {\
    using IntWrapperType = meta_enum_internal::IntWrapper<EnumeratorT>;\
    IntWrapperType __VA_ARGS__;\
    return meta_enum_internal::ResolveEnumValuesArray<\
        Type, EnumeratorT, Type##_internal_size()>({__VA_ARGS__});\
  }());\
  constexpr static auto Type##_value_to_string = [](Type e) {\
    for(const auto& member: Type##_meta.members) {\
      if(member.value == e)\
        return member.name;\
    }\
    return std::string_view("__INVALID_ENUM_VAL__");\
  \
  };\
  constexpr static auto Type##_meta_from_name = \
    [](std::string_view s)->std::optional<MetaEnumMember<Type>> {\
    for(const auto& member: Type##_meta.members) {\
      if(member.name == s)\
        return member;\
    }\
    return std::nullopt;\
  \
  };\
  constexpr static auto Type##_meta_from_value = \
    [](Type v)->std::optional<MetaEnumMember<Type>> {\
    for(const auto& member: Type##_meta.members) {\
      if(member.value == v)\
        return member;\
    }\
    return std::nullopt;\
  \
  };\
  constexpr static auto Type##_meta_from_index = [](size_t i) {\
    std::optional<MetaEnumMember<Type>> result;\
    if(i < Type##_meta.members.size())\
      result = Type##_meta.members[i];\
    return result;\
  \
  }

#define meta_enum_class(Type, EnumeratorT, ...)\
  enum class Type: EnumeratorT { __VA_ARGS__};\
  constexpr static auto Type##_internal_size = []() constexpr {\
    using IntWrapperType = meta_enum_internal::IntWrapper<EnumeratorT>;\
    IntWrapperType __VA_ARGS__;\
    return std::initializer_list<IntWrapperType>{__VA_ARGS__}.size();\
  };\
  constexpr static auto Type##_meta = meta_enum_internal::ParseMetaEnum<\
    Type, EnumeratorT, Type##_internal_size()>(#__VA_ARGS__, []() {\
    using IntWrapperType = meta_enum_internal::IntWrapper<EnumeratorT>;\
    IntWrapperType __VA_ARGS__;\
    return meta_enum_internal::ResolveEnumValuesArray<\
        Type, EnumeratorT, Type##_internal_size()>({__VA_ARGS__});\
  }());\
  constexpr static auto Type##_value_to_string = [](Type e) {\
    for(const auto& member: Type##_meta.members) {\
      if(member.value == e)\
        return member.name;\
    }\
    return std::string_view("__INVALID_ENUM_VAL__");\
  \
  };\
  constexpr static auto Type##_meta_from_name = \
    [](std::string_view s)->std::optional<MetaEnumMember<Type>> {\
    for(const auto& member: Type##_meta.members) {\
      if(member.name == s)\
        return member;\
    }\
    return std::nullopt;\
  \
  };\
  constexpr static auto Type##_meta_from_value = \
    [](Type v)->std::optional<MetaEnumMember<Type>> {\
    for(const auto& member: Type##_meta.members) {\
      if(member.value == v)\
        return member;\
    }\
    return std::nullopt;\
  \
  };\
  constexpr static auto Type##_meta_from_index = [](size_t i) {\
    std::optional<MetaEnumMember<Type>> result;\
    if(i < Type##_meta.members.size())\
      result = Type##_meta.members[i];\
    return result;\
  \
  }

#endif
