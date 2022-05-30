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

using namespace matchit;

template<typename T>
using limits = std::numeric_limits<T>;

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
    //@formatter:off
    return match(static_cast<msgpack::UnpackerError>(ev))(
        pattern | UnpackerError::OutOfRange       = expr("out of range data-access during deserialization"),
        pattern | UnpackerError::IntegerOverflow  = expr("data overflows specified integer type"),
        pattern | UnpackerError::DataNotMatchType = expr("data does not match type of object"),
        pattern | UnpackerError::BadStdArraySize  = expr("data has a different size than specified std::array object"),
        pattern | _                               = expr("(unrecognized error)")
    );
    //@formatter:on
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
    // @formatter:off
    return match(static_cast<msgpack::PackerError>(ev)) (
      pattern | PackerError::LengthError = expr("length of map, array, string or binary data exceeding 2^32 -1 elements"),
      pattern | _                        = expr("(unrecognized error)")
    );
    // @formatter:on
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
concept MsgPackArray = (!MsgPackBinary<T>)
    && (is_stdarray<T>::value
        || is_stdvector<T>::value
        || is_stdlist<T>::value
        || is_stdset<T>::value);

template<typename T>
concept MsgPackArrayOrMap = MsgPackArray<T> || MsgPackMap<T>;

template<typename T>
concept MsgPackString = std::same_as<T, std::string>;

template<typename T>
concept MsgPackStringOrBinary = MsgPackString<T> || MsgPackBinary<T>;

template<typename T>
concept MsgPackFloatingPoint = std::is_floating_point_v<T>
    && (sizeof(T) == 4 || sizeof(T) == 8)
    && (limits<T>::radix == 2);

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
    return serialized_object_;
  }

  void clear()
  {
    serialized_object_.clear();
  }

  std::error_code ec{};

private:
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
    uint8_t t = MsgPackMap<T> ? FormatConstants::map16 : FormatConstants::array16;
    size_t len = match(a_value.size())(
        pattern | (_ < 16) =
            [&] { return sizeof(uint8_t); },
        pattern | (_ < limits<uint16_t>::max()) =
            [&] { return serialized_object_.emplace_back(t), sizeof(uint16_t); },
        pattern | (_ < limits<uint32_t>::max()) =
            [&] { return serialized_object_.emplace_back(t + 1), sizeof(uint32_t); },
        pattern | _ =
            [&]() { return (ec = PackerError::LengthError), 0; }
    );

    if (len == 0) return;
    uint8_t mask = (a_value.size() < 16) ? (MsgPackMap<T> ? 0b10000000 : 0b10010000) : 0x00;
    for (auto i = len; i > 0; --i)
      serialized_object_.emplace_back(uint8_t(a_value.size() >> (8U * (i - 1)) | mask & 0xFF));

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
    uint8_t t = MsgPackString<T> ? FormatConstants::str8 : FormatConstants::bin8;

    if (MsgPackString<T> && a_value.size() < 32) {
      serialized_object_.emplace_back(uint8_t(a_value.size()) | 0b10100000);
    } else {
      auto len = match(a_value.size())(
          pattern | (_ < limits<uint8_t>::max()) =
              [&]() { return serialized_object_.emplace_back(t), sizeof(uint8_t); },
          pattern | (_ < limits<uint16_t>::max()) =
              [&]() { return serialized_object_.emplace_back(t + 1), sizeof(uint16_t); },
          pattern | (_ < limits<uint32_t>::max()) =
              [&]() { return serialized_object_.emplace_back(t + 2), sizeof(uint32_t); },
          pattern | _ =
              [&]() { return (ec = PackerError::LengthError), 0; }
      );

      if (len == 0) return;
      for (auto i = len; i > 0; --i)
        serialized_object_.emplace_back(uint8_t(a_value.size() >> (8U * (i - 1)) & 0xFF));
    }

    for (const auto &el : a_value) serialized_object_.emplace_back(static_cast<uint8_t>(el));
  }

  template<MsgPackIntegral T>
  void pack_type(const T &a_value)
  {
    if (ec) return;

    size_t len{0};
    uint64_t value64 = std::make_unsigned_t<T>(a_value);
    for (len = 8; len > 1; --len)
      if ((uint8_t(value64 >> 8U * (len - 1)) & 0xFFL) != 0) break;

    auto is_signed = std::is_signed_v<T>;

    uint8_t t = match(len)(
        pattern | _ > 4 = [&]() { return (len = 8, is_signed ? int64 : uint64); },
        pattern | _ > 2 = [&]() { return (len = 4, is_signed ? int32 : uint32); },
        pattern | _ > 1 = [&]() { return (len = 2, is_signed ? int16 : uint16); },
        pattern | _ = [&]() {
          return ((uint8_t(a_value & 0x80) == 0) || (uint8_t(a_value & 0xE0) == 0xE0)) ?
                 (len = 0, 0) : (is_signed ? int8 : uint8);
        }
    );

    (t > 0) ? serialized_object_.emplace_back(t) : serialized_object_.emplace_back(uint8_t(a_value));
    for (auto i = len; i > 0U; --i)
      serialized_object_.emplace_back(uint8_t(value64 >> (8U * (i - 1)) & 0xFFL));
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
        pattern | 4 = [&]() { serialized_object_.emplace_back(FormatConstants::float32); },
        pattern | 8 = [&]() { serialized_object_.emplace_back(FormatConstants::float64); }
    );

    for (auto i = sizeof(T); i > 0; --i)
      serialized_object_.emplace_back(uint8_t(ieee754_float >> (8U * (i - 1)) & 0xFF));
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
    serialized_object_.emplace_back(FormatConstants::nil);
  }

  template<>
  void pack_type(const bool &a_value)
  {
    if (ec) return;
    if (a_value)
      serialized_object_.emplace_back(FormatConstants::true_bool);
    else
      serialized_object_.emplace_back(FormatConstants::false_bool);
  }

