/*!
  MPL 1.1/GPL 2.0/LGPL 2.1 tri-license
  Copyright (C) 2022  Wuping Xin
*/

/*!
  MIT License
  Copyright (c) 2019 Mike Loomis
*/

#ifndef WXLIB_MSGPACK_HPP
#define WXLIB_MSGPACK_HPP

#include <array>
#include <bitset>
#include <chrono>
#include <cmath>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <vector>

#include <matchit/matchit.hpp>

namespace msgpack {

enum class UnpackerError
{
  OutOfRange = 1,
  IntegerOverflow = 2,
  DataNotMatchType = 3,
  BadStdArraySize = 4
};

enum class PackerError
{
  LengthError = 1
};

struct UnpackerErrorCategory : public std::error_category
{
public:
  const char *name() const noexcept override
  {
    return "unpacker";
  };

  std::string message(int ev) const override
  {
    switch (static_cast<msgpack::UnpackerError>(ev)) {
      case UnpackerError::OutOfRange: //
        return "out of range data-access during deserialization";
      case UnpackerError::IntegerOverflow://
        return "data overflows specified integer type";
      case UnpackerError::DataNotMatchType://
        return "data does not match type of object";
      case UnpackerError::BadStdArraySize://
        return "data has a different size than specified std::array object";
      default: //
        return "(unrecognized error)";
    }
  };
};

struct PackerErrorCategory : public std::error_category
{
public:
  const char *name() const noexcept override
  {
    return "packer";
  };

  std::string message(int ev) const override
  {
    switch (static_cast<msgpack::PackerError>(ev)) {
      case PackerError::LengthError: //
        return "length of map, array, string or binary data exceeding 2^32 -1 elements";
      default: //
        return "(unrecognized error)";
    }
  };
};

const UnpackerErrorCategory unpacker_error_category{};
const PackerErrorCategory packer_error_category{};

inline std::error_code make_error_code(msgpack::UnpackerError e)
{
  return {static_cast<int>(e), unpacker_error_category};
}

inline std::error_code make_error_code(msgpack::PackerError e)
{
  return {static_cast<int>(e), packer_error_category};
}

}

namespace std {

template<>
struct [[maybe_unused]] is_error_code_enum<msgpack::UnpackerError> : public true_type
{
};

template<>
struct [[maybe_unused]] is_error_code_enum<msgpack::PackerError> : public true_type
{
};

}

