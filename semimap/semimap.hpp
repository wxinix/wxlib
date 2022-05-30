/*!
  MPL 1.1/GPL 2.0/LGPL 2.1 tri-license
  Copyright (C) 2022  Wuping Xin
*/

/*
 * Copyright (c) 2018 Fabian Renn-Giles
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies
 * or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef WXLIB_SEMIMAP_HPP
#define WXLIB_SEMIMAP_HPP

#include <cstring>
#include <memory>
#include <new>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

// check C++ version
#if (defined(_MSVC_LANG) && _MSVC_LANG < 202002L) || ((!defined(_MSVC_LANG)) && __cplusplus < 202002L)
#error semi::map and semi::static_map require C++20 support
#endif

#ifdef __GNUC__
#define semi_branch_expect(x, y) __builtin_expect(x, y)
#else
#define semi_branch_expect(x, y) x
#endif

namespace semi {

namespace detail {

// evaluates to the type returned by a constexpr lambda
template<typename IdentifierT>
using identifier_t = decltype(std::declval<IdentifierT>()());

constexpr std::size_t constexpr_strlen(const char *a_str) // NOLINT(misc-no-recursion)
{
  return a_str[0] == 0 ? 0 : constexpr_strlen(a_str + 1) + 1;
}

template<auto...>
struct TypeCreator
{
};

template<typename IdentifierT>
requires std::is_integral_v<identifier_t<IdentifierT>>
constexpr auto idval2type(IdentifierT a_id)
{
  return TypeCreator<a_id()>{};
}

template<typename IdentifierT, std::size_t ...Is>
requires std::is_same_v<identifier_t<IdentifierT>, const char *>
constexpr auto idval2type(IdentifierT a_id, std::index_sequence<Is...>)
{
  return TypeCreator<a_id()[Is]...>{};
}

template<typename IdentifierT>
requires std::is_same_v<identifier_t<IdentifierT>, const char *>
constexpr auto idval2type(IdentifierT a_id)
{
  return idval2type(a_id, std::make_index_sequence<constexpr_strlen(a_id())>{});
}

template<typename IdentifierT>
requires std::is_enum_v<identifier_t<IdentifierT>>
constexpr auto idval2type(IdentifierT a_id)
{
  return TypeCreator<a_id()>{};
}

template<typename, typename, bool>
struct default_tag
{
};

// super simple flat map implementation
template<typename KeyT, typename ValueT>
class flat_map
{
public:
  template<typename... ArgTs>
  ValueT &get(const KeyT &a_key, ArgTs &&... a_args)
  {
    for (auto &pair : storage_)
      if (a_key == pair.first) return pair.second;

    return storage_.emplace_back(a_key, ValueT(std::forward<ArgTs>(a_args)...)).second;
  }

  auto size() const
  {
    return storage_.size();
  }

  void erase(const KeyT &a_key)
  {
    for (auto it = storage_.begin(); it != storage_.end(); ++it) {
      if (it->first == a_key) {
        storage_.erase(it);
        return;
      }
    }
  }

  bool contains(const KeyT &a_key) const
  {
    for (auto &pair : storage_)
      if (pair.first == a_key) return true;

    return false;
  }

private:
  std::vector<std::pair<KeyT, ValueT>> storage_;
};

} // namespace detail

// forward declaration
template<typename, typename, typename>
class map;

template<typename KeyT, typename ValueT, typename TagT = detail::default_tag<KeyT, ValueT, true>>
class static_map
{
public:
  static_map() = delete;

  template<typename IdentifierT, typename... ArgTs>
  requires std::is_invocable_v<IdentifierT> && std::is_convertible_v<detail::identifier_t<IdentifierT>, KeyT>
  static ValueT &get(IdentifierT a_id, ArgTs &&... a_args)
  {
    using UniqueTypeForKeyValue = decltype(detail::idval2type(a_id));

    auto *mem = storage<UniqueTypeForKeyValue>;
    auto &l_init_flag = init_flag<UniqueTypeForKeyValue>;

    if (!semi_branch_expect(l_init_flag, true)) {
      KeyT key(a_id());
      auto it = runtime_map.find(key);

      if (it != runtime_map.end())
        it->second = u_ptr(new(mem) ValueT(std::move(*it->second)), {&l_init_flag});
      else
        runtime_map.emplace_hint(it, key, u_ptr(new(mem) ValueT(std::forward<ArgTs>(a_args)...), {&l_init_flag}));

      l_init_flag = true;
    }

    return *std::launder(reinterpret_cast<ValueT *>(mem));
  }

  template<typename... ArgTs>
  static ValueT &get(const KeyT &a_key, ArgTs &&... a_args)
  {
    auto it = runtime_map.find(a_key);

    if (it != runtime_map.end())
      return *it->second;
    else
      return *runtime_map.emplace_hint(it, a_key, u_ptr(new ValueT(std::forward<ArgTs>(a_args)...), {nullptr}))->second;
  }

  template<typename IdentifierT>
  requires std::is_invocable_v<IdentifierT>
  static bool contains(IdentifierT a_id)
  {
    using UniqueTypeForKeyValue = decltype(detail::idval2type(a_id));
    // @formatter:off
    if (!semi_branch_expect(init_flag<UniqueTypeForKeyValue>, true)) {
    // @formatter:on
      auto key = a_id();
      return contains(key);
    }

    return true;
  }

  static bool contains(const KeyT &a_key)
  {
    return (runtime_map.find(a_key) != runtime_map.end());
  }

  template<typename Identifier>
  requires std::is_invocable_v<Identifier>
  static void erase(Identifier identifier)
  {
    erase(identifier());
  }

  static void erase(const KeyT &a_key)
  {
    runtime_map.erase(a_key);
  }

  static void clear()
  {
    runtime_map.clear();
  }

private:
  struct ValueDeleter
  {
    void operator()(ValueT *a_value)
    {
      if (init_flag != nullptr) {
        a_value->~ValueT();
        *init_flag = false;
      } else {
        delete a_value;
      }
    }

    bool *init_flag = nullptr;
  };

  using u_ptr = std::unique_ptr<ValueT, ValueDeleter>;

  template<typename, typename, typename>
  friend
  class map;

  template<typename>
  alignas(ValueT) static char storage[sizeof(ValueT)];

  template<typename>
  static bool init_flag;

  static std::unordered_map<KeyT, std::unique_ptr<ValueT, ValueDeleter>> runtime_map;
};

template<typename KeyT, typename ValueT, typename TagT>
std::unordered_map<KeyT, typename static_map<KeyT, ValueT, TagT>::u_ptr> static_map<KeyT, ValueT, TagT>::runtime_map;

template<typename KeyT, typename ValueT, typename TagT>
template<typename>
alignas(ValueT) char static_map<KeyT, ValueT, TagT>::storage[sizeof(ValueT)];

template<typename KeyT, typename ValueT, typename TagT>
template<typename>
bool static_map<KeyT, ValueT, TagT>::init_flag = false;

template<typename KeyT, typename ValueT, typename TagT = detail::default_tag<KeyT, ValueT, false>>
class map
{
public:
  ~map()
  {
    clear();
  }

  template<typename IdentifierT, typename... ArgTs>
  ValueT &get(IdentifierT a_key, ArgTs &... a_args)
  {
    return staticmap::get(a_key).get(this, std::forward<ArgTs>(a_args)...);
  }

  template<typename IdentifierT>
  bool contains(IdentifierT a_key)
  {
    return staticmap::contains(a_key) ? staticmap::get(a_key).contains(this) : false;
  }

  template<typename IdentifierT>
  void erase(IdentifierT a_key)
  {
    if (staticmap::contains(a_key)) {
      auto &map = staticmap::get(a_key);
      map.erase(this);
      if (map.size() == 0) staticmap::erase(a_key);
    }
  }

  void clear()
  {
    auto it = staticmap::runtime_map.begin();

    while (it != staticmap::runtime_map.end()) {
      auto &map = *it->second;
      map.erase(this);

      if (map.size() == 0) {
        it = staticmap::runtime_map.erase(staticmap::runtime_map.find(it->first));
        continue;
      }

      ++it;
    }
  }

private:
  using staticmap = static_map<KeyT, detail::flat_map<map<KeyT, ValueT> *, ValueT>, TagT>;
};

#undef semi_branch_expect

} // namespace semi

#endif