private:
  std::vector<uint8_t> serialized_object_;
};

class Unpacker
{
public:
  Unpacker(const Unpacker &) = delete;
  Unpacker(Unpacker &&) = delete;
  Unpacker &operator=(Unpacker &) = delete;
  Unpacker &operator=(Unpacker &&) = delete;

  Unpacker() : begin_(nullptr), end_(nullptr)
  {};

  explicit Unpacker(const uint8_t *a_start, std::size_t a_size) : begin_(a_start), end_(a_start + a_size)
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
    begin_ = a_begin;
    end_ = begin_ + a_size;
  }

  std::error_code ec{};

private:
  uint8_t current_byte()
  {
    return (begin_ < end_) ? *begin_ : (ec = UnpackerError::OutOfRange, 0);
  }

  void next(int64_t a_bytes = 1)
  {
    if (end_ - begin_ >= 0)
      begin_ += a_bytes;
    else
      ec = UnpackerError::OutOfRange;
  }

  template<typename T>
  void unpack_type(T &a_value)
  {
    if (ec) return;
    auto recursive_data = std::vector<uint8_t>{};
    unpack_type(recursive_data);

    auto recursive_unpacker = Unpacker{recursive_data.data(), recursive_data.size()};
    a_value.pack(recursive_unpacker);
    ec = recursive_unpacker.ec;
  }

  template<MsgPackIntegral T>
  void unpack_type(T &a_value)
  {
    if (ec) return;
    auto is_overflow = [&](size_t act, size_t min) { return (act < min) && (ec = UnpackerError::IntegerOverflow, true); };
    uint8_t byte_value{current_byte()};

    uint8_t len = match(byte_value)(
        //@formatter:off
        pattern | or_(int64, uint64) = [&]() { return is_overflow(sizeof(T), 8) ? 0 : 8; },
        pattern | or_(int32, uint32) = [&]() { return is_overflow(sizeof(T), 4) ? 0 : 4; },
        pattern | or_(int16, uint16) = [&]() { return is_overflow(sizeof(T), 2) ? 0 : 2; },
        pattern | or_(int8 , uint8 ) = [&]() { return is_overflow(sizeof(T), 1) ? 0 : 1; },
        //@formatter:on
        pattern | _ = [&] {
          return ((byte_value & 0x80) == 0x00) || ((byte_value & 0xE0) == 0xE0) ?
                 (a_value = byte_value, 0) : (ec = UnpackerError::DataNotMatchType, 0);
        }
    );

    if (next(), len > 0) {
      uint64_t value64{0};

      for (auto i = len; i > 0; --i) {
        value64 |= uint64_t(current_byte()) << 8U * (i - 1);
        next();
      }

      a_value = T(value64);
    }
  }

  template<MsgPackArrayOrMap T>
  void unpack_type(T &a_value)
  {
    if (ec) return;
    uint32_t len = [&]() {
      auto l = match(current_byte())(//@formatter:off
          pattern | or_(array32, map32) = expr(4),
          pattern | or_(array16, map16) = expr(2),
          pattern | _ = expr(0) //@formatter:on
      );

      uint32_t result{0};
      if (l > 0) {
        next();
        for (auto i = l; i > 0; i--) { (result += uint32_t(current_byte()) << 8 * (i - 1)), next(); }
      } else {
        (result = current_byte() & 0b00001111), next();
      }
      return result;
    }();

    if (is_stdarray<T>::value && a_value.size() != len) {
      ec = UnpackerError::BadStdArraySize;
      return;
    }

    for (auto i = 0U; i < len; i++) {
      if constexpr (MsgPackArray<T>) {
        typename T::value_type value{};
        unpack_type(value);
        if constexpr (is_stdarray<T>::value)
          a_value[i] = std::move(value);
        else
          a_value.emplace_back(value);
      } else {
        typename T::key_type key{};
        typename T::mapped_type value{};
        unpack_type(key);
        unpack_type(value);
        a_value.insert_or_assign(key, value);
      }
    }
  }

  template<MsgPackStringOrBinary T>
  void unpack_type(T &a_value)
  {
    if (ec) return;
    uint32_t len = [&]() {
      auto l = match(current_byte())(//@formatter:off
          pattern | or_(str32, bin32) = expr(4),
          pattern | or_(str16, bin16) = expr(2),
          pattern | or_(str8,  bin8 ) = expr(1),
          pattern | _ = expr(0) //@formatter:on
      );

      uint32_t result{0};
      if (l > 0) {
        next();
        for (auto i = l; i > 0; i--) { (result += uint32_t(current_byte()) << 8 * (i - 1)), next(); };
      } else {
        (result = current_byte() & 0b00011111), next();
      }
      return result;
    }();

    if (begin_ + len <= end_)
      (a_value = T{begin_, begin_ + len}), next(len);
    else
      ec = UnpackerError::OutOfRange;
  }

  template<MsgPackFloatingPoint T>
  void unpack_type(T &a_value)
  {
    if (ec) return;
    match(current_byte())(
        // Stored as IEEE-754 floating point format.
        pattern | or_(float64, float32) = [&]() {
          using int_t = std::conditional_t<sizeof(T) == 4, uint32_t, uint64_t>;
          int_t data{0};
          next();

          for (auto i = sizeof(T); i > 0; --i) {
            data += std::conditional_t<sizeof(T) == 8, uint64_t, uint8_t>(current_byte()) << 8 * (i - 1);
            next();
          }

          auto bits = std::bitset<8 * sizeof(T)>(data);
          T mantissa = 1.0;

          for (auto i = sizeof(T) == 8 ? 52U : 23U; i > 0; --i)
            if (bits[i - 1])
              mantissa += 1.0 / (int_t(1) << ((sizeof(T) == 8 ? 53U : 24U) - i));

          if (bits[8 * sizeof(T) - 1])
            mantissa *= -1;

          std::conditional_t<sizeof(T) == 8, uint16_t, uint8_t> exponent{0};

          for (auto i = 0U; i < (sizeof(T) == 8 ? 11 : 8); ++i)
            exponent += bits[i + (sizeof(T) == 8 ? 52 : 23)] << i;

          exponent -= (sizeof(T) == 8 ? 1023 : 127);
          a_value = ldexp(mantissa, exponent);
        },

        // Could have been stored in integral format: int8, int16, int32, int64, or fixint.
        pattern | _ = [&]() {
          int64_t data{0};
          unpack_type(data);
          a_value = T(data);
        }
    );
  }

  template<typename ClockT, typename DurationT>
  void unpack_type(std::chrono::time_point<ClockT, DurationT> &a_value)
  {
    if (ec) return;
    using timepoint_t = typename std::chrono::time_point<ClockT, DurationT>;
    using rep_t = typename timepoint_t::rep;

    auto placeholder = rep_t{};

    unpack_type(placeholder);
    a_value = static_cast<timepoint_t>(DurationT(placeholder));
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

private:
  const uint8_t *begin_;
  const uint8_t *end_;
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
  auto packer = Packer{};
  a_packable.pack(packer);
  a_ec = packer.ec;
  return packer.vector();
}