namespace msgpack {

using namespace matchit;

/*!
  positive fixint = 0x00 - 0x7F
  fixmap = 0x80 - 0x8F
  fixarray = 0x90 - 0x9A
  fixstr = 0xA0 - 0xBF
  negative fixint = 0xE0 - 0xFF
*/
enum FormatConstants : uint8_t
{
  nil = 0xC0,
  false_bool = 0xC2,
  true_bool = 0xC3,
  bin8 = 0xC4,
  bin16 = 0xC5,
  bin32 = 0xC6,
  ext8 = 0xC7,
  ext16 = 0xC8,
  ext32 = 0xC9,
  float32 = 0xCA,
  float64 = 0xCB,
  uint8 = 0xCC,
  uint16 = 0xCD,
  uint32 = 0xCE,
  uint64 = 0xCF,
  int8 = 0xD0,
  int16 = 0xD1,
  int32 = 0xD2,
  int64 = 0xD3,
  fixext1 = 0xD4,
  fixext2 = 0xD5,
  fixext4 = 0xD6,
  fixext8 = 0xD7,
  fixext16 = 0xD8,
  str8 = 0xD9,
  str16 = 0xDA,
  str32 = 0xDB,
  array16 = 0xDC,
  array32 = 0xDD,
  map16 = 0xDE,
  map32 = 0xDF
};

template<typename T>
struct is_stdvector : std::false_type
{
};

template<typename T, typename A>
struct is_stdvector<std::vector<T, A>> : std::true_type
{
};

template<typename T>
struct is_stdlist : std::false_type
{
};

template<typename T, typename A>
struct is_stdlist<std::list<T, A>> : std::true_type
{
};

template<typename T>
struct is_stdmap : std::false_type
{
};

template<typename T, typename A>
struct is_stdmap<std::map<T, A>> : std::true_type
{
};

template<typename T, typename A>
struct is_stdmap<std::unordered_map<T, A>> : std::true_type
{
};

template<typename T>
struct is_stdset : std::false_type
{
};

template<typename T, typename A>
struct is_stdset<std::set<T, A>> : std::true_type
{
};

template<typename T>
struct is_stdarray : std::false_type
{
};

template<typename T, std::size_t N>
struct is_stdarray<std::array<T, N>> : std::true_type
{
};

template<typename T>
concept MsgPackIntegral = std::is_integral_v<T> && sizeof(T) <= 8;

template<typename T>
concept MsgPackMap = is_stdmap<T>::value;

template<typename T>
concept MsgPackBinary = std::same_as<T, std::vector<uint8_t>>;

template<typename T>
concept MsgPackArray = (is_stdarray<T>::value || is_stdvector<T>::value || is_stdlist<T>::value || is_stdset<T>::value)
    && (!MsgPackBinary<T>);

template<typename T>
concept MsgPackArrayOrMap = MsgPackArray<T> || MsgPackMap<T>;

template<typename T>
concept MsgPackString = std::same_as<T, std::string>;

template<typename T>
concept MsgPackStringOrBinary = MsgPackString<T> || MsgPackBinary<T>;

template<typename T>
concept MsgPackFloatingPoint = std::is_floating_point_v<T> && (sizeof(T) == 4 || sizeof(T) == 8)
    && (std::numeric_limits<T>::radix == 2);

class Packer
{
public:
  Packer() = default;
  Packer(const Packer &) = delete;
  Packer(Packer &&) = delete;
  Packer &operator=(Packer &) = delete;
  Packer &operator=(Packer &&) = delete;

  template<typename ... Ts>
  void operator()(const Ts &... args)
  {
    (pack_type(std::forward<const Ts &>(args)), ...);
  }

  template<typename ... Ts>
  void process(const Ts &... args)
  {
    (pack_type(std::forward<const Ts &>(args)), ...);
  }

  const std::vector<uint8_t> &vector() const
  {
    return m_serialized_object;
  }

  void clear()
  {
    m_serialized_object.clear();
  }

  std::error_code ec{};
private:
  std::vector<uint8_t> m_serialized_object;

  template<typename T>
  void pack_type(const T &a_value)
  {
    if (ec) return;
    auto recursive_packer = Packer{};
    const_cast<T &>(a_value).pack(recursive_packer);
    pack_type(recursive_packer.vector());
  }

  template<MsgPackArrayOrMap T>
  void pack_type(const T &a_value)
  {
    if (ec) return;
    uint8_t l_type = MsgPackMap<T> ? FormatConstants::map16 : FormatConstants::array16;
    uint8_t l_mask = (a_value.size() < 16) ? (MsgPackMap<T> ? 0b10000000 : 0b10010000) : 0x00;

    size_t l_nbytes = match(a_value.size())(
        pattern | (_ < 16) =
            [&] { return sizeof(uint8_t); },

        pattern | (_ < std::numeric_limits<uint16_t>::max()) =
            [&] { return m_serialized_object.emplace_back(l_type), sizeof(uint16_t); },

        pattern | (_ < std::numeric_limits<uint32_t>::max()) =
            [&] { return m_serialized_object.emplace_back(l_type + 1), sizeof(uint32_t); },

        pattern | _ = [&]() { return (ec = PackerError::LengthError), 0; }
    );

    if (l_nbytes == 0)
      return;

    for (auto i = l_nbytes; i > 0; --i)
      m_serialized_object.emplace_back(uint8_t(a_value.size() >> (8U * (i - 1)) | l_mask & 0xFF));

    for (const auto &el : a_value)
      if constexpr (MsgPackMap<T>)
        pack_type(std::get<0>(el)), pack_type(std::get<1>(el));
      else
        pack_type(el);
  }

  template<MsgPackStringOrBinary T>
  void pack_type(const T &a_value)
  {
    if (ec) return;
    uint8_t l_type = MsgPackString<T> ? FormatConstants::str8 : FormatConstants::bin8;

    if (MsgPackString<T> && a_value.size() < 32) {
      m_serialized_object.emplace_back(uint8_t(a_value.size()) | 0b10100000);
    } else {
      size_t l_nbytes = match(a_value.size())(
          pattern | _ < std::numeric_limits<uint8_t>::max() =
              [&]() { return m_serialized_object.emplace_back(l_type), sizeof(uint8_t); },

          pattern | _ < std::numeric_limits<uint16_t>::max() =
              [&]() { return m_serialized_object.emplace_back(l_type + 1), sizeof(uint16_t); },

          pattern | _ < std::numeric_limits<uint32_t>::max() =
              [&]() { return m_serialized_object.emplace_back(l_type + 2), sizeof(uint32_t); },

          pattern | _ = [&]() { return (ec = PackerError::LengthError), 0; }
      );

      if (l_nbytes == 0)
        return;

      for (auto i = l_nbytes; i > 0; --i)
        m_serialized_object.emplace_back(uint8_t(a_value.size() >> (8U * (i - 1)) & 0xFF));
    }

    for (const auto &el : a_value)
      m_serialized_object.emplace_back(static_cast<uint8_t>(el));
  }

  template<MsgPackIntegral T>
  void pack_type(const T &a_value)
  {
    if (ec) return;
    uint64_t l_value = std::make_unsigned_t<T>(a_value);
    size_t l_nbytes{0};
    auto l_signed = std::is_signed_v<T>;

    for (l_nbytes = 8; l_nbytes > 1; --l_nbytes)
      if ((uint8_t(l_value >> 8U * (l_nbytes - 1)) & 0xFFL) != 0) break;

    uint8_t l_type = match(l_nbytes)(
        pattern | _ > 4 = [&]() { return (l_nbytes = 8, l_signed ? int64 : uint64); },
        pattern | _ > 2 = [&]() { return (l_nbytes = 4, l_signed ? int32 : uint32); },
        pattern | _ > 1 = [&]() { return (l_nbytes = 2, l_signed ? int16 : uint16); },

        pattern | _ = [&]() {
          return ((uint8_t(a_value & 0x80) == 0) || (uint8_t(a_value & 0xE0) == 0xE0)) ?
                 (l_nbytes = 0, 0) : (l_signed ? int8 : uint8);
        }
    );

    (l_type > 0) ? m_serialized_object.emplace_back(l_type) : m_serialized_object.emplace_back(uint8_t(a_value));

    for (auto i = l_nbytes; i > 0U; --i)
      m_serialized_object.emplace_back(uint8_t(l_value >> (8U * (i - 1)) & 0xFFL));
  }

  template<MsgPackFloatingPoint T>
  void pack_type(const T &a_value)
  {
    if (ec) return;
    using int_t = std::conditional_t<sizeof(T) == 4, uint32_t, uint64_t>;
    T integral_part;

    if (std::modf(a_value, &integral_part) == 0)
      return pack_type(int64_t(integral_part));

    auto exponent = std::ilogb(a_value);
    auto full_mantissa = a_value / std::scalbn(1.0, exponent);
    auto sign_mask = std::bitset<sizeof(T) * 8>(int_t(std::signbit(full_mantissa)) << sizeof(T) * 8 - 1);

    auto normalized_mantissa_mask = std::bitset<sizeof(T) * 8>();
    T implied_mantissa = std::fabs(full_mantissa) - 1.0f;
    auto significant = sizeof(T) == sizeof(float) ? 23 : 52;

    for (auto i = significant; i > 0; --i) {
      integral_part = 0;
      implied_mantissa *= 2;
      implied_mantissa = std::modf(implied_mantissa, &integral_part);

      if (uint8_t(integral_part) == 1)
        normalized_mantissa_mask |= std::bitset<sizeof(T) * 8>(int_t(1) << (i - 1));
    }

    int_t ieee754_float;
    auto excess = (sizeof(T) == 4 ? std::pow(2, 7) : std::pow(2, 10)) - 1;
    auto excess_exponent_mask = std::bitset<sizeof(T) * 8>(int_t(exponent + excess) << significant);

    if (sizeof(T) == 4)
      ieee754_float = (sign_mask | excess_exponent_mask | normalized_mantissa_mask).to_ulong();
    else
      ieee754_float = (sign_mask | excess_exponent_mask | normalized_mantissa_mask).to_ullong();

    match(sizeof(T))(
        pattern | 4 = [&]() { m_serialized_object.emplace_back(FormatConstants::float32); },
        pattern | 8 = [&]() { m_serialized_object.emplace_back(FormatConstants::float64); }
    );

    for (auto i = sizeof(T); i > 0; --i)
      m_serialized_object.emplace_back(uint8_t(ieee754_float >> (8U * (i - 1)) & 0xFF));
  }