template<Packable T>
std::vector<uint8_t> pack(T &&a_packable, std::error_code &a_ec)
{
  auto packer = Packer{};
  a_packable.pack(packer);
  a_ec = packer.ec;
  return packer.vector();
}

template<Packable T>
std::vector<uint8_t> pack(T &a_packable)
{
  std::error_code ec;
  return pack(std::forward<T &>(a_packable), ec);
}

template<Packable T>
std::vector<uint8_t> pack(T &&a_packable)
{
  std::error_code ec;
  return pack(std::forward<T &&>(a_packable), ec);
}

template<Packable T>
T unpack(const uint8_t *a_start, const std::size_t a_size, std::error_code &a_ec)
{
  auto packable = T{};
  auto unpacker = Unpacker(a_start, a_size);
  packable.pack(unpacker);
  a_ec = unpacker.ec;
  return packable;
}

template<Packable T>
T unpack(const uint8_t *a_start, const std::size_t a_size)
{
  std::error_code ec{};
  return unpack<T>(a_start, a_size, ec);
}

template<Packable T>
T unpack(const std::vector<uint8_t> &a_data, std::error_code &a_ec)
{
  return unpack<T>(a_data.data(), a_data.size(), a_ec);
}

template<Packable T>
T unpack(const std::vector<uint8_t> &a_data)
{
  std::error_code ec;
  return unpack<T>(a_data.data(), a_data.size(), ec);
}

}
#endif