  template<typename T>
  void pack_type(const std::chrono::time_point<T> &a_value)
  {
    if (ec) return;
    pack_type(a_value.time_since_epoch().count());
  }

  template<>
  void pack_type(const std::nullptr_t &/*value*/)
  {
    if (ec) return;
    m_serialized_object.emplace_back(FormatConstants::nil);
  }

  template<>
  void pack_type(const bool &a_value)
  {
    if (ec) return;
    if (a_value)
      m_serialized_object.emplace_back(FormatConstants::true_bool);
    else
      m_serialized_object.emplace_back(FormatConstants::false_bool);
  }
};

class Unpacker
{
public:
  Unpacker(const Unpacker &) = delete;
  Unpacker(Unpacker &&) = delete;
  Unpacker &operator=(Unpacker &) = delete;
  Unpacker &operator=(Unpacker &&) = delete;

  Unpacker() : m_begin(nullptr), m_end(nullptr)
  {};

  explicit Unpacker(const uint8_t *a_start, std::size_t a_size) : m_begin(a_start), m_end(a_start + a_size)
  {};

  template<typename ... Ts>
  void operator()(Ts &... args)
  {
    (unpack_type(std::forward<Ts &>(args)), ...);
  }

  template<typename ... Ts>
  void process(Ts &... args)
  {
    (unpack_type(std::forward<Ts &>(args)), ...);
  }

  void set_data(const uint8_t *a_begin, std::size_t a_size)
  {
    m_begin = a_begin;
    m_end = m_begin + a_size;
  }

  std::error_code ec{};

private:
  const uint8_t *m_begin;
  const uint8_t *m_end;

  uint8_t current_byte()
  {
    return (m_begin < m_end) ? *m_begin : (ec = UnpackerError::OutOfRange, 0);
  }

  void next(int64_t bytes = 1)
  {
    if (m_end - m_begin >= 0)
      m_begin += bytes;
    else
      ec = UnpackerError::OutOfRange;
  }

  template<typename T>
  void unpack_type(T &value)
  {
    if (ec) return;
    auto recursive_data = std::vector<uint8_t>{};
    unpack_type(recursive_data);

    auto recursive_unpacker = Unpacker{recursive_data.data(), recursive_data.size()};
    value.pack(recursive_unpacker);
    ec = recursive_unpacker.ec;
  }

  template<MsgPackIntegral T>
  void unpack_type(T &a_value)
  {
    if (ec) return;
    auto is_overflow = [&](size_t act, size_t min) { return (act < min) && (ec = UnpackerError::IntegerOverflow, true); };
    uint8_t l_byte{current_byte()};

    uint8_t l_nbytes = match(l_byte)(
        //@formatter:off
        pattern | or_(int64, uint64) = [&]() { return is_overflow(sizeof(T), 8) ? 0 : 8; },
        pattern | or_(int32, uint32) = [&]() { return is_overflow(sizeof(T), 4) ? 0 : 4; },
        pattern | or_(int16, uint16) = [&]() { return is_overflow(sizeof(T), 2) ? 0 : 2; },
        pattern | or_(int8 , uint8 ) = [&]() { return is_overflow(sizeof(T), 1) ? 0 : 1; },
        //@formatter:on
        pattern | _ = [&] {
          return ((l_byte & 0x80) == 0x00) || ((l_byte & 0xE0) == 0xE0) ?
                 (a_value = l_byte, 0) : (ec = UnpackerError::DataNotMatchType, 0);
        }
    );

    if (next(), l_nbytes > 0) {
      uint64_t l_value64{0};

      for (auto i = l_nbytes; i > 0; --i) {
        l_value64 |= uint64_t(current_byte()) << 8U * (i - 1);
        next();
      }

      a_value = T(l_value64);
    }
  }

  template<MsgPackArrayOrMap T>
  void unpack_type(T &a_value)
  {
    if (ec) return;
    uint32_t l_size = [&]() {
      auto l_nbytes = match(current_byte())(
          //@formatter:off
          pattern | or_(array32, map32) = expr(4),
          pattern | or_(array16, map16) = expr(2),
          pattern | _                   = expr(0)
          //@formatter:on
      );

      uint32_t l_sz{0};

      if (l_nbytes > 0) {
        next();
        for (auto i = l_nbytes; i > 0; i--) {
          l_sz += uint32_t(current_byte()) << 8 * (i - 1);
          next();
        }
      } else {
        l_sz = current_byte() & 0b00001111;
        next();
      }

      return l_sz;
    }();

    if (is_stdarray<T>::value && a_value.size() != l_size) {
      ec = UnpackerError::BadStdArraySize;
      return;
    }

    for (auto i = 0U; i < l_size; i++) {
      if constexpr (MsgPackArray<T>) {
        typename T::value_type l_value{};
        unpack_type(l_value);
        if constexpr (is_stdarray<T>::value)
          a_value[i] = std::move(l_value);
        else
          a_value.emplace_back(l_value);
      } else {
        typename T::key_type l_key{};
        typename T::mapped_type l_value{};
        unpack_type(l_key);
        unpack_type(l_value);
        a_value.insert_or_assign(l_key, l_value);
      }
    }
  }

  template<MsgPackStringOrBinary T>
  void unpack_type(T &a_value)
  {
    if (ec) return;
    uint32_t l_size = [&]() {
      auto l_nbytes = match(current_byte())(
          //@formatter:off
          pattern | or_(str32, bin32) = expr(4),
          pattern | or_(str16, bin16) = expr(2),
          pattern | or_(str8,  bin8 ) = expr(1),
          pattern | _                 = expr(0)
          //@formatter:on
      );

      uint32_t l_sz{0};

      if (l_nbytes > 0) {
        next();
        for (auto i = l_nbytes; i > 0; i--) {
          l_sz += uint32_t(current_byte()) << 8 * (i - 1);
          next();
        }
      } else {
        l_sz = current_byte() & 0b00011111;
        next();
      }

      return l_sz;
    }();

    if (m_begin + l_size <= m_end) {
      a_value = T{m_begin, m_begin + l_size};
      next(l_size);
    } else {
      ec = UnpackerError::OutOfRange;
    }
  }

  template<MsgPackFloatingPoint T>
  void unpack_type(T &a_value)
  {
    if (ec) return;
    match(current_byte())(
        // Stored as IEEE-754 floating point format.
        pattern | or_(float64, float32) = [&]() {
          using int_t = std::conditional_t<sizeof(T) == 4, uint32_t, uint64_t>;
          int_t l_data{0};
          next();

          for (auto i = sizeof(T); i > 0; --i) {
            l_data += std::conditional_t<sizeof(T) == 8, uint64_t, uint8_t>(current_byte()) << 8 * (i - 1);
            next();
          }

          auto l_bits = std::bitset<8 * sizeof(T)>(l_data);
          T l_mantissa = 1.0;

          for (auto i = sizeof(T) == 8 ? 52U : 23U; i > 0; --i)
            if (l_bits[i - 1])
              l_mantissa += 1.0 / (int_t(1) << ((sizeof(T) == 8 ? 53U : 24U) - i));

          if (l_bits[8 * sizeof(T) - 1])
            l_mantissa *= -1;

          std::conditional_t<sizeof(T) == 8, uint16_t, uint8_t> l_exponent{0};

          for (auto i = 0U; i < (sizeof(T) == 8 ? 11 : 8); ++i)
            l_exponent += l_bits[i + (sizeof(T) == 8 ? 52 : 23)] << i;

          l_exponent -= (sizeof(T) == 8 ? 1023 : 127);
          a_value = ldexp(l_mantissa, l_exponent);
        },
        // Could have been stored in integral format: int8, int16, int32, int64, or fixint.
        pattern | _ = [&]() {
          int64_t l_data{0};
          unpack_type(l_data);
          a_value = T(l_data);
        }
    );
  }

  template<typename ClockT, typename DurationT>
  void unpack_type(std::chrono::time_point<ClockT, DurationT> &a_value)
  {
    if (ec) return;
    using timepoint_t = typename std::chrono::time_point<ClockT, DurationT>;
    using rep_t = typename timepoint_t::rep;

    auto l_placeholder = rep_t{};

    unpack_type(l_placeholder);
    a_value = static_cast<timepoint_t>(DurationT(l_placeholder));
  }

  template<>
  void unpack_type(std::nullptr_t &/*value*/)
  {
    if (ec) return;
    next();
  }

  template<>
  void unpack_type(bool &a_value)
  {
    if (ec) return;
    a_value = current_byte() != 0xC2;
    next();
  }
};

/*!
 * Packable concept requires the type to have a method named pack, which takes
 * reference (and must be a reference) to a Packer or Unpacker instance, and
 * returns void.
 *
 * @tparam T
 */
template<typename T>
concept Packable = requires(T packable, Packer &packer, Unpacker &unpacker) {
  //@formatter:off
  { packable.pack(std::forward<Packer&>(packer))     } -> std::same_as<void>;
  { packable.pack(std::forward<Unpacker&>(unpacker)) } -> std::same_as<void>;
  //@formatter:on
};

template<Packable T>
std::vector<uint8_t> pack(T &a_packable, std::error_code &a_ec)
{
  auto l_packer = Packer{};
  a_packable.pack(l_packer);
  a_ec = l_packer.ec;
  return l_packer.vector();
}

template<Packable T>
std::vector<uint8_t> pack(T &&a_packable, std::error_code &a_ec)
{
  auto l_packer = Packer{};
  a_packable.pack(l_packer);
  a_ec = l_packer.ec;
  return l_packer.vector();
}

template<Packable T>
std::vector<uint8_t> pack(T &a_packable)
{
  std::error_code l_ec;
  return pack(std::forward<T&>(a_packable), l_ec);
}

template<Packable T>
std::vector<uint8_t> pack(T &&a_packable)
{
  std::error_code l_ec;
  return pack(std::forward<T&&>(a_packable), l_ec);
}

template<Packable T>
T unpack(const uint8_t *a_start, const std::size_t a_size, std::error_code &a_ec)
{
  auto l_packable = T{};
  auto l_unpacker = Unpacker(a_start, a_size);
  l_packable.pack(l_unpacker);
  a_ec = l_unpacker.ec;
  return l_packable;
}

template<Packable T>
T unpack(const uint8_t *a_start, const std::size_t a_size)
{
  std::error_code l_ec{};
  return unpack<T>(a_start, a_size, l_ec);
}

template<Packable T>
T unpack(const std::vector<uint8_t> &a_data, std::error_code &a_ec)
{
  return unpack<T>(a_data.data(), a_data.size(), a_ec);
}

template<Packable T>
T unpack(const std::vector<uint8_t> &a_data)
{
  std::error_code l_ec;
  return unpack<T>(a_data.data(), a_data.size(), l_ec);
}

}
#endif